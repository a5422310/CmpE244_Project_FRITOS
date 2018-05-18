#ifndef L5_APPLICATION_EQUALIZER_HPP_
#define L5_APPLICATION_EQUALIZER_HPP_

#include <stdio.h>
#include "scheduler_task.hpp"

#define EQ_OUTPUT_ADC_P026    3   ///< ADC0.3
#define STROBE_PIN 30
#define RESET_PIN 29


class AudioAnalyzer{
    public:
    uint16_t values[7] = {0};
    AudioAnalyzer();
        bool init();
        uint16_t read_pin_value();
        void set_pin(int pin_number);
        void clear_pin(int pin_number);
};

#endif
