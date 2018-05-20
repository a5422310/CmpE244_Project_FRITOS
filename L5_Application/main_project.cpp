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
#define DEMO    0
SemaphoreHandle_t xMutex;

void testRGB(void * params)
{
    FRITOS sjone(DEMO);
    AudioAnalyzer audio;
    LabGPIO sw0(1,9), sw1(1,10), sw2(1,14), sw3(1,15), out1(1,22), out2(1,23), out3(1,28);

    sjone.initRGB();
    audio.init();
    sw0.setAsInput();
    sw1.setAsInput();
    sw2.setAsInput();
    sw3.setAsInput();
    out1.setAsOutput();
    out2.setAsOutput();
    out3.setAsOutput();

    while(1)
    {
        if(Acceleration_Sensor::getInstance().getX() > 500)
        {
            sjone.convert_i=1;
            sjone.convert_j=1;
        }
        else
        {
            sjone.convert_i=0;
            sjone.convert_j=0;
        }
        sjone.splashscreen();   //display splash screen before pushes any switch
        if(sw0.getLevel() || sw1.getLevel() || sw2.getLevel() || sw3.getLevel())
        {
            out1.setHigh();     //play mp3 while any switches is pushed
            sjone.drawPixelwithAudio(audio, sw0, sw1, sw2, sw3, out1, out2, out3);  //run pixel drawing with audio input
        }
    }

}

void playAudio(void * params)
{
    FRITOS sjone(DEMO);
    LabGPIO in1(0,30), in2(1,19), in3(1,20);
    sjone.initMP3();

    in1.setAsInput();
    in2.setAsInput();
    in3.setAsInput();

    while(1)
    {
        if(in1.getLevel() || in2.getLevel() || in3.getLevel())
        {
            sjone.playMP3("odesza_a_moment_apart.mp3", in1, in2);
            sjone.playMP3("odesza_falls.mp3", in1, in2);
            sjone.playMP3("odesza_higher_ground.mp3", in1, in2);
            sjone.playMP3("odesza_late_night.mp3", in1, in2);
        }
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
