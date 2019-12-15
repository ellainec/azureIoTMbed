// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

//#define USE_MQTT

#include <stdlib.h>
#include "mbed.h"
#include "iothubtransporthttp.h"
#include "iothub_client_core_common.h"
#include "iothub_client_ll.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/agenttime.h"
#include "jsondecoder.h"
#include "button.hpp"
#include "BG96Interface.h"
#include "easy-connect.h"
#include "azure_message_helper.h"

#define IOT_AGENT_OK CODEFIRST_OK

#include "azure_certs.h"
                      

/* initialize the expansion board && sensors */

#include "XNucleoIKS01A2.h"
static HTS221Sensor   *hum_temp;
static LSM6DSLSensor  *acc_gyro;
static LPS22HBSensor  *pressure;

static const char* connectionString = "HostName=iotc-c522e121-b0fa-43a6-942f-4a32df173949.azure-devices.net;DeviceId=a43789a5-177c-45e1-aa0f-2f5782e48be3;SharedAccessKey=wx7Xi5OcwtBNlc1+EAE1CNVVsKgX0GQEqUcNfp/U2Aw=";

//device nucleoboard#2
//static const char* connectionString = "HostName=iotc-c522e121-b0fa-43a6-942f-4a32df173949.azure-devices.net;DeviceId=6293368e-960b-4207-807f-61a0d4e188f6;SharedAccessKey=YcJQb5SZglNpM08mdFlAHHItT32R3KBK7C8VQLS7Xl0=";
// to report F uncomment this #define CTOF(x)         (((double)(x)*9/5)+32)
#define CTOF(x)         (x)

Thread azure_client_thread(osPriorityNormal, 10*1024, NULL, "azure_client_thread");
static void azure_task(void);
BG96Interface* bg961;
//
// The mems sensor is setup to generate an interrupt with a tilt
// is detected at which time the blue LED is set to blink, also
// initialize all the ensors...
//

static int tilt_event;

// void mems_int1(void)
// {
//     tilt_event++;
// }

void mems_init(void)
{
    //acc_gyro->attach_int1_irq(&mems_int1);  // Attach callback to LSM6DSL INT1
    hum_temp->enable();                     // Enable HTS221 enviromental sensor
    pressure->enable();                     // Enable barametric pressure sensor
    acc_gyro->enable_x();                   // Enable LSM6DSL accelerometer
    //acc_gyro->enable_tilt_detection();      // Enable Tilt Detection

}

//
// The main routine simply prints a banner, initializes the system
// starts the worker threads and waits for a termination (join)

int main(void)
{
    printStartMessage();

    if (bg961->setPSMConfig(4)) {
        printf("psm config set\n");
    } else {
        printf("psm config NOT set\n");
    }
    if (platform_init() != 0) {
       printf("Error initializing the platform\r\n");
       return -1;
       }

    bg961 = (BG96Interface*) easy_get_netif(false);
//     char T3412[9] = "00000100";
//     char T3324[9] = "00000010";
//     if (bg961->psm(T3412, T3324)) {
//     	printf("psm set up successfully\n");
//     } else {
//     	printf("not successful!\n");
//    }
    char psmConfig[21];
    bg961->readPSMConfig(psmConfig);
    printf("psm config: %s \n", psmConfig);
//    printf("wait for first PSM ");
//    if(bg961->waitForPSM()) {
//    	printf("psm started\n");
//    } else {
//    	printf("psm not started\n");
//    }

    char psmSettings[21];
    bg961->readPSMSettings(psmSettings);
    printf("psm settings: %s\n", psmSettings);
    XNucleoIKS01A2 *mems_expansion_board = XNucleoIKS01A2::instance(I2C_SDA, I2C_SCL, D4, D5);
    hum_temp = mems_expansion_board->ht_sensor;
    acc_gyro = mems_expansion_board->acc_gyro;
    pressure = mems_expansion_board->pt_sensor;
    mems_init();
    azure_client_thread.start(azure_task);
    azure_client_thread.join();
    platform_deinit();
    printf(" - - - - - - - ALL DONE - - - - - - - \n");
    return 0;
}

// int g_message_count_send_confirmations;
//  static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
//  {
//      //userContextCallback;
//      // When a message is sent this callback will get envoked
//      g_message_count_send_confirmations++;
//      printf("Confirmation callback received for message %lu with result %s\r\n", (unsigned long)g_message_count_send_confirmations, ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
//  }

 void status_callback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback) {
	 printf("connection status reason: %s", ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason));
	 printf("connection status result: %s", ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS, result));
 }
void sendMessage(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, char* buffer, size_t size)
{
    printf("at send message \n");
    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray((const unsigned char*)buffer, size);
    if (messageHandle == NULL) {
        printf("unable to create a new IoTHubMessage\r\n");
        return;
        }
    if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, NULL, NULL) != IOTHUB_CLIENT_OK)
        printf("FAILED to send! [RSSI=%d]\n", platform_RSSI());
    else
        printf("OK. [RSSI=%d]\n",platform_RSSI());

    IoTHubMessage_Destroy(messageHandle);
}

void azure_task(void)
{
    //bool tilt_detection_enabled=true;
    float gtemp, ghumid, gpress;

    int  k;
    int  msg_sent=1;

    IoTDevice* iotDev = (IoTDevice*)malloc(sizeof(IoTDevice));
    if (iotDev == NULL) {
        printf("Failed to malloc space for IoTDevice\r\n");
        return;
    }
    // set C2D and device method callback
    // IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, receiveMessageCallback, NULL);

    //
    // setup the iotDev struction contents...
    //
    iotDev->ObjectName      = (char*)"Avnet NUCLEO-L496ZG+BG96 Azure IoT Client";
    iotDev->ObjectType      = (char*)"SensorData";
    iotDev->Version         = (char*)"version";
    iotDev->ReportingDevice = (char*)"testing";
    iotDev->TOD             = (char*)"";
    iotDev->Temperature     = 0.0;
    iotDev->mag1            = 0;
    iotDev->mag2            = 0;
    iotDev->mag3            = 0;
    iotDev->Humidity        = 0;
    iotDev->Pressure        = 0;
    iotDev->Tilt            = 0x2;
    iotDev->ButtonPress     = 0;
    while (true) {                
        Timer t;
        // printf("wait for psm... \n");
        // char psmBuffer[100];
        bool done = false;
        // if(bg961->waitForPSM()) {
        //     printf("psm started\n");
        // } else {
        //     printf("psm not started\n");
        // }
        t.start();
            /* Setup IoTHub client configuration */
        IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, HTTP_Protocol);

        if (iotHubClientHandle == NULL) {
            printf("Failed on IoTHubClient_Create\r\n");
            return;
        }

        // add the certificate information
        if (IoTHubClient_LL_SetOption(iotHubClientHandle, "TrustedCerts", certificates) != IOTHUB_CLIENT_OK)
            printf("failure to set option \"TrustedCerts\"\r\n");


        if (IoTHubClient_LL_SetOption(iotHubClientHandle, "product_info", "TELUSIOTKIT") != IOTHUB_CLIENT_OK)
            printf("failure to set option \"product_info\"\r\n");


        unsigned int minimumPollingTime = 9;
        if (IoTHubClient_LL_SetOption(iotHubClientHandle, "MinimumPollingTime", &minimumPollingTime) != IOTHUB_CLIENT_OK)
            printf("failure to set option \"MinimumPollingTime\"\r\n");


        if (IoTHubClientCore_LL_SetRetryPolicy(iotHubClientHandle, IOTHUB_CLIENT_RETRY_NONE, 1) != IOTHUB_CLIENT_OK){
            printf("failure to set retry option\n");
        }

        if (IoTHubClient_LL_SetConnectionStatusCallback(iotHubClientHandle, status_callback, NULL) != IOTHUB_CLIENT_OK) {
            printf("failure to set connection status callback!");
        }

        
        char*  msg;
        size_t msgSize;

        hum_temp->get_temperature(&gtemp);           // get Temp
        hum_temp->get_humidity(&ghumid);             // get Humidity
        pressure->get_pressure(&gpress);             // get pressure


        iotDev->Temperature = CTOF(gtemp);
        iotDev->Humidity    = (int)ghumid;
        iotDev->Pressure    = (int)gpress;

        // if( tilt_event ) {
        //     tilt_event = 0;
        //     iotDev->Tilt |= 1;
        // }

        printf("(%04d)",msg_sent++);
        msg = makeMessage(iotDev);
        msgSize = strlen(msg);
        sendMessage(iotHubClientHandle, msg, msgSize);
        free(msg);
        iotDev->Tilt &= 0x2;

        /* schedule IoTHubClient to send events/receive commands */

        //IoTHubClient_LL_DoWork(iotHubClientHandle);
        IOTHUB_CLIENT_STATUS status;
        printf("before doWOrk \n");
         while ((IoTHubClient_LL_GetSendStatus(iotHubClientHandle, &status) == IOTHUB_CLIENT_OK) && (status == IOTHUB_CLIENT_SEND_STATUS_BUSY))
         {
             printf("at do work");
             IoTHubClient_LL_DoWork(iotHubClientHandle);
             ThisThread::sleep_for(100); // @suppress("Invalid arguments")
         }

// @suppress("Function cannot be resolved")
            IoTHubClient_LL_Destroy(iotHubClientHandle);
            t.stop();
            printf("The time taken was %f seconds\n", t.read());
            ThisThread::sleep_for(120000);  //in msec // @suppress("Function cannot be resolved")
        }
    free(iotDev);
    return;
}

