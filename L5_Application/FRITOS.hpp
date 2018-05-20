#ifndef FRITOS_HPP_
#define FRITOS_HPP_

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
#include <inttypes.h>
#include <AudioAnalyzer.hpp>
#include <LabGPIO.hpp>
#include "acceleration_sensor.hpp"

extern SemaphoreHandle_t xMutex;

//VS1053 SPI Instructions
#define SPI_WRITE   0x02
#define SPI_READ    0x03

//VS1053 SCI Registers
#define SCI_MODE 0x00
#define SCI_STATUS 0x01
#define SCI_BASS 0x02
#define SCI_CLOCKF 0x03
#define SCI_DECODE_TIME 0x04
#define SCI_AUDATA 0x05
#define SCI_WRAM 0x06
#define SCI_WRAMADDR 0x07
#define SCI_HDAT0 0x08
#define SCI_HDAT1 0x09
#define SCI_AIADDR 0x0A
#define SCI_VOL 0x0B
#define SCI_AICTRL0 0x0C
#define SCI_AICTRL1 0x0D
#define SCI_AICTRL2 0x0E
#define SCI_AICTRL3 0x0F

class FRITOS
{
private:
    uint8_t dropping = 0;
public:
    // Port 1
    enum VS1053_PIN
    {
        DREQ = 23,
        XDCS = 28,
        SDCS = 29,
        XCS = 30,
        RST = 31,
    };
    // Port 2
    enum RGB_PIN
    {
        R1 = 0,
        G1 = 1,
        B1 = 2,
        R2 = 3,
        G2 = 4,
        B2 = 5,
        A = 6,
        B = 7,
        C = 8,
    };
    // Port 0
    enum MATRIX_PIN
    {
        LAT = 1,
        OE = 0,
        CLK = 20,
    };

    uint8_t m_matrixWidth;
    uint8_t m_matrixHeight;

    bool convert_i = 0;
    bool convert_j = 0;

    FRITOS(bool);
    ~FRITOS();

    void initMP3(void);
    void setupMP3(void);
    uint8_t transfer(uint8_t);
    void writeRegister(uint8_t, uint8_t, uint8_t);
    int readRegister(uint8_t);
    bool mp3Ready(void);
    void setVolume(uint8_t, uint8_t);
    void setBass(uint8_t, uint8_t);
    void playMP3(const char*, LabGPIO in1, LabGPIO in2);

    void enableXCS(void);
    void disableXCS(void);

    void enableRST(void);
    void disableRST(void);

    void enableSDCS(void);
    void disableSDCS(void);

    void enableXDCS(void);
    void disableXDCS(void);

    FIL mp3File;
    void startPlaylist(const char*, LabGPIO in1, LabGPIO in2);

    void initRGB(void);
    void DisplayColorMode(uint8_t);
    void splashscreen(void);
    void pausescreen(LabGPIO sw2, LabGPIO out2);
    void enableOE(void);
    void disableOE(void);
    void clockTick(void);
    void latchReset(void);
    void set_row(uint8_t);
    void set_color_top(uint32_t);
    void set_color_bottom(uint32_t);
    void drawPixelwithAudio(AudioAnalyzer audio, LabGPIO sw0, LabGPIO sw1, LabGPIO sw2, LabGPIO sw3, LabGPIO out1, LabGPIO out2, LabGPIO out3);


    volatile uint8_t  m_matrixBuffer[16][32];

    bool m_debug;
};

#endif /* FRITOS_HPP_ */
