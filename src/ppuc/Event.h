/*
  Event.h
  Created by Markus Kalkbrenner, 2021.

  Play more pinball!
*/

#ifndef EVENT_h
#define EVENT_h

#define PLATFORM_WPC           1
#define PLATFORM_DATA_EAST     2
#define PLATFORM_SYS11         3

#define EVENT_SOURCE_ANY      42 // "*"
#define EVENT_SOURCE_DEBUG    66 // "B" Debug
#define EVENT_CONFIGURATION   67 // "C" Configure I/O
#define EVENT_SOURCE_DMD      68 // "D" VPX/DOF/PUP
#define EVENT_SOURCE_EVENT    69 // "E" VPX/DOF/PUP common event from different system, like
#define EVENT_SOURCE_EFFECT   70 // "F" custom event from running Effect
#define EVENT_SOURCE_GI       71 // "G" WPC GI
#define EVENT_SOURCE_LIGHT    76 // "L" VPX/DOF/PUP lights, mainly playfield inserts
#define EVENT_NULL            78 // "N" NULL event
#define EVENT_SOURCE_SOUND    79 // "O" sound command
#define EVENT_POLL_EVENTS     80 // "P" Poll events command, mainly read switches
#define EVENT_SOURCE_SOLENOID 83 // "S" VPX/DOF/PUP includes flashers
#define EVENT_SOURCE_SWITCH   87 // "W" VPX/DOF/PUP

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;

struct Event {
    UINT8 sourceId;
    UINT16 eventId;
    UINT8 value;

    Event(UINT8 sId, UINT16 eId) {
        sourceId = sId;
        eventId = eId;
        value = 1;
    }

    Event(UINT8 sId, UINT16 eId, UINT8 v) {
        sourceId = sId;
        eventId = eId;
        value = v;
    }

    bool operator==(const Event &other) const {
        return this->sourceId == other.sourceId
            && this->eventId == other.eventId
            && this->value == other.value;
    }

    bool operator!=(const Event &other) const {
        return !(*this == other);
    }
};

#endif

/* Event examples
 *
 * EVENT_CONFIGURATION
 *   sourceId: "C"
 *   eventId:  I/O board number eventId&1111000000000000, max 16 boards
 *             kind of I/O port eventId&0000111100000000, 0 is solenoid
 *                                                        1 is switch
 *                                                        2 is lamp (light matrix) red
 *                                                        3 is lamp (light matrix) green
 *                                                        4 is lamp (light matrix) blue
 *                                                        5 is lamp (light matrix) white
 *                                                        6 is flasher red
 *                                                        7 is flasher green
 *                                                        8 is flasher blue
 *                                                        9 is flasher white
 *             number           eventId&0000000011111111, number in light matrix, switch matrix or number of high power output
 *   value:    0-255, PWM value for solenoids
 */