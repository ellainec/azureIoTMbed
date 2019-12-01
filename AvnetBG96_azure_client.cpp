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
<<<<<<< Updated upstream
#include "bg96gps.hpp"
#include "azure_message_helper.h"
=======
#include "button.hpp"
#include "BG96Interface.h"
#include "easy-connect.h"
>>>>>>> Stashed changes

#define IOT_AGENT_OK CODEFIRST_OK

#include "azure_certs.h"
                      

/* initialize the expansion board && sensors */

#include "XNucleoIKS01A2.h"
static HTS221Sensor   *hum_temp;
static LSM6DSLSensor  *acc_gyro;
static LPS22HBSensor  *pressure;
<<<<<<< Updated upstream
=======
static LSM303AGRMagSensor *mag;

//device 1
//static const char* connectionString = "HostName=iotc-c522e121-b0fa-43a6-942f-4a32df173949.azure-devices.net;DeviceId=35f3adb7-d7f6-4efb-9da3-b1db552c44a7;SharedAccessKey=bCwmvrv+hOHJn7iYFzpnPadK5PEbRslmpY6EEWDEDSI=";

//device 2
static const char* connectionString = "HostName=iotc-c522e121-b0fa-43a6-942f-4a32df173949.azure-devices.net;DeviceId=837a1bc1-ce82-41d0-b5b3-9327f345faf8;SharedAccessKey=sbNeGs4Z423GR3WFgtQfBeV7PnDithMOqGPU3K1CtIY=";

//device 3
//static const char* connectionString = "HostName=iotc-c522e121-b0fa-43a6-942f-4a32df173949.azure-devices.net;DeviceId=087ceb74-be24-4b4a-971c-afab67987a11;SharedAccessKey=wGW4NiY5cJilX5o3BWe1RiLN3wyqhj2La5ccSsWK4M0=";
>>>>>>> Stashed changes


static const char* connectionString = "HostName=iotc-c522e121-b0fa-43a6-942f-4a32df173949.azure-devices.net;DeviceId=a43789a5-177c-45e1-aa0f-2f5782e48be3;SharedAccessKey=wx7Xi5OcwtBNlc1+EAE1CNVVsKgX0GQEqUcNfp/U2Aw=";

//device nucleoboard#2
//static const char* connectionString = "HostName=iotc-c522e121-b0fa-43a6-942f-4a32df173949.azure-devices.net;DeviceId=6293368e-960b-4207-807f-61a0d4e188f6;SharedAccessKey=YcJQb5SZglNpM08mdFlAHHItT32R3KBK7C8VQLS7Xl0=";
// to report F uncomment this #define CTOF(x)         (((double)(x)*9/5)+32)
#define CTOF(x)         (x)

Thread azure_client_thread(osPriorityNormal, 10*1024, NULL, "azure_client_thread");
static void azure_task(void);
<<<<<<< Updated upstream
EventFlags deleteOK;
size_t g_message_count_send_confirmations;

/* create the GPS elements for example program */
BG96Interface* bg96Interface;

//static int tilt_event;
=======
BG96Interface* bg961;
//
// The mems sensor is setup to generate an interrupt with a tilt
// is detected at which time the blue LED is set to blink, also
// initialize all the ensors...
//

static int tilt_event;
>>>>>>> Stashed changes

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
<<<<<<< Updated upstream
     if (platform_init() != 0) {
         printf("Error initializing the platform\r\n");
        return 0;
    }
    //printStartMessage();
=======
    printf("\r\n");
    printf("     ****\r\n");
    printf("    **  **     Azure IoTClient Example, version %s\r\n", APP_VERSION);
    printf("   **    **    by AVNET\r\n");
    printf("  ** ==== **   \r\n");
    printf("\r\n");
    printf("The example program interacts with Azure IoTHub sending \r\n");
    printf("sensor data and receiving messeages (using ARM Mbed OS v5.x)\r\n");
    printf("->using %s Environmental Sensor\r\n", ENV_SENSOR);
    #ifdef IOTHUBTRANSPORTHTTP_H
        printf("->using HTTPS Transport Protocol\r\n");
    #else
        printf("->using MQTT Transport Protocol\r\n");
    #endif
    printf("\r\n");

    if (platform_init() != 0) {
       printf("Error initializing the platform\r\n");
       return -1;
       }

    bg961 = (BG96Interface*) easy_get_netif(false);
    char response2[100];
//     if (bg961->test()) {
//     	printf("psm set up successfully\n");
//     } else {
//     	printf("not successful!\n");
//    }

    bg961->test2(response2);
    printf("test2 response %s \n", response2);
   if (bg961->psmEnterIndication()) {
   	printf("indication set\n");
   } else {
   	printf("indication not set \n");
   }

   printf("wait for first PSM ");
   if(bg961->waitForPSM()) {
   	printf("psm started\n");
   } else {
   	printf("psm not started\n");
   }
//    printf("deinit platform\n");
//    platform_deinit();
//    printf("init platform\n");
//    if (platform_init() != 0) {
//       printf("Error initializing the platform\r\n");
//       return -1;
//     }
>>>>>>> Stashed changes
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

static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    deleteOK.set(0x1);
}
<<<<<<< Updated upstream

=======
int g_message_count_send_confirmations;
 static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
 {
     //userContextCallback;
     // When a message is sent this callback will get envoked
     g_message_count_send_confirmations++;
     printf("Confirmation callback received for message %lu with result %s\r\n", (unsigned long)g_message_count_send_confirmations, ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
 }

 void status_callback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback) {
	 printf("connection status reason: %s", ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason));
	 printf("connection status result: %s", ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS, result));
 }
>>>>>>> Stashed changes
void sendMessage(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, char* buffer, size_t size)
{
    printf("at send message \n");
    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray((const unsigned char*)buffer, size);
    if (messageHandle == NULL) {
        printf("unable to create a new IoTHubMessage\r\n");
        return;
        }
    if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, send_confirm_callback, NULL) != IOTHUB_CLIENT_OK)
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
    int psmSet = 0;

<<<<<<< Updated upstream

    while (true) {
        printf("at start of while\n");
        /* Setup IoTHub client configuration */
=======
    // set C2D and device method callback
    // IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, receiveMessageCallback, NULL);
    IoTDevice* iotDev = (IoTDevice*)malloc(sizeof(IoTDevice));
    if (iotDev == NULL) {
        printf("Failed to malloc space for IoTDevice\r\n");
        return;
    }
    //
    // setup the iotDev struction contents...
    //
    iotDev->ObjectName      = (char*)"Avnet NUCLEO-L496ZG+BG96 Azure IoT Client";
    iotDev->ObjectType      = (char*)"SensorData";
    iotDev->Version         = (char*)APP_VERSION;
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
    while (runTest) {                
        Timer t;
        printf("wait for psm... \n");
        char psmBuffer[100];
        bool done = false;
        if(bg961->waitForPSM()) {
            printf("psm started\n");
        } else {
            printf("psm not started\n");
        }
        t.start();
            /* Setup IoTHub client configuration */
>>>>>>> Stashed changes
        IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, HTTP_Protocol);

        if (iotHubClientHandle == NULL) {
            printf("Failed on IoTHubClient_Create\r\n");
            return;
<<<<<<< Updated upstream
            }
=======
        }
>>>>>>> Stashed changes

        // add the certificate information
        if (IoTHubClient_LL_SetOption(iotHubClientHandle, "TrustedCerts", certificates) != IOTHUB_CLIENT_OK)
            printf("failure to set option \"TrustedCerts\"\r\n");

<<<<<<< Updated upstream
        #if MBED_CONF_APP_TELUSKIT == 1
            if (IoTHubClient_LL_SetOption(iotHubClientHandle, "product_info", "TELUSIOTKIT") != IOTHUB_CLIENT_OK)
                printf("failure to set option \"product_info\"\r\n");
        #endif

        // polls will happen effectively at ~10 seconds.  The default value of minimumPollingTime is 25 minutes. 
        // For more information, see:
        //     https://azure.microsoft.com/documentation/articles/iot-hub-devguide/#messaging

        unsigned int minimumPollingTime = 9;
        if (IoTHubClient_LL_SetOption(iotHubClientHandle, "MinimumPollingTime", &minimumPollingTime) != IOTHUB_CLIENT_OK)
                printf("failure to set option \"MinimumPollingTime\"\r\n");

        IoTDevice* iotDev = (IoTDevice*)malloc(sizeof(IoTDevice));
        if (iotDev == NULL) {
            return;
        }
        setUpIotStruct(iotDev);
        // mbed_stats_cpu_t stats;
        // mbed_stats_cpu_get(&stats);
        // printf("Uptime: %llu ", stats.uptime / 1000);
        // printf("Sleep time: %llu ", stats.sleep_time / 1000);
        // printf("Deep Sleep: %llu\n", stats.deep_sleep_time / 1000);

=======

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

        
>>>>>>> Stashed changes
        char*  msg;
        size_t msgSize;

        // gps.gpsLocation(&gdata);
        // iotDev->lat = gdata.lat;
        // iotDev->lon = gdata.lon;
        // iotDev->gpstime = gdata.utc;
        // memcpy(iotDev->gpsdate, gdata.date, 7);


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
<<<<<<< Updated upstream
        IOTHUB_CLIENT_STATUS status;
        while ((IoTHubClient_LL_GetSendStatus(iotHubClientHandle, &status) == IOTHUB_CLIENT_OK) && (status == IOTHUB_CLIENT_SEND_STATUS_BUSY))
        {
            printf("at dowork\n");
            IoTHubClient_LL_DoWork(iotHubClientHandle);
            ThisThread::sleep_for(100); 
        }
        deleteOK.wait_all(0x1);
        free(iotDev);
        IoTHubClient_LL_Destroy(iotHubClientHandle);
        bg96Interface->psm();
        ThisThread::sleep_for(10000);
        bg96Interface->wakeUp();
    }
=======

        //IoTHubClient_LL_DoWork(iotHubClientHandle);
        IOTHUB_CLIENT_STATUS status;
        printf("before doWOrk \n");
         while ((IoTHubClient_LL_GetSendStatus(iotHubClientHandle, &status) == IOTHUB_CLIENT_OK) && (status == IOTHUB_CLIENT_SEND_STATUS_BUSY))
         {
             printf("at do work");
             IoTHubClient_LL_DoWork(iotHubClientHandle);
             ThisThread::sleep_for(100); // @suppress("Invalid arguments")
         }

#if defined(MBED_HEAP_STATS_ENABLED)
        mbed_stats_heap_t heap_stats; //jmf

        mbed_stats_heap_get(&heap_stats);
        printf("  Current heap: %lu\r\n", heap_stats.current_size);
        printf(" Max heap size: %lu\r\n", heap_stats.max_size);
        printf("     alloc_cnt:	%lu\r\n", heap_stats.alloc_cnt);
        printf("alloc_fail_cnt:	%lu\r\n", heap_stats.alloc_fail_cnt);
        printf("    total_size:	%lu\r\n", heap_stats.total_size);
        printf(" reserved_size:	%lu\r\n", heap_stats.reserved_size);
#endif

#if defined(MBED_STACK_STATS_ENABLED)
        int cnt_ss = osThreadGetCount();
        mbed_stats_stack_t *stats_ss = (mbed_stats_stack_t*) malloc(cnt_ss * sizeof(mbed_stats_stack_t));

        cnt_ss = mbed_stats_stack_get_each(stats_ss, cnt_ss);
        for (int i = 0; i < cnt_ss; i++)
            printf("Thread: 0x%lX, Stack size: %lu, Max stack: %lu\r\n", stats_ss[i].thread_id, stats_ss[i].reserved_size, stats_ss[i].max_size);
#endif

#if defined(MBED_THREAD_STATS_ENABLED)
#define MAX_THREAD_STATS  10
            mbed_stats_thread_t *stats = new mbed_stats_thread_t[MAX_THREAD_STATS];
            int count = mbed_stats_thread_get_each(stats, MAX_THREAD_STATS);

            for(int i = 0; i < count; i++) {
                printf("ID: 0x%lx \n", stats[i].id);
                printf("Name: %s \n", stats[i].name);
                printf("State: %ld \n", stats[i].state);
                printf("Priority: %ld \n", stats[i].priority);
                printf("Stack Size: %ld \n", stats[i].stack_size);
                printf("Stack Space: %ld \n", stats[i].stack_space);
                printf("\n");
                }
#endif
// @suppress("Function cannot be resolved")
            IoTHubClient_LL_Destroy(iotHubClientHandle);
            t.stop();
            printf("The time taken was %f seconds\n", t.read());
            //ThisThread::sleep_for(80000);  //in msec // @suppress("Function cannot be resolved")
        }
    free(iotDev);
    //IoTHubClient_LL_Destroy(iotHubClientHandle);
>>>>>>> Stashed changes
    return;
}

