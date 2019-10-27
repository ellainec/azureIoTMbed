#include "mbed.h"
#include "mbed_stats.h"
#include "rtos.h"
#define DEVICE_STDIO_MESSAGES
DigitalOut led1(LED3);
 
 
   // Blink function toggles the led in a long running loop
//    void blink() {
//            led1 = !led1;
//    }

int main() {
    while (1) {
        printf("with tolchain");
        printf("Deep sleep allowed: %i\r\n", sleep_manager_can_deep_sleep());
        printf("going to deep sleeeeep! \n");
        ThisThread::sleep_for(10000);
        mbed_stats_cpu_t stats;
       mbed_stats_cpu_get(&stats);
       printf("Uptime: %llu ", stats.uptime / 1000);
       printf("Sleep time: %llu ", stats.sleep_time / 1000);
       printf("Deep Sleep: %llu\n", stats.deep_sleep_time / 1000);
    }
}
 