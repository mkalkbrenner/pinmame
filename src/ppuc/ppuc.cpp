#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <chrono>
#include <thread>

#include "yaml-cpp/yaml.h"
#include "serialib/serialib.h"

#include "libpinmame.h"
#include "pin2dmd/pin2dmd.h"
#include "Event.h"

#if defined(_WIN32) || defined(_WIN64)
#define CLEAR_SCREEN "cls"
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#define CLEAR_SCREEN "clear"
#endif

typedef unsigned char UINT8;
typedef unsigned short UINT16;

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
	return p_audioInfo->samplesPerFrame;
}

int CALLBACK OnAudioUpdated(void* p_buffer, int samples) {
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
    while ((c = getopt(argc, argv, "cs:")) != -1) {
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
    std::string c_serial = ppuc_config["serial"].as<std::string>();
    std::string c_rom = ppuc_config["rom"].as<std::string>();

    system(CLEAR_SCREEN);

    int pin2dmd = Pin2dmdInit();
    printf("PIN2DMD: %d\n", pin2dmd);

    // Connection to serial port
    char errorOpening = serial.openDevice(opt_serial ? opt_serial : c_serial.c_str(), 115200);

    // If connection fails, return the error code otherwise, display a success message
    if (errorOpening!=1) {
        return errorOpening;
    }

    // Disable DTR, otherwise Arduino will reset permanently.
    serial.clearDTR();

    printf("RTS %d\n", serial.isRTS());
    printf("DTR %d\n", serial.isDTR());

    msg[0] = (UINT8) 255;
    msg[5] = (UINT8) 255;

    PinmameConfig config = {
            AUDIO_FORMAT_FLOAT,
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
