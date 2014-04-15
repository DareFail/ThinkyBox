//
//  neuroSkyEEG.h
//  ThinkBox
//
//  Created by James Steinberg on 4/11/14.
//  Copyright (c) 2014 ___JamesSteinberg___. All rights reserved.
//

#ifndef __ThinkBox__neuroSkyEEG__
#define __ThinkBox__neuroSkyEEG__

#include <iostream>
#include <signal.h>
#include <CoreFoundation/CoreFoundation.h>

class NeuroSkyEEG
{
private:
    /**
     * Baud rate for use with TG_Connect() and TG_SetBaudrate().
     */
#define TG_BAUD_1200         1200
#define TG_BAUD_2400         2400
#define TG_BAUD_4800         4800
#define TG_BAUD_9600         9600
#define TG_BAUD_57600       57600
#define TG_BAUD_115200     115200
    
    /**
     * Data format for use with TG_Connect() and TG_SetDataFormat().
     */
#define TG_STREAM_PACKETS      0
#define TG_STREAM_5VRAW        1
#define TG_STREAM_FILE_PACKETS 2
    
    /**
     * Data type that can be requested from TG_GetValue().
     */
#define TG_DATA_BATTERY      0
#define TG_DATA_POOR_SIGNAL  1
#define TG_DATA_ATTENTION    2
#define TG_DATA_MEDITATION   3
#define TG_DATA_RAW          4
#define TG_DATA_DELTA        5
#define TG_DATA_THETA        6
#define TG_DATA_ALPHA1       7
#define TG_DATA_ALPHA2       8
#define TG_DATA_BETA1        9
#define TG_DATA_BETA2       10
#define TG_DATA_GAMMA1      11
#define TG_DATA_GAMMA2      12
#define TG_DATA_BLINK_STRENGTH 37
    
    
    /*
     * ThinkGear function pointers
     */
    
    int (*TG_GetDriverVersion)() = NULL;
    int (*TG_GetNewConnectionId)() = NULL;
    int (*TG_Connect)(int, const char *, int, int) = NULL;
    int (*TG_ReadPackets)(int, int) = NULL;
    float (*TG_GetValue)(int, int) = NULL;
    int (*TG_EnableBlinkDetection)(int, int) = NULL;
    int (*TG_GetValueStatus)(int, int) = NULL;
    
    const char * portname = "/dev/tty.MindWaveMobile-DevA";       // port name used to be argv[1]
    int retVal;                       // return values from TG functions
    
    int numPackets;                    // number of packets returned from ReadPackets
    float signalQuality;             // poor signal status 1-255
    float attention;                 // eSense attention 1-100
    float meditation;                // eSense meditation 1-100
    float didBlink;             // eSense blinking  0 or 1
    float blinkStrength;          // strength of the blink 1-255

public:
    
    bool initialize(void);
    void setEEGData(void);
    void enableBlink(void);
    
    // getters
    float getSignalQuality(void) { return signalQuality; };             // poor signal status 1-255
    float getAttention(void) { return attention; };                 // eSense attention 1-100
    float getMeditation(void) { return meditation; };                // eSense meditation 1-100
    float getDidBlink(void) { return didBlink; };             // eSense blinking  0 or 1
    float getBlinkStrength(void) { return blinkStrength; };          // strength of the blink 1-255
};

void siginthandler(int sig);

#endif /* defined(__ThinkBox__neuroSkyEEG__) */


