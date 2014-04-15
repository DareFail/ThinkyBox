//
//  neuroSkyEEG.cpp
//  ThinkBox
//
//  Created by James Steinberg on 4/11/14.
//  Copyright (c) 2014 ___JamesSteinberg___. All rights reserved.
//

#include "neuroSkyEEG.h"


int connectionID = -1;        // ThinkGear connection handle

CFURLRef bundleURL;           // path reference to bundle
CFBundleRef thinkGearBundle;  // bundle reference

int (*TG_Disconnect)(int) = NULL;
void (*TG_FreeConnection)(int) = NULL;


/**
 * This function handles signal interrupts.
 *
 * Basically perform cleanup on the objects and then exit the program.
 */
void siginthandler(int sig){
    fprintf(stderr, "\nDisconnecting...\n");
    
    // close the connection
    if(connectionID != -1){
        TG_Disconnect(connectionID);
        TG_FreeConnection(connectionID);
    }
    
    // release the bundle references
    if(bundleURL)
        CFRelease(bundleURL);
    
    if(thinkGearBundle)
        CFRelease(thinkGearBundle);
    
    exit(1);
}

bool NeuroSkyEEG::initialize(void)
{
    retVal = -1;                       // return values from TG functions
    
    numPackets = 0;                    // number of packets returned from ReadPackets
    signalQuality = 0.0;             // poor signal status 1-255
    attention = 0.0;                 // eSense attention 1-100
    meditation = 0.0;                // eSense meditation 1-100
    didBlink = 0.0;             // eSense blinking  0 or 1
    blinkStrength = 0.0;          // strength of the blink 1-255
    
    // register the signal interrupt handler
    signal(SIGINT, siginthandler);
    
    srand(time(NULL));
    
    // create the path reference to the bundle
    bundleURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                              CFSTR("/Users/jamespsteinberg/Documents/ThinkBox/ThinkBox/ThinkGear.bundle"),
                                              kCFURLPOSIXPathStyle,
                                              true);
    
    // create the bundle reference
    thinkGearBundle = CFBundleCreate(kCFAllocatorDefault, bundleURL);
    
    // make sure the bundle actually exists
    if(!thinkGearBundle){
        fprintf(stderr, "Error: Could not find ThinkGear.bundle. Does it exist in the current directory?\n");
        exit(1);
    }
    
    // now start setting the function pointers
    TG_GetDriverVersion =   (int (*)())CFBundleGetFunctionPointerForName(thinkGearBundle, CFSTR("TG_GetDriverVersion"));
    TG_GetNewConnectionId = (int (*)())CFBundleGetFunctionPointerForName(thinkGearBundle, CFSTR("TG_GetNewConnectionId"));
    TG_Connect =            (int (*)(int, const char *, int, int))CFBundleGetFunctionPointerForName(thinkGearBundle, CFSTR("TG_Connect"));
    TG_ReadPackets =        (int (*)(int, int))CFBundleGetFunctionPointerForName(thinkGearBundle, CFSTR("TG_ReadPackets"));
    TG_GetValue =           (float (*)(int, int))CFBundleGetFunctionPointerForName(thinkGearBundle, CFSTR("TG_GetValue"));
    TG_Disconnect =         (int (*)(int))CFBundleGetFunctionPointerForName(thinkGearBundle, CFSTR("TG_Disconnect"));
    TG_FreeConnection =     (void (*)(int))CFBundleGetFunctionPointerForName(thinkGearBundle, CFSTR("TG_FreeConnection"));
    TG_EnableBlinkDetection =     (int (*)(int, int))CFBundleGetFunctionPointerForName(thinkGearBundle, CFSTR("TG_EnableBlinkDetection"));
    TG_GetValueStatus =          (int (*)(int, int))CFBundleGetFunctionPointerForName(thinkGearBundle, CFSTR("TG_GetValueStatus"));
    
    // check for any invalid function pointers
    if(!TG_GetDriverVersion || !TG_GetNewConnectionId || !TG_Connect || !TG_ReadPackets ||
       !TG_GetValue || !TG_Disconnect || !TG_FreeConnection || !TG_EnableBlinkDetection || !TG_GetValueStatus){
        fprintf(stderr, "Error: Expected functions in ThinkGear.bundle were not found. Are you using the right version?\n");
        exit(1);
    }
    
    // get the connection ID
    connectionID = TG_GetNewConnectionId();
    
    fprintf(stderr, "Connecting to %s ... ", portname);
    
    // attempt to connect
    retVal = TG_Connect(connectionID, portname, TG_BAUD_9600, TG_STREAM_PACKETS);

    return retVal;
}


void NeuroSkyEEG::setEEGData()
{
    // read the packets from the output stream
    numPackets = TG_ReadPackets(connectionID, -1);
    
    // check whether we've received any new packets
    if(numPackets > 0){
        // if so, parse them
        signalQuality = TG_GetValue(connectionID, TG_DATA_POOR_SIGNAL);
        attention = TG_GetValue(connectionID, TG_DATA_ATTENTION);
        meditation = TG_GetValue(connectionID, TG_DATA_MEDITATION);
        didBlink = TG_GetValueStatus(connectionID, TG_DATA_BLINK_STRENGTH);
        blinkStrength = TG_GetValue(connectionID, TG_DATA_BLINK_STRENGTH);
        
        // then output everything
        fprintf(stdout, "\rPoorSig: %3.0f, Att: %3.0f, Med: %3.0f, Blk: %3.0f, blkstrgth: %3.0f", signalQuality, attention, meditation, didBlink, blinkStrength);
        fflush(stdout);
    }
}

void NeuroSkyEEG::enableBlink()
{
    // enable blink tracking
    TG_EnableBlinkDetection(connectionID, 1 );
}
