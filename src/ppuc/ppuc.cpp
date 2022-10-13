#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <chrono>
#include <queue>
#include <thread>

#include <AL/al.h>
#include <AL/alc.h>

#include "yaml-cpp/yaml.h"

#include "Event.h"
#include "libpinmame.h"
#include "pin2dmd/pin2dmd.h"
#include "serialib/serialib.h"

#if defined(_WIN32) || defined(_WIN64)
#define CLEAR_SCREEN "cls"
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#define CLEAR_SCREEN "clear"
#endif

typedef unsigned char UINT8;
typedef unsigned short UINT16;

#define MAX_AUDIO_BUFFERS 4
#define MAX_AUDIO_QUEUE_SIZE 10


ALuint _audioSource;
ALuint _audioBuffers[MAX_AUDIO_BUFFERS];
std::queue<void*> _audioQueue;
int _audioChannels;
int _audioSampleRate;


UINT8 msg[6] = {0};
// Serial object
serialib serial;

YAML::Node ppuc_config;

void CALLBACK Game(PinmameGame* game) {
	printf("Game(): name=%s, description=%s, manufacturer=%s, year=%s, flags=%lu, found=%d\n",
		game->name, game->description, game->manufacturer, game->year, (unsigned long)game->flags, game->found);
}

void CALLBACK OnStateUpdated(int state) {
	printf("OnStateUpdated(): state=%d\n", state);

	if (!state) {
		exit(1);
	}
	else {
		PinmameMechConfig mechConfig;
		memset(&mechConfig, 0, sizeof(mechConfig));

		mechConfig.sol1 = 11;
		mechConfig.length = 240;
		mechConfig.steps = 240;
		mechConfig.type = NONLINEAR | REVERSE | ONESOL;
		mechConfig.sw[0].swNo = 32;
		mechConfig.sw[0].startPos = 0;
		mechConfig.sw[0].endPos = 5;

		PinmameSetMech(0, &mechConfig);
	}
}

void CALLBACK OnDisplayAvailable(int index, int displayCount, PinmameDisplayLayout* p_displayLayout) {
	printf("OnDisplayAvailable(): index=%d, displayCount=%d, type=%d, top=%d, left=%d, width=%d, height=%d, depth=%d, length=%d\n",
		index,
		displayCount,
		p_displayLayout->type,
		p_displayLayout->top,
		p_displayLayout->left,
		p_displayLayout->width,
		p_displayLayout->height,
		p_displayLayout->depth,
		p_displayLayout->length);
}

void CALLBACK OnDisplayUpdated(int index, void* p_displayData, PinmameDisplayLayout* p_displayLayout) {
	printf("OnDisplayUpdated(): index=%d, type=%d, top=%d, left=%d, width=%d, height=%d, depth=%d, length=%d\n",
		index,
		p_displayLayout->type,
		p_displayLayout->top,
		p_displayLayout->left,
		p_displayLayout->width,
		p_displayLayout->height,
		p_displayLayout->depth,
		p_displayLayout->length);

	if ((p_displayLayout->type & DMD) == DMD) {
        Pin2dmdRender(p_displayLayout->width, p_displayLayout->height, (UINT8 *) p_displayData, p_displayLayout->depth, PinmameGetHardwareGen() & (SAM | SPA));
	}
    else if((p_displayLayout->type & RAWDMD) == RAWDMD) {
        // todo get the number of frames from libpinmame
        Pin2dmdRenderRaw(p_displayLayout->width, p_displayLayout->height, (UINT8 *) p_displayData, 2);
    }
	else {
		//DumpAlphanumeric(index, (UINT16*)p_displayData, p_displayLayout);
	}
}

int CALLBACK OnAudioAvailable(PinmameAudioInfo* p_audioInfo) {
	printf("OnAudioAvailable(): format=%d, channels=%d, sampleRate=%.2f, framesPerSecond=%.2f, samplesPerFrame=%d, bufferSize=%d\n",
		p_audioInfo->format,
		p_audioInfo->channels,
		p_audioInfo->sampleRate,
		p_audioInfo->framesPerSecond,
		p_audioInfo->samplesPerFrame,
		p_audioInfo->bufferSize);

    _audioChannels = p_audioInfo->channels;
    _audioSampleRate = (int) p_audioInfo->sampleRate;

    for (int index = 0; index < MAX_AUDIO_BUFFERS; index++) {
        int bufferSize = p_audioInfo->samplesPerFrame * _audioChannels * sizeof(int16_t);
        void* p_buffer = malloc(bufferSize);
        memset(p_buffer, 0, bufferSize);

        alBufferData(_audioBuffers[index], _audioChannels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16,
                     p_buffer,
                     bufferSize,
                     _audioSampleRate);
    }

    alSourceQueueBuffers(_audioSource, MAX_AUDIO_BUFFERS, _audioBuffers);
    alSourcePlay(_audioSource);

	return p_audioInfo->samplesPerFrame;
}

int CALLBACK OnAudioUpdated(void* p_buffer, int samples) {
    if (_audioQueue.size() >= MAX_AUDIO_QUEUE_SIZE) {
        while (!_audioQueue.empty()) {
            void* p_destBuffer = _audioQueue.front();

            free(p_destBuffer);
            _audioQueue.pop();
        }
    }

    int bufferSize = samples * _audioChannels * sizeof(int16_t);
    void* p_destBuffer = malloc(bufferSize);
    memcpy(p_destBuffer, p_buffer, bufferSize);

    _audioQueue.push(p_destBuffer);

    ALint buffersProcessed;
    alGetSourcei(_audioSource, AL_BUFFERS_PROCESSED, &buffersProcessed);

    if (buffersProcessed <= 0) {
        return samples;
    }

    while (buffersProcessed > 0) {
        ALuint buffer = 0;
        alSourceUnqueueBuffers(_audioSource, 1, &buffer);

        if (_audioQueue.size() > 0) {
            void* p_destBuffer = _audioQueue.front();

            alBufferData(buffer,
                         _audioChannels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16,
                         p_destBuffer,
                         bufferSize,
                         _audioSampleRate);

            free(p_destBuffer);
            _audioQueue.pop();
        }

        alSourceQueueBuffers(_audioSource, 1, &buffer);
        buffersProcessed--;
    }

    ALint state;
    alGetSourcei(_audioSource, AL_SOURCE_STATE, &state);

    if (state != AL_PLAYING) {
        alSourcePlay(_audioSource);
    }

    return samples;
}

void CALLBACK OnSolenoidUpdated(int solenoid, int isActive) {
	printf("OnSolenoidUpdated: solenoid=%d, isActive=%d\n", solenoid, isActive);
}

void CALLBACK OnMechAvailable(int mechNo, PinmameMechInfo* p_mechInfo) {
	printf("OnMechAvailable: mechNo=%d, type=%d, length=%d, steps=%d, pos=%d, speed=%d\n",
		mechNo,
		p_mechInfo->type,
		p_mechInfo->length,
		p_mechInfo->steps,
		p_mechInfo->pos,
		p_mechInfo->speed);
}

void CALLBACK OnMechUpdated(int mechNo, PinmameMechInfo* p_mechInfo) {
	printf("OnMechUpdated: mechNo=%d, type=%d, length=%d, steps=%d, pos=%d, speed=%d\n",
		mechNo,
		p_mechInfo->type,
		p_mechInfo->length,
		p_mechInfo->steps,
		p_mechInfo->pos,
		p_mechInfo->speed);
}

void CALLBACK OnConsoleDataUpdated(void* p_data, int size) {
	printf("OnConsoleDataUpdated: size=%d\n", size);
}

int CALLBACK IsKeyPressed(PINMAME_KEYCODE keycode) {
	return 0;
}

void sendEvent(Event* event) {
    //     = (UINT8) 255;
    msg[1] = (UINT8) event->sourceId;
    msg[2] = event->eventId >> 8;
    msg[3] = event->eventId & 0xff;
    msg[4] = event->value;
    //     = (UINT8) 255;

    if (serial.writeBytes(msg, 6)) printf("Sent Event.\n");

    // delete the event and free the memory
    delete event;
}

int main (int argc, char **argv) {
    char *config_file = NULL;
    char *opt_serial = NULL;

    int c;
    // The options argument is a string that specifies the option characters that are valid for this program. An option
    // character in this string can be followed by a colon (‘:’) to indicate that it takes a required argument.
    while ((c = getopt(argc, argv, "c:s:")) != -1) {
        switch (c) {
            case 'c':
                config_file = optarg;
                break;
            case 's':
                opt_serial = optarg;
                break;
            case '?':
                if (optopt == 'c')
                    fprintf(stderr, "Option -%c requires the config file path as argument.\n", optopt);
                else if (isprint(optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr,
                            "Unknown option character `\\x%x'.\n",
                            optopt);
                return 1;
            default:
                abort();
        }
    }

    ppuc_config = YAML::LoadFile(config_file);
    std::string c_serial = ppuc_config["serialPort"].as<std::string>();
    std::string c_rom = ppuc_config["rom"].as<std::string>();

    const ALCchar *defaultDeviceName = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
    ALCdevice *device = alcOpenDevice(defaultDeviceName);

    ALCcontext *context = alcCreateContext(device, NULL);
    alcMakeContextCurrent(context);

    alGenSources((ALuint) 1, &_audioSource);
    alGenBuffers(MAX_AUDIO_BUFFERS, _audioBuffers);

    // Connection to serial port
    char errorOpening = serial.openDevice(opt_serial ? opt_serial : c_serial.c_str(), 115200);

    // If connection fails, return the error code otherwise, display a success message
    if (errorOpening!=1) {
        printf("Unable to open serial device: %s\n", opt_serial ? opt_serial : c_serial.c_str());
        return errorOpening;
    }

    // Disable DTR, otherwise Arduino will reset permanently.
    serial.clearDTR();

    msg[0] = (UINT8) 255;
    msg[5] = (UINT8) 255;

    system(CLEAR_SCREEN);

    int pin2dmd = Pin2dmdInit();
    printf("PIN2DMD: %d\n", pin2dmd);

    PinmameConfig config = {
            AUDIO_FORMAT_INT16,
            44100,
            "",
            false, // RAW DMD
            &OnStateUpdated,
            &OnDisplayAvailable,
            &OnDisplayUpdated,
            &OnAudioAvailable,
            &OnAudioUpdated,
            &OnMechAvailable,
            &OnMechUpdated,
            &OnSolenoidUpdated,
            &OnConsoleDataUpdated,
            &IsKeyPressed,
    };

#if defined(_WIN32) || defined(_WIN64)
    snprintf((char*)config.vpmPath, MAX_PATH, "%s%s\\pinmame\\", getenv("HOMEDRIVE"), getenv("HOMEPATH"));
#else
    snprintf((char*)config.vpmPath, MAX_PATH, "%s/.pinmame/", getenv("HOME"));
#endif

    PinmameSetConfig(&config);

    PinmameSetHandleKeyboard(0);
    PinmameSetHandleMechanics(0);

    int changedLampStates[PinmameGetMaxLamps() * 2];

	if (PinmameRun(c_rom.c_str()) == OK) {
		while (1) {
			std::this_thread::sleep_for(std::chrono::microseconds(1000));
            int count = PinmameGetChangedLamps(changedLampStates);
            for (int c = 0; c < count;) {
                UINT16 lampNo = changedLampStates[c++];
                UINT8 lampState = changedLampStates[c++] == 0 ? 0 : 1;

                printf("Lamp updated: lampNo=%d, lampState=%d\n",
                       lampNo,
                       lampState);

                Event* event = new Event(EVENT_SOURCE_LIGHT, lampNo, lampState);
                sendEvent(event);
            }
		}
	}

    // Close the serial device
    serial.closeDevice();

	return 0;
}
