#ifndef LABGPIO_HPP_
#define LABGPIO_HPP_

#include <stdio.h>
#include "LPC17xx.h"
#include "lpc_isr.h"
#include "FreeRTOS.h"
#include "task.h"
#include "uart0_min.h"
#include "eint.h"
#include "semphr.h"

void vReadSwitch(void * pvParameters);

class LabGPIO
{
private:
    const uint8_t m_port;
    const uint8_t m_pin;
    volatile LPC_GPIO_TypeDef *m_GPIOPointer;
public:

    LabGPIO(uint8_t port, uint8_t pin);

    void setAsInput();
    void setAsOutput();
    void setDirection(bool output);
    void setHigh();
    void setLow();
    void set(bool high);

    bool getLevel();
    void toggleLevel();
    ~LabGPIO();
};

#endif /* LABGPIO_HPP_ */
