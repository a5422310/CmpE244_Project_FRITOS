#include <AudioAnalyzer.hpp>
#include "bit_manip.h"
#include "eint.h"
#include "lpc_pwm.hpp"
#include "printf_lib.h"
#include "adc0.h"
#include "FreeRTOS.h"
#include "lpc_sys.h"
#include "printf_lib.h"
#include "semphr.h"


AudioAnalyzer::AudioAnalyzer()
{
    /* Nothing to do */
}

bool AudioAnalyzer::init() {


    LPC_PINCON->PINSEL1 &= ~(3 << 26); // make P0.29 as GPIO // use as reset pin
    LPC_PINCON->PINSEL1 &= ~(3 << 28); // make P0.30 as GPIO // use as strobe
    LPC_GPIO0->FIODIR |= (3 << 29); // make both output
    // enable adc

    BIT(LPC_PINCON->PINSEL1).b21_20 = 1; // set p0.26 as ADC //AD0.3

//    vSemaphoreCreateBinary(sem_read_ADC);

    printf("Equalizer initialized successfully.\n");
    return true;
}

uint16_t AudioAnalyzer::read_pin_value() {
    return adc0_get_reading(EQ_OUTPUT_ADC_P026);
}

void AudioAnalyzer::set_pin(int pin_number) {
        LPC_GPIO0->FIOPIN |= (1 << pin_number);
}
void AudioAnalyzer::clear_pin(int pin_number) {
      LPC_GPIO0->FIOPIN &= ~(1 << pin_number);
}
