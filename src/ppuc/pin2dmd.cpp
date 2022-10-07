#include <string.h>
#if defined(__linux__) || defined(__unix__)
#include <libusb-1.0/libusb.h>
#elif defined(__APPLE__)
#include "/usr/local/Cellar/libusb/1.0.26/include/libusb-1.0/libusb.h"
#endif
#include "pin2dmd.h"

//define PIN2DMD vendor id and product id
#define VID 0x0314
#define PID 0xe457

//endpoints for PIN2DMD communication
#define EP_IN 0x81
#define EP_OUT 0x01

bool Pin2dmd = false;
bool Pin2dmdXL = false;
bool Pin2dmdHD = false;

struct libusb_device **devs;
struct libusb_device_handle *MyLibusbDeviceHandle = NULL;
struct libusb_device_descriptor desc;
struct libusb_context *ctx = NULL;

UINT8 OutputBuffer[65536] = {};

int Pin2dmdInit() {
    static int ret = 0;
    static unsigned char product[256] = {};
    static const char* string = NULL;

    libusb_init(&ctx); /* initialize the library */

    int device_count = libusb_get_device_list(ctx, &devs);

    //Now look through the list that we just populated. We are trying to see if any of them match our device.
    int i;
    for (i = 0; i < device_count; i++) {
        libusb_get_device_descriptor(devs[i], &desc);
        if (VID == desc.idVendor && PID == desc.idProduct) {
            break;
        }
    }

    if (VID == desc.idVendor && PID == desc.idProduct) {
        libusb_open(devs[i], &MyLibusbDeviceHandle);
    }
    else {
        return 0;
    }

    libusb_free_device_list(devs, 1);

    if (MyLibusbDeviceHandle == NULL) {
        libusb_close(MyLibusbDeviceHandle);
        libusb_exit(ctx);
        return 0;
    }

    ret = libusb_get_string_descriptor_ascii(MyLibusbDeviceHandle, desc.iProduct, product, 256);

    if (libusb_claim_interface(MyLibusbDeviceHandle, 0) < 0)  //claims the interface with the Operating System
    {
        //Closes a device opened since the claim interface is failed.
        libusb_close(MyLibusbDeviceHandle);
        libusb_exit(ctx);
        return 0;
    }

    string = (const char*)product;
    if (ret > 0) {
        if (strcmp(string, "PIN2DMD") == 0) {
            Pin2dmd = true;
            ret = 1;
        }
        else if (strcmp(string, "PIN2DMD XL") == 0) {
            Pin2dmdXL = true;
            ret = 2;
        }
        else if (strcmp(string, "PIN2DMD HD") == 0) {
            Pin2dmdHD = true;
            ret = 3;
        }
        else {
            ret = 0;
        }
    }

    return ret;
}

void Pin2dmdRender(UINT16 width, UINT16 height, UINT8* Buffer, int bitDepth) {
    UINT8 Header[4] = {};

    if (width == 256 && height == 64) {
        if (Pin2dmd || Pin2dmdXL) {
            /* todo: implement scaleDown() for Capcom Football Flipper
            UINT8 scaleBuffer[(128 * 32 / 8 * 6) + 4] = {};
            //scale down to 128x32
            for (int i = 0; i < bitDepth; i++)
                scaleDown(scaleBuffer + 4 + (i * 512), OutputBuffer + 4 + (i * 2048), 2048);
            if (bitDepth == 2) {
                scaleBuffer[0] = 0x81; // frame sync bytes
                scaleBuffer[1] = 0xC3;
                scaleBuffer[2] = 0xE8;
                scaleBuffer[3] = 0x2;
                libusb_bulk_transfer(MyLibusbDeviceHandle, EP_OUT, scaleBuffer, 1028, NULL, 1000);
            }
            else if (bitDepth == 4) {
                scaleBuffer[0] = 0x81; // frame sync bytes
                scaleBuffer[1] = 0xC3;
                scaleBuffer[2] = 0xE7;
                scaleBuffer[3] = 0x0;
                libusb_bulk_transfer(MyLibusbDeviceHandle, EP_OUT, scaleBuffer, 2052, NULL, 1000);
            }
            else if (bitDepth == 6) {
                scaleBuffer[0] = 0x81; // frame sync bytes
                scaleBuffer[1] = 0xC3;
                scaleBuffer[2] = 0xE8;
                scaleBuffer[3] = 0x6;
                libusb_bulk_transfer(MyLibusbDeviceHandle, EP_OUT, scaleBuffer, 3076, NULL, 1000);
            }
             */
        }
        else if (Pin2dmdHD) {
            if (bitDepth == 2) {
                UINT8 OutputBuffer[4100] = {};
                Header[0] = 0x81;
                Header[1] = 0xc3;
                Header[2] = 0xe8;
                Header[3] = 8; //number 512 byte chunks
                memcpy(&OutputBuffer[4], Buffer, 4096);
                libusb_bulk_transfer(MyLibusbDeviceHandle, EP_OUT, OutputBuffer, 4100, NULL, 1000);
            }
            else if (bitDepth == 4) {
                UINT8 OutputBuffer[8196] = {};
                Header[0] = 0x81;
                Header[1] = 0xc3;
                Header[2] = 0xe8;
                Header[3] = 16; //number 512 byte chunks
                memcpy(&OutputBuffer[4], Buffer, 8192);
                libusb_bulk_transfer(MyLibusbDeviceHandle, EP_OUT, OutputBuffer, 8196, NULL, 1000);
            }
            else if (bitDepth == 6) {
                UINT8 OutputBuffer[12292] = {};
                Header[0] = 0x81;
                Header[1] = 0xc3;
                Header[2] = 0xe8;
                Header[3] = 24; //number 512 byte chunks
                memcpy(&OutputBuffer[4], Buffer, 12288);
                libusb_bulk_transfer(MyLibusbDeviceHandle, EP_OUT, OutputBuffer, 12292, NULL, 1000);
            }
        }
    }
    else if (width == 192 && height == 64) {
        if (Pin2dmd) {
            /* todo: implement scaleDown() for Data East big DMD
            UINT8 scaleBuffer[(128 * 32 / 8 * 6) + 4] = {};
            //scale down to 128x32
            for (int i = 0; i < bitDepth; i++)
                scaleDown(scaleBuffer + 4 + (i * 512), OutputBuffer + 4 + (i * 1536), 1536);
            if (bitDepth == 2) {
                scaleBuffer[0] = 0x81; // frame sync bytes
                scaleBuffer[1] = 0xC3;
                scaleBuffer[2] = 0xE8;
                scaleBuffer[3] = 0x2;
                libusb_bulk_transfer(MyLibusbDeviceHandle, EP_OUT, scaleBuffer, 1028, NULL, 1000);
            }
            else if (bitDepth == 4) {
                scaleBuffer[0] = 0x81; // frame sync bytes
                scaleBuffer[1] = 0xC3;
                scaleBuffer[2] = 0xE7;
                scaleBuffer[3] = 0x0;
                libusb_bulk_transfer(MyLibusbDeviceHandle, EP_OUT, scaleBuffer, 2052, NULL, 1000);
            }
            else if (bitDepth == 6) {
                scaleBuffer[0] = 0x81; // frame sync bytes
                scaleBuffer[1] = 0xC3;
                scaleBuffer[2] = 0xE8;
                scaleBuffer[3] = 0x6;
                libusb_bulk_transfer(MyLibusbDeviceHandle, EP_OUT, scaleBuffer, 3076, NULL, 1000);
            }
             */
        }
        else if (Pin2dmdXL || Pin2dmdHD) {
            if (bitDepth == 2) {
                UINT8 OutputBuffer[3076] = {};
                OutputBuffer[0] = 0x81;
                OutputBuffer[1] = 0xc3;
                OutputBuffer[2] = 0xe8;
                OutputBuffer[3] = 6; //number 512 byte chunks
                memcpy(&OutputBuffer[4], Buffer, 3072);
                libusb_bulk_transfer(MyLibusbDeviceHandle, EP_OUT, OutputBuffer, 3076, NULL, 1000);
            }
            else if (bitDepth == 4) {
                UINT8 OutputBuffer[6148] = {};
                OutputBuffer[0] = 0x81;
                OutputBuffer[1] = 0xc3;
                OutputBuffer[2] = 0xe8;
                OutputBuffer[3] = 12; //number 512 byte chunks
                memcpy(&OutputBuffer[4], Buffer, 6144);
                libusb_bulk_transfer(MyLibusbDeviceHandle, EP_OUT, OutputBuffer, 6148, NULL, 1000);
            }
            else if (bitDepth == 6) {
                UINT8 OutputBuffer[9220] = {};
                OutputBuffer[0] = 0x81;
                OutputBuffer[1] = 0xc3;
                OutputBuffer[2] = 0xe8;
                OutputBuffer[3] = 18; //number 512 byte chunks
                memcpy(&OutputBuffer[4], Buffer, 9216);
                libusb_bulk_transfer(MyLibusbDeviceHandle, EP_OUT, OutputBuffer, 9220, NULL, 1000);
            }
        }
    }
    else if (width == 128 && height == 32) {
        if (bitDepth == 2) {
            if (Pin2dmd || Pin2dmdXL || Pin2dmdHD) {
                UINT8 OutputBuffer[1028] = {};
                OutputBuffer[0] = 0x81;
                OutputBuffer[1] = 0xc3;
                OutputBuffer[2] = 0xe8;
                OutputBuffer[3] = 2; //number 512 byte chunks
                memcpy(&OutputBuffer[4], Buffer, 1024);
                libusb_bulk_transfer(MyLibusbDeviceHandle, EP_OUT, OutputBuffer, 1028, NULL, 1000);
            }
        }
        else if (bitDepth == 4) {
            if (Pin2dmd || Pin2dmdXL || Pin2dmdHD) {
                UINT8 OutputBuffer[2052] = {};
                OutputBuffer[0] = 0x81;
                OutputBuffer[1] = 0xc3;
                OutputBuffer[2] = 0xe7;
                OutputBuffer[3] = 0x00;
                memcpy(&OutputBuffer[4], Buffer, 2048);
                libusb_bulk_transfer(MyLibusbDeviceHandle, EP_OUT, OutputBuffer, 2052, NULL, 1000);
            }
        }
        else if (bitDepth == 6) {
            if (Pin2dmd || Pin2dmdXL || Pin2dmdHD) {
                UINT8 OutputBuffer[3076] = {};
                OutputBuffer[0] = 0x81;
                OutputBuffer[1] = 0xc3;
                OutputBuffer[2] = 0xe8;
                OutputBuffer[3] = 6; //number 512 byte chunks
                memcpy(&OutputBuffer[4], Buffer, 3072);
                libusb_bulk_transfer(MyLibusbDeviceHandle, EP_OUT, OutputBuffer, 3076, NULL, 1000);
            }
        }
        else if (bitDepth == 15) {
            if (Pin2dmd) {
                UINT8 OutputBuffer[7684] = {};
                OutputBuffer[0] = 0x81;
                OutputBuffer[1] = 0xc3;
                OutputBuffer[2] = 0xe8;
                OutputBuffer[3] = 15; //number 512 byte chunks
                memcpy(&OutputBuffer[4], Buffer, 7680);
                libusb_bulk_transfer(MyLibusbDeviceHandle, EP_OUT, OutputBuffer, 7684, NULL, 1000);
            }
        }
    }
}

void Pin2dmdRenderWpcRaw(UINT16 width, UINT16 height, UINT8* Buffer) {
    if (width == 128 && height == 32) {
        if (Pin2dmd || Pin2dmdXL || Pin2dmdHD) {
            UINT8 OutputBuffer[1540] = {};
            OutputBuffer[0] = 0x52; // WPC RAW mode
            OutputBuffer[1] = 0x80;
            OutputBuffer[2] = 0x20;
            OutputBuffer[3] = 3; // number of 512 byte chunks
            memcpy(&OutputBuffer[4], Buffer, 1536);
            libusb_bulk_transfer(MyLibusbDeviceHandle, EP_OUT, OutputBuffer, 1540, NULL, 1000);
        }
    }
}
