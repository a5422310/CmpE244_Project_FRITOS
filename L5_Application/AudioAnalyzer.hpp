#ifndef AUDIOANALYZER_HPP_
#define AUDIOANALYZER_HPP_

#include <stdio.h>
#include "adc0.h"
#include "FreeRTOS.h"
#include "lpc_sys.h"


#define STROBE_PIN 30
#define RESET_PIN 29

class AudioAnalyzer
{
    public:
        AudioAnalyzer();
        bool init();
        void set_pin(int pin_number);
        void clear_pin(int pin_number);
        uint16_t read_pin_value();
        uint16_t values[7] = {0};
};

#endif /* AUDIOANALYZER_HPP_ */
