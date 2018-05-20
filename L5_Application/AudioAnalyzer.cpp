#include <AudioAnalyzer.hpp>

AudioAnalyzer::AudioAnalyzer()
{
}

bool AudioAnalyzer::init()
{
    LPC_PINCON->PINSEL1 &= ~(3 << 26);          //make P0.29 as GPIO (reset)
    LPC_PINCON->PINSEL1 &= ~(3 << 28);          //make P0.30 as GPIO (strobe)
    LPC_GPIO0->FIODIR |= (3 << 29);             //make both as output

    BIT(LPC_PINCON->PINSEL1).b21_20 = 1;        //set p0.26 as ADC

    printf("Equalizer initialized successfully.\n");
    return true;
}

uint16_t AudioAnalyzer::read_pin_value()
{
    return adc0_get_reading(3);                 //adc03
}

void AudioAnalyzer::set_pin(int pin_number)
{
    LPC_GPIO0->FIOPIN |= (1 << pin_number);
}
void AudioAnalyzer::clear_pin(int pin_number)
{
    LPC_GPIO0->FIOPIN &= ~(1 << pin_number);
}
