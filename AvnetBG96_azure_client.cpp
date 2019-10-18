// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

//#define USE_MQTT

#include <stdlib.h>
#include "mbed.h"

#include "jsondecoder.h"

#include "button.hpp"

#define APP_VERSION "1.2"
#define IOT_AGENT_OK CODEFIRST_OK
                     

// to report F uncomment this #define CTOF(x)         (((double)(x)*9/5)+32)
#define CTOF(x)         (x)


Thread LED_thread(osPriorityNormal, 256, NULL, "LED_thread");
static void LED_task(void);

/* LED Management */
DigitalOut   RED_led(LED1);
DigitalOut   BLUE_led(LED2);
DigitalOut   GREEN_led(LED3);

const int    blink_interval = 500; //msec
int          RED_state, BLUE_state, GREEN_state;

#define GREEN       4  //0 0100 GREEN
#define BLUE        2  //0 0010
#define RED         1  //0 0001 RED

#define LED_ON      8  //0 1xxx
#define LED_BLINK  16  //1 xxxx
#define LED_OFF     0  //0 0xxx

#define SET_LED(l,s) (l##_led = ((l##_state=s)&LED_ON)? 1: 0)

//
// The LED thread simply manages the LED's on an on-going basis
//
static void LED_task(void)
{
    while (true) {
        if( GREEN_state & LED_OFF ) 
            GREEN_led = 0;
        else if( GREEN_state & LED_ON ) 
            GREEN_led = 1;
        else if( GREEN_state & LED_BLINK ) 
            GREEN_led = !GREEN_led;

        if( BLUE_state & LED_OFF ) 
            BLUE_led = 0;
        else if( BLUE_state & LED_ON ) 
            BLUE_led = 1;
        else if( BLUE_state & LED_BLINK ) 
            BLUE_led = !BLUE_led;

        if( RED_state & LED_OFF ) 
            RED_led = 0;
        else if( RED_state & LED_ON ) 
            RED_led = 1;
        else if( RED_state & LED_BLINK ) 
            RED_led = !RED_led;

        ThisThread::sleep_for(blink_interval);  //in msec
        }
}


/* Button callbacks for a press and release (light an LED) */
static bool button_pressed = false;
void ub_press(void)
{
    button_pressed = true;
    SET_LED(RED,LED_ON);
}

void ub_release(int x)
{
    button_pressed = false;
    SET_LED(RED,LED_OFF);
}



//
// The main routine simply prints a banner, initializes the system
// starts the worker threads and waits for a termination (join)

int main(void)
{
    printf("\r\n");
    printf("     ****\r\n");
    printf("    **  **     Azure IoTClient Example, version %s\r\n");
    printf("   **    **    by AVNET\r\n");
    printf("  ** ==== **   \r\n");
    printf("\r\n");
    printf("The example program interacts with Azure IoTHub sending \r\n");
    printf("sensor data and receiving messeages (using ARM Mbed OS v5.x)\r\n");
    printf("->using %s Environmental Sensor\r\n");

    LED_thread.start(LED_task);
    LED_thread.join();

    printf(" - - - - - - - ALL DONE - - - - - - - \n");
    return 0;
}