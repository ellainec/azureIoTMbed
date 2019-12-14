// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

//#define USE_MQTT

#include <stdlib.h>
#include "mbed.h"
#ifdef USE_MQTT
#include "iothubtransportmqtt.h"
#else
#include "iothubtransporthttp.h"
#endif
#include "iothub_client_core_common.h"
#include "iothub_client_ll.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/agenttime.h"
#include "jsondecoder.h"
#include "button.hpp"

#define APP_VERSION "1.2"
#define IOT_AGENT_OK CODEFIRST_OK

#include "azure_certs.h"

/* The following is the message we will be sending to Azure */
typedef struct IoTDevice_t {
    char* ObjectName;
    char* ObjectType;
    char* Version;
    char* ReportingDevice;
    int mag1;
    int mag2;
    int mag3;
    float Temperature;
    int   Humidity;
    int   Pressure;
    int   Tilt;
    int   ButtonPress;
    char* TOD;
    } IoTDevice;

#define IOTDEVICE_MSG_FORMAT       \
   "{"                             \
     "\"ObjectName\":\"%s\","      \
     "\"ObjectType\":\"%s\","      \
     "\"Version\":\"%s\","         \
     "\"ReportingDevice\":\"%s\"," \
     "\"mag1\":\"%d\","            \
     "\"mag2\":\"%d\","            \
     "\"mag3\":\"%d\","            \
     "\"Temperature\":\"%.02f\","  \
     "\"Humidity\":\"%d\","        \
     "\"Pressure\":\"%d\","        \
     "\"Tilt\":\"%d\","            \
     "\"ButtonPress\":\"%d\","     \
     "\"TOD\":\"%s UTC\""          \
   "}"

/* initialize the expansion board && sensors */

#define ENV_SENSOR "IKS01A2"
#include "XNucleoIKS01A2.h"
static HTS221Sensor   *hum_temp;
static LSM6DSLSensor  *acc_gyro;
static LPS22HBSensor  *pressure;
static LSM303AGRMagSensor *mag;

//device 1
//static const char* connectionString = "HostName=iotc-c522e121-b0fa-43a6-942f-4a32df173949.azure-devices.net;DeviceId=35f3adb7-d7f6-4efb-9da3-b1db552c44a7;SharedAccessKey=bCwmvrv+hOHJn7iYFzpnPadK5PEbRslmpY6EEWDEDSI=";

static char* deviceName = "LTE-M 3";
//device 2
//static const char* connectionString = "HostName=iotc-c522e121-b0fa-43a6-942f-4a32df173949.azure-devices.net;DeviceId=837a1bc1-ce82-41d0-b5b3-9327f345faf8;SharedAccessKey=sbNeGs4Z423GR3WFgtQfBeV7PnDithMOqGPU3K1CtIY=";

//device 3
//static const char* connectionString = "HostName=iotc-c522e121-b0fa-43a6-942f-4a32df173949.azure-devices.net;DeviceId=087ceb74-be24-4b4a-971c-afab67987a11;SharedAccessKey=wGW4NiY5cJilX5o3BWe1RiLN3wyqhj2La5ccSsWK4M0=";

//device 4
//static const char* connectionString = "HostName=iotc-c522e121-b0fa-43a6-942f-4a32df173949.azure-devices.net;DeviceId=a43789a5-177c-45e1-aa0f-2f5782e48be3;SharedAccessKey=wx7Xi5OcwtBNlc1+EAE1CNVVsKgX0GQEqUcNfp/U2Aw=";

//device 5
//static const char* connectionString = "HostName=iotc-c522e121-b0fa-43a6-942f-4a32df173949.azure-devices.net;DeviceId=92943ebe-deb0-40a7-9cd9-f039beb120ba;SharedAccessKey=YpBVkVQ+VLTtcROOrKbStjwl6tDDYdcQJEZZhETWQ3w=";

//lte 1
//static const char* connectionString = "HostName=iotc-c522e121-b0fa-43a6-942f-4a32df173949.azure-devices.net;DeviceId=7c6ede62-c63a-409f-9179-f08223000872;SharedAccessKey=K8jecqvN5Tz7TN8yGyPPNU4eLkr50Itn26fH8kw52pc=";

//lte 2
//static const char* connectionString = "HostName=iotc-c522e121-b0fa-43a6-942f-4a32df173949.azure-devices.net;DeviceId=fd044ec6-b886-48d2-baf8-ab07b807afa1;SharedAccessKey=QpOELRF1o/rNWdOzMYTyDE7QIrK7uANCxWjxW/GFaho=";

//lte 3
static const char* connectionString = "HostName=iotc-c522e121-b0fa-43a6-942f-4a32df173949.azure-devices.net;DeviceId=2457a99b-6ebd-4e8c-945f-6734e46331ca;SharedAccessKey=ZW21oZyo2WzFAxdFTVPsYIT9iVVXxtqpep8xzQ3s48g=";

// to report F uncomment this #define CTOF(x)         (((double)(x)*9/5)+32)
#define CTOF(x)         (x)

Thread azure_client_thread(osPriorityNormal, 8*1024, NULL, "azure_client_thread"); // @suppress("Type cannot be resolved")
static void azure_task(void);


//
// The mems sensor is setup to generate an interrupt with a tilt
// is detected at which time the blue LED is set to blink, also
// initialize all the ensors...
//

static int tilt_event;

void mems_int1(void)
{
    tilt_event++;
}

void mems_init(void)
{
    acc_gyro->attach_int1_irq(&mems_int1);  // Attach callback to LSM6DSL INT1
    hum_temp->enable();                     // Enable HTS221 enviromental sensor
    pressure->enable();                     // Enable barametric pressure sensor
    acc_gyro->enable_x();                   // Enable LSM6DSL accelerometer
    acc_gyro->enable_tilt_detection();      // Enable Tilt Detection
    mag->enable();

}

//
// The main routine simply prints a banner, initializes the system
// starts the worker threads and waits for a termination (join)

int main(void)
{
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


    XNucleoIKS01A2 *mems_expansion_board = XNucleoIKS01A2::instance(I2C_SDA, I2C_SCL, D4, D5);
    hum_temp = mems_expansion_board->ht_sensor;
    acc_gyro = mems_expansion_board->acc_gyro;
    pressure = mems_expansion_board->pt_sensor;
    mag      = mems_expansion_board->magnetometer;

    mems_init();
    azure_client_thread.start(azure_task);

    azure_client_thread.join();
    platform_deinit();
    printf(" - - - - - - - ALL DONE - - - - - - - \n");
    return 0;
}

//
// This function sends the actual message to azure
//
//
// *************************************************************
//  AZURE STUFF...

char* makeMessage(IoTDevice* iotDev)
{
    static char buffer[80];
    const int   msg_size = 512;
    char*       ptr      = (char*)malloc(msg_size);
    time_t      rawtime;
    struct tm   *ptm;

    time(&rawtime);
    ptm = gmtime(&rawtime);
    strftime(buffer,80,"%a %F %X",ptm);
    iotDev->TOD = buffer;
    int c = (strstr(buffer,":")-buffer) - 2;
    printf("Send IoTHubClient Message@%s - ",&buffer[c]);
    snprintf(ptr, msg_size, IOTDEVICE_MSG_FORMAT,
                            iotDev->ObjectName,
                            iotDev->ObjectType,
                            iotDev->Version,
                            iotDev->ReportingDevice,
                            iotDev->mag1,
                            iotDev->mag2,
                            iotDev->mag3,
                            iotDev->Temperature,
                            iotDev->Humidity,
                            iotDev->Pressure,
                            iotDev->Tilt,
                            iotDev->ButtonPress,
                            iotDev->TOD);
    return ptr;
}

// static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
// {
//     //userContextCallback;
//     // When a message is sent this callback will get envoked
//     g_message_count_send_confirmations++;
//     printf("Confirmation callback received for message %lu with result %s\r\n", (unsigned long)g_message_count_send_confirmations, ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
// }

void sendMessage(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, char* buffer, size_t size)
{
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

IOTHUBMESSAGE_DISPOSITION_RESULT receiveMessageCallback(
    IOTHUB_MESSAGE_HANDLE message,
    void *userContextCallback)
{
    const unsigned char *buffer = NULL;
    size_t size = 0;

    if (IOTHUB_MESSAGE_OK != IoTHubMessage_GetByteArray(message, &buffer, &size))
    {
        return IOTHUBMESSAGE_ABANDONED;
    }

    // message needs to be converted to zero terminated string
    char * temp = (char *)malloc(size + 1);
    if (temp == NULL)
    {
        return IOTHUBMESSAGE_ABANDONED;
    }
    strncpy(temp, (char*)buffer, size);
    temp[size] = '\0';

    printf("Receiving message: '%s'\r\n", temp);
    if( !strcmp(temp,"led-blink") ) {
        printf("start blinking\n");
        }
    if( !strcmp(temp,"led-on") ) {
        printf("turn on\n");
        }
    if( !strcmp(temp,"led-off") ) {
        printf("turn off\n");
        }

    free(temp);

    return IOTHUBMESSAGE_ACCEPTED;
}


void azure_task(void)
{
    bool button_press = false, runTest = true;
    bool tilt_detection_enabled=true;
    float gtemp, ghumid, gpress;

    int  k;
    int  msg_sent=1;


    /* Setup IoTHub client configuration */
#ifdef IOTHUBTRANSPORTHTTP_H
    IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, HTTP_Protocol);
#else
    IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, MQTT_Protocol);
#endif

    if (iotHubClientHandle == NULL) {
        printf("Failed on IoTHubClient_Create\r\n");
        return;
        }

    // add the certificate information
    if (IoTHubClient_LL_SetOption(iotHubClientHandle, "TrustedCerts", certificates) != IOTHUB_CLIENT_OK)
        printf("failure to set option \"TrustedCerts\"\r\n");

#if MBED_CONF_APP_TELUSKIT == 1
    if (IoTHubClient_LL_SetOption(iotHubClientHandle, "product_info", "TELUSIOTKIT") != IOTHUB_CLIENT_OK)
        printf("failure to set option \"product_info\"\r\n");
#endif

#ifdef IOTHUBTRANSPORTHTTP_H
    // polls will happen effectively at ~10 seconds.  The default value of minimumPollingTime is 25 minutes.
    // For more information, see:
    //     https://azure.microsoft.com/documentation/articles/iot-hub-devguide/#messaging

    unsigned int minimumPollingTime = 9;
    if (IoTHubClient_LL_SetOption(iotHubClientHandle, "MinimumPollingTime", &minimumPollingTime) != IOTHUB_CLIENT_OK)
        printf("failure to set option \"MinimumPollingTime\"\r\n");
#endif

    if (IoTHubClientCore_LL_SetRetryPolicy(iotHubClientHandle, IOTHUB_CLIENT_RETRY_NONE, 1) != IOTHUB_CLIENT_OK){
        printf("failure to set retry option\n");
    }
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
    iotDev->Version         = (char*)APP_VERSION;
    iotDev->ReportingDevice = deviceName;
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
        char*  msg;
        size_t msgSize;
        int32_t magData[3];

        hum_temp->get_temperature(&gtemp);           // get Temp
        hum_temp->get_humidity(&ghumid);             // get Humidity
        pressure->get_pressure(&gpress);             // get pressure
        mag->get_m_axes(magData);

        iotDev->Temperature = CTOF(gtemp);
        iotDev->Humidity    = (int)ghumid;
        iotDev->Pressure    = (int)gpress;
        iotDev->mag1        = (int)magData[0];
        iotDev->mag2        = (int)magData[1];
        iotDev->mag3        = (int)magData[2];

        if( tilt_event ) {
            tilt_event = 0;
            iotDev->Tilt |= 1;
        }

        printf("(%04d)",msg_sent++);
        msg = makeMessage(iotDev);
        msgSize = strlen(msg);
        sendMessage(iotHubClientHandle, msg, msgSize);
        free(msg);
        iotDev->Tilt &= 0x2;
        iotDev->ButtonPress = 0;

        /* schedule IoTHubClient to send events/receive commands */

        IoTHubClient_LL_DoWork(iotHubClientHandle);

        // while ((IoTHubClient_LL_GetSendStatus(iotHubClientHandle, &status) == IOTHUB_CLIENT_OK) && (status == IOTHUB_CLIENT_SEND_STATUS_BUSY))
        // {
        //     IoTHubClient_LL_DoWork(iotHubClientHandle);
        //     ThreadAPI_Sleep(100); // @suppress("Invalid arguments")
        // }

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
        ThisThread::sleep_for(30000);  //in msec // @suppress("Function cannot be resolved")

        }
    free(iotDev);
    IoTHubClient_LL_Destroy(iotHubClientHandle);
    return;
}

