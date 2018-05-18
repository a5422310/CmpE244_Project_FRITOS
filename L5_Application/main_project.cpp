#if 1
#include "FRITOS.hpp"

#include "tasks.hpp"
#include "examples/examples.hpp"
#include "LPC17xx.h"
#include "FreeRTOS.h"
#include "stdio.h"
#include "uart0_min.h"
#include "light_sensor.hpp"
#include "storage.hpp"
#include "event_groups.h"
#include <string.h>
#include <LabGPIO.hpp>

#define DEBUG   1
SemaphoreHandle_t xMutex;

void testRGB(void * params)
{
    FRITOS sjone(DEBUG);
    AudioAnalyzer audio;
    LabGPIO sw0(1,9), sw1(1,10), sw2(1,14), sw3(1,15);

    sjone.initRGB();
    audio.init();
    sw0.setAsInput();
    sw1.setAsInput();
    sw2.setAsInput();
    sw3.setAsInput();
    while(1)
    {
        sjone.splashscreen();
        if(sw0.getLevel() || sw1.getLevel() || sw2.getLevel() || sw3.getLevel())
        {
            sjone.drawPixelwithAudio(audio, sw0, sw1, sw2, sw3);
        }
//        vTaskDelay(1000);
    }

}

void playAudio(void * params)
{
    FRITOS sjone(DEBUG);
    sjone.initMP3();

    while(1)
    {
//        vTaskDelay(60000);
        sjone.playMP3("odesza_a_moment_apart.mp3");
//        sjone.playMP3("odesza_falls.mp3");
//        sjone.playMP3("odesza_higher_ground.mp3");
//        sjone.playMP3("odesza_late_night.mp3");
//        sjone.startPlaylist("odesza_playlist.txt");
        vTaskDelay(999999);
    }
}

int main(void)
{
    const uint32_t STACK_SIZE = 1024;

    xMutex = xSemaphoreCreateMutex();

//    xTaskCreate(playAudio, "playAudio", STACK_SIZE, NULL, PRIORITY_HIGH, NULL);
    xTaskCreate(testRGB, "testRGB", STACK_SIZE, NULL, PRIORITY_HIGH, NULL);

    vTaskStartScheduler();

    return 0;
}
#endif
