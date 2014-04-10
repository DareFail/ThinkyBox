//
//  main.cpp
//  ThinkBox
//
//  Created by James Steinberg on 4/8/14.
//  Copyright (c) 2014 ___JamesSteinberg___. All rights reserved.
//

/**
 * This program serves as a simple example of how one can use ThinkGear.bundle inside their Core Foundation
 * (e.g. Cocoa and Carbon-based) apps. For more details on OS X bundles, read:
 * http://developer.apple.com/DOCUMENTATION/CoreFoundation/Conceptual/CFBundles/CFBundles.html
 *
 * Or check the "How to use the ThinkGear API in Xcode (Mac OS X)" document in the ThinkGear documentation.
 *
 * Note: When executing the program, make sure ThinkGear.bundle is in the same current directory.
 */


/* Use the following line on a Windows machine:
 #include <GL/glut.h>
 */
/* This line is for Max OSX  */
#include <GLUT/glut.h>

#include <CoreFoundation/CoreFoundation.h>
#include <string>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

// width of playing field
const int COORDINATE_WIDTH = 14;
const int LEFT_WALL = 0 - (COORDINATE_WIDTH / 2);
const int RIGHT_WALL = (COORDINATE_WIDTH / 2);
const int UPPER_WALL = (COORDINATE_WIDTH / 2);
const int LOWER_WALL = 0 - (COORDINATE_WIDTH / 2);
// where enemy can appear
const float ENEMY_UPPER_BOUNDARY_Y = 5;
const float ENEMY_LOWER_BOUNDARY_Y = -5;
// number of enemies
const int ENEMY_AMOUNT = 2;
const float GRAVITY_MAX = 0.45f;

const float ENEMY_WIDTH = 0.15f;
const float ENEMY_HEIGHT = 3.0f;

float enemyMovement = 0.035f;

// out of bounds for enemy
const int OUT_OF_BOUNDS_X = LEFT_WALL;
// enemy spawn point
const int RESET_X = RIGHT_WALL;

// random y coordinate placement
float placeEnemy()
{
    float b = ENEMY_UPPER_BOUNDARY_Y;
    float a = ENEMY_LOWER_BOUNDARY_Y;
    float random = ((float)rand()) / (float)RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    return a + r;
}

// arrays of enemy positions
float posX_enemy[ENEMY_AMOUNT];
float posY_enemy[ENEMY_AMOUNT];
float posZ_enemy[ENEMY_AMOUNT];
bool collision_enemy[ENEMY_AMOUNT];

const float ORIGINAL_HEIGHT = 1.0f;
const float ORIGINAL_WIDTH = 1.0f;
float posX = 0.0f;
float posY = 0.0f;
float playerHeight = ORIGINAL_HEIGHT;
float playerWidth = ORIGINAL_WIDTH;
int crashes = 0;
int success = 0;
float successPercent = 100.0f;

const float GRAVITY = 0.0045f;
// player position and velocity
float velY = 0.0f;

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

CFURLRef bundleURL;           // path reference to bundle
CFBundleRef thinkGearBundle;  // bundle reference

int connectionID = -1;        // ThinkGear connection handle

/*
 * ThinkGear function pointers
 */

int (*TG_GetDriverVersion)() = NULL;
int (*TG_GetNewConnectionId)() = NULL;
int (*TG_Connect)(int, const char *, int, int) = NULL;
int (*TG_ReadPackets)(int, int) = NULL;
float (*TG_GetValue)(int, int) = NULL;
int (*TG_Disconnect)(int) = NULL;
void (*TG_FreeConnection)(int) = NULL;
int (*TG_EnableBlinkDetection)(int, int) = NULL;
int (*TG_GetValueStatus)(int, int) = NULL;

const char * portname = "/dev/tty.MindWaveMobile-DevA";       // port name used to be argv[1]
int retVal = -1;                       // return values from TG functions

int numPackets = 0;                    // number of packets returned from ReadPackets
float signalQuality = 0.0;             // poor signal status 1-255
float attention = 0.0;                 // eSense attention 1-100
float meditation = 0.0;                // eSense meditation 1-100
float didBlink = 0.0;             // eSense blinking  0 or 1
float blinkStrength = 0.0;          // strength of the blink 1-255

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

// writes text to opengl

void drawString(void * font, char *s, float x, float y, float z)

{
    unsigned int i;
    glRasterPos3f(x, y, z);
    for (i = 0; i < strlen(s); i++)
        glutBitmapCharacter(font, s[i]);
}

void glutStuff(int, char**);
void setEEGData(void);

// handles collision between player and enemy
bool isColliding()
{
    
    for (int i = 0; i < ENEMY_AMOUNT; i++)
    {
        if(collision_enemy[i] == false)
        {
            float playerLeftSide = posX - (playerWidth / 2);
            float playerRightSide = posX + (playerWidth / 2);
            float playerTopSide = posY + (playerHeight / 2);
            float playerBottomSide = posY - (playerHeight / 2);
        
            float enemyLeftSide = posX_enemy[i] - (ENEMY_WIDTH / 2);
            float enemyRightSide = posX_enemy[i] + (ENEMY_WIDTH / 2);
            float enemyTopSide = posY_enemy[i] + (ENEMY_HEIGHT / 2);
            float enemyBottomSide = posY_enemy[i] - (ENEMY_HEIGHT / 2);
        
            if (playerLeftSide < enemyRightSide && playerRightSide > enemyLeftSide && playerTopSide > enemyBottomSide && playerBottomSide < enemyTopSide)
            {
                collision_enemy[i] = true;
                crashes = crashes + 1;
                return true;
            }
        }
    }
    
    //If none of the sides from A are outside B
    return false;
}

void changeSize(int w, int h) {
    
	// Prevent a divide by zero, when window is too short
	// (you cant make a window of zero width).
	if (h == 0)
		h = 1;
    
	float ratio =  w * 1.0 / h;
    
	// Use the Projection Matrix
	glMatrixMode(GL_PROJECTION);
    
	// Reset Matrix
	glLoadIdentity();
    
	// Set the viewport to be the entire window
	glViewport(0, 0, w, h);
    
	// Set the correct perspective.
	gluPerspective(45.0f, ratio, 0.1f, 100.0f);
    
	// Get Back to the Modelview
	glMatrixMode(GL_MODELVIEW);
}

void renderScene(void) {
    
	playerHeight = ((145.0 - attention) / 40.0);
    playerWidth = playerHeight;
    
	// Clear Color and Depth Buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
	// Reset transformations
	glLoadIdentity();
	// Set the camera
	gluLookAt(	0.0f, 0.0f, 10.0f,
              0.0f, 0.0f,  0.0f,
              0.0f, 1.0f,  0.0f);
    
    glPushMatrix();
	glTranslatef( 0.0, posY, 0.0 );
    glScalef( playerHeight, playerHeight, 0.0 );
    
	//Multi-colored side - FRONT
    glBegin(GL_POLYGON);
    
    glColor3f( 1.0, 0.0, 0.0 );     glVertex2f(  0.5, -0.5);      // P1 is red
    glColor3f( 0.0, 1.0, 0.0 );     glVertex2f(  0.5,  0.5);      // P2 is green
    glColor3f( 0.0, 0.0, 1.0 );     glVertex2f( -0.5,  0.5);      // P3 is blue
    glColor3f( 1.0, 0.0, 1.0 );     glVertex2f( -0.5, -0.5);      // P4 is purple
    
    glEnd();
    
    glPopMatrix();
    //obstacles
    // enemy cubes
    for (int i = 0; i < ENEMY_AMOUNT; i++)
    {
        glPushMatrix();
        glTranslatef(posX_enemy[i], posY_enemy[i], posZ_enemy[i]);
        glBegin(GL_QUADS);
       glColor3f( 1.0, 0.0, 0.0 ); glVertex2f(0 - (ENEMY_WIDTH / 2), 0 - (ENEMY_HEIGHT / 2));
        glColor3f( 1.0, 0.0, 1.0 );   glVertex2f((ENEMY_WIDTH / 2), 0 - (ENEMY_HEIGHT / 2));
        glColor3f( 1.0, 0.0, 0.0 ); glVertex2f((ENEMY_WIDTH / 2), (ENEMY_HEIGHT / 2));
         glColor3f( 1.0, 0.0, 1.0 );  glVertex2f((0 - ENEMY_WIDTH / 2), (ENEMY_HEIGHT / 2));
        glEnd();
         glPopMatrix();
        
        
        if (posX_enemy[i] < OUT_OF_BOUNDS_X)
        {
            posX_enemy[i] = RESET_X;
            if (collision_enemy[i])
            {
                collision_enemy[i] = false;
            }
            else
            {
                success = success + 1;
            }
            posY_enemy[i] = placeEnemy();
        }
        
        // moves enemy
        posX_enemy[i] = posX_enemy[i] - enemyMovement;
    }
    
   if (velY <= GRAVITY_MAX)
   {
       velY = velY - GRAVITY;
   }
    
    if (posY >= -2.8 || velY >= 0)
    {
        posY = posY + velY;
    }
    if (didBlink)
    {
        velY = (blinkStrength / 700.0);
    }
    setEEGData();
    
    // title and data
    glColor3f( 1.0, 1.0, 1.0 );
    char data[50];
    sprintf(data, "Crashes: %d     Successes: %d", crashes, success);
    drawString(GLUT_BITMAP_HELVETICA_18, data, 4, 3.5, 0);
    if((success + crashes) == 0)
        successPercent = 100.0;
    else
        successPercent = (float)success * 100.0 / (float)(success + crashes);
    sprintf(data, "Success: %.2f%%!", successPercent);
    drawString(GLUT_BITMAP_HELVETICA_18, data, -5, 3.5, 0);
    
    isColliding();

    
	glutSwapBuffers();
}


/**
 * The main driver for this program.
 *
 * Handle command-line arguments, initialize the ThinkGear connection,
 * and handle output.
 */
int main (int argc, char** argv) {
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
    
    // check whether the connection attempt was successful
    if(!retVal){
        fprintf(stderr, "connected.\n");
        
        // enable blink tracking
        TG_EnableBlinkDetection(connectionID, 1 );
        
        // enemy positions
        float enemySpace = (float)COORDINATE_WIDTH / ((float)ENEMY_AMOUNT);
        float offset = 0;
        for (int i = 0; i < ENEMY_AMOUNT; i++)
        {
            posX_enemy[i] = RESET_X + offset;
            offset = offset + enemySpace;
            collision_enemy[i] = false;
            posZ_enemy[i] = 0;
            posY_enemy[i] = placeEnemy();
        }
        
        // visuals
        glutStuff(argc, argv);
        
        
    }
    else {
        fprintf(stderr, "unable to connect. (%d)\n", retVal);
        exit(1);
    }
    
    return 0;
}

void glutStuff(int argc, char** argv)
{
    // init GLUT and create window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100,100);
	glutInitWindowSize(320,320);
	glutCreateWindow("Lighthouse3D- GLUT Tutorial");
    
    ///* extra
     
     //Initialize Projection Matrix
     glMatrixMode(GL_PROJECTION);
     glLoadIdentity();
     
     //Initialize Modelview Matrix
     glMatrixMode(GL_MODELVIEW);
     glLoadIdentity();
     
     //Initialize clear color
     glClearColor(0.f, 0.f, 0.f, 1.f);
     //*/
    
    
	// register callbacks
	glutDisplayFunc(renderScene);
	glutReshapeFunc(changeSize);
	glutIdleFunc(renderScene);
    
	// enter GLUT event processing cycle
	glutMainLoop();
    
}

void setEEGData()
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

