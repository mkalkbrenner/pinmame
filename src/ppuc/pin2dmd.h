#ifndef PIN2DMD_H
#define PIN2DMD_H

typedef unsigned char UINT8;
typedef unsigned short UINT16;

int Pin2dmdInit();
void Pin2dmdRender(UINT16 width, UINT16 height, UINT8* Buffer, int bitDepth);
void Pin2dmdRenderWpcRaw(UINT16 width, UINT16 height, UINT8* Buffer);

#endif /* PIN2DMD_H */