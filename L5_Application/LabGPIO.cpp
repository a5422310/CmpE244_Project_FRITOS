#include "LabGPIO.hpp"
#include "LabGPIOInterrupts.hpp"

volatile bool switchReleased = false;
//SemaphoreHandle_t button_press_semaphore = NULL;

void vControlLED(void * pvParameters)
{
    /* Get Parameter */
    uint32_t param = (uint32_t)(pvParameters);
    /* Define Constants Here */

    /* Define Local Variables and Objects */
    LabGPIO internalLED(1,8);
    LabGPIO externalLED(0,1);

    /* Initialization Code */
    internalLED.setAsOutput();
    internalLED.setHigh();
    externalLED.setAsOutput();
    externalLED.setHigh();

#if 1
    while(1)
    {
        if (xSemaphoreTake(button_press_semaphore, portMAX_DELAY))
        {
            if(param) {
                externalLED.toggleLevel();
                printf("external LED toggled\n");
            } else {
                internalLED.toggleLevel();
                printf("internal LED toggled\n");
            }
        }
    }
#else
    while(1)
    {
        if(switchReleased)
        {
            if(param)
            {
                externalLED.toggleLevel();
                switchReleased = false;
                printf("external LED toggled\n");
            } else {
                internalLED.toggleLevel();
                switchReleased = false;
                printf("internal LED toggled\n");
            }
        }
    }
#endif
    /* Only necessary if above loop has a condition */
    vTaskDelete(NULL);
}

void vReadSwitch(void * pvParameters)
{
    /* Get Parameter */
    uint32_t param = (uint32_t)(pvParameters);
    /* Define Constants Here */

    /* Define Local Variables and Objects */
    LabGPIO internalSwitch(1,15);
    LabGPIO externalSwitch(0,0);

    /* Initialization Code */
    internalSwitch.setAsInput();
    externalSwitch.setAsInput();

    while(1)
    {
        if(param)
        {
            while(externalSwitch.getLevel())
            {
                if(!externalSwitch.getLevel())
                {
                    switchReleased = true;
                    printf("external switch released\n");
                }
            }
        } else {
            while(internalSwitch.getLevel())
            {
                if(!internalSwitch.getLevel())
                {
                    switchReleased = true;
                    printf("internal switch released\n");
                }
            }
        }
    }
    /* Only necessary if above loop has a condition */
    vTaskDelete(NULL);
}

LabGPIO::LabGPIO(uint8_t port, uint8_t pin) :
    m_port(port),
    m_pin(pin)
{
    switch(port)
    {
        case 0:
            m_GPIOPointer = LPC_GPIO0;
            break;
        case 1:
            m_GPIOPointer = LPC_GPIO1;
            break;
        case 2:
            m_GPIOPointer = LPC_GPIO2;
            break;
        case 3:
            m_GPIOPointer = LPC_GPIO3;
            break;
        default:
            break;
    }
}

void LabGPIO::setAsInput()
{
    m_GPIOPointer->FIODIR &= ~(1 << m_pin);
}

void LabGPIO::setAsOutput()
{
    m_GPIOPointer->FIODIR |= (1 << m_pin);
}

void LabGPIO::setDirection(bool output)
{
    output ? setAsOutput() : setAsInput();
}

void LabGPIO::setHigh()
{
    m_GPIOPointer->FIOSET = (1 << m_pin);
}

void LabGPIO::setLow()
{
    m_GPIOPointer->FIOCLR = (1 << m_pin);
}

void LabGPIO::set(bool high)
{
    high ? setHigh() : setLow();
}

bool LabGPIO::getLevel()
{
    return (m_GPIOPointer->FIOPIN & (1 << m_pin));
}

void LabGPIO::toggleLevel()
{
    getLevel() ? setLow() : setHigh();
}

LabGPIO::~LabGPIO() {}
