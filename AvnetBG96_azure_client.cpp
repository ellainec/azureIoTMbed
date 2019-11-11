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
#include "bg96gps.hpp"
#include "azure_message_helper.h"

#define IOT_AGENT_OK CODEFIRST_OK

#include "azure_certs.h"
                      

/* initialize the expansion board && sensors */

#include "XNucleoIKS01A2.h"
static HTS221Sensor   *hum_temp;
static LSM6DSLSensor  *acc_gyro;
static LPS22HBSensor  *pressure;


static const char* connectionString = "HostName=iotc-c522e121-b0fa-43a6-942f-4a32df173949.azure-devices.net;DeviceId=35f3adb7-d7f6-4efb-9da3-b1db552c44a7;SharedAccessKey=bCwmvrv+hOHJn7iYFzpnPadK5PEbRslmpY6EEWDEDSI=";

// to report F uncomment this #define CTOF(x)         (((double)(x)*9/5)+32)
#define CTOF(x)         (x)

Thread azure_client_thread(osPriorityNormal, 8*1024, NULL, "azure_client_thread");
Thread power_down_thread(osPriorityNormal, 8*1024, NULL, "power_down_thread");
Thread power_up_thread(osPriorityNormal, 8*1024, NULL, "power_up_thread");
static void azure_task(void);
EventFlags powerUpFlag;
EventFlags connectedFlag;
EventFlags sendMessageFlag;
EventFlags powerDownFlag;
EventFlags powerDownCompleteFlag;
EventFlags messageSentFlag;
EventFlags deleteOK;
size_t g_message_count_send_confirmations;

/* create the GPS elements for example program */
gps_data gdata; 
bg96_gps gps;   
BG96Interface* bg96Interface;
bool connected = false;

//
// The mems sensor is setup to generate an interrupt with a tilt 
// is detected at which time the blue LED is set to blink, also
// initialize all the ensors...
//

//static int tilt_event;

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

void powerUp(void) {
    while(true){
        powerUpFlag.wait_all(0x1);
        printf("POWERING ON \n");
        if (platform_init() != 0) {
            printf("Error initializing the platform\r\n");
            connectedFlag.set(0x1);
            return;
        }
        connected = true;
        bg96Interface = (BG96Interface*) easy_get_netif(true);
        printf("[ start GPS ] ");
        //gps.gpsPower(true);
        printf("Successful.\r\n[get GPS loc] ");
        fflush(stdout);
        //gps.gpsLocation(&gdata);
        //printf("Latitude = %6.3f Longitude = %6.3f date=%s, time=%6.0f\n\n",gdata.lat,gdata.lon,gdata.date,gdata.utc);
        connectedFlag.set(0x1);
    }
}

void BG96_Modem_PowerON(void)
{
    DigitalOut BG96_RESET(D7);
    DigitalOut BG96_PWRKEY(D10);
    DigitalOut BG97_WAKE(D11);

    BG96_RESET = 0;
    BG96_PWRKEY = 0;
    BG97_WAKE = 0;
    wait_ms(300);

    BG96_RESET = 1;
    BG97_WAKE = 1;
    BG96_PWRKEY = 1;
    wait_ms(400);

    BG96_RESET = 0;
    wait_ms(10);
}

void BG96_Modem_PowerOFF(void)
{
    DigitalOut BG96_RESET(D7);
    DigitalOut BG96_PWRKEY(D10);
    DigitalOut BG97_WAKE(D11);

    BG96_RESET = 0;
    BG96_PWRKEY = 0;
    BG97_WAKE = 0;
    wait_ms(300);
}

void powerDown(){
    while(true){
        powerDownFlag.wait_all(0x1);
        printf("POWERING OFF\n");
        nsapi_error_t ret;
        ret = bg96Interface->disconnect();
        if (ret == NSAPI_ERROR_OK) {
            printf("disconnected succesfully\n");
        } else if( ret == NSAPI_ERROR_DEVICE_ERROR) {
            printf("disconnect not successful\n");
        } else {
            printf("dunno what I got...\n");
        }
        connected = false;
        BG96_Modem_PowerOFF();
        printf("DONE POWERING OFF\n");
        powerDownCompleteFlag.set(0x1);
    }
}

//
// The main routine simply prints a banner, initializes the system
// starts the worker threads and waits for a termination (join)

int main(void)
{
    printStartMessage();
    XNucleoIKS01A2 *mems_expansion_board = XNucleoIKS01A2::instance(I2C_SDA, I2C_SCL, D4, D5);
    hum_temp = mems_expansion_board->ht_sensor;
    acc_gyro = mems_expansion_board->acc_gyro;
    pressure = mems_expansion_board->pt_sensor;
    power_up_thread.start(powerUp);
    azure_client_thread.start(azure_task);
    power_down_thread.start(powerDown);
    mems_init();
    while (true) {
        printf("hello thereeee \n");
        powerUpFlag.set(0x1);
        connectedFlag.wait_all(0x1);
        if (connected) {
            sendMessageFlag.set(0x1);
            messageSentFlag.wait_all(0x1);
            powerDownFlag.set(0x1);
            powerDownCompleteFlag.wait_all(0x1);
        } 
        ThisThread::sleep_for(180000);
    }
    platform_deinit();
    printf(" - - - - - - - ALL DONE - - - - - - - \n");
    return 0;
}

static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    //userContextCallback;
    // When a message is sent this callback will get envoked
    g_message_count_send_confirmations++;
    printf("Confirmation callback received for message %lu with result %s\r\n", (unsigned long)g_message_count_send_confirmations, ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
    deleteOK.set(0x1);
}

void sendMessage(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, char* buffer, size_t size)
{
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
    bool runTest = true;
    //bool tilt_detection_enabled=true;
    float gtemp, ghumid, gpress;

    int  k;
    int  msg_sent=1;


    while (true) {
        sendMessageFlag.wait_all(0x1);

        /* Setup IoTHub client configuration */
        IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, HTTP_Protocol);

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

        // polls will happen effectively at ~10 seconds.  The default value of minimumPollingTime is 25 minutes. 
        // For more information, see:
        //     https://azure.microsoft.com/documentation/articles/iot-hub-devguide/#messaging

        unsigned int minimumPollingTime = 9;
        if (IoTHubClient_LL_SetOption(iotHubClientHandle, "MinimumPollingTime", &minimumPollingTime) != IOTHUB_CLIENT_OK)
                printf("failure to set option \"MinimumPollingTime\"\r\n");

        IoTDevice* iotDev = new IoTDevice{};
        // IoTDevice* iotDev = (IoTDevice*)malloc(sizeof(IoTDevice));
        if (iotDev == NULL) {
            printf("Failed to malloc space for IoTDevice\r\n");
            return;
        }
        setUpIotStruct(iotDev);
        mbed_stats_cpu_t stats;
        mbed_stats_cpu_get(&stats);
        printf("Uptime: %llu ", stats.uptime / 1000);
        printf("Sleep time: %llu ", stats.sleep_time / 1000);
        printf("Deep Sleep: %llu\n", stats.deep_sleep_time / 1000);

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
        IOTHUB_CLIENT_STATUS status;
        while ((IoTHubClient_LL_GetSendStatus(iotHubClientHandle, &status) == IOTHUB_CLIENT_OK) && (status == IOTHUB_CLIENT_SEND_STATUS_BUSY))
        {
        	printf("busy \n");
            IoTHubClient_LL_DoWork(iotHubClientHandle);
            ThisThread::sleep_for(100); 
        }
        printf("MESSAGE FLAG SENT SET\n");
        deleteOK.wait_all(0x1);
        delete iotDev;
        IoTHubClient_LL_Destroy(iotHubClientHandle);
        messageSentFlag.set(0x1);
    }
    return;
}

