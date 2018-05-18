#include "FRITOS.hpp"

FRITOS::FRITOS (bool debug = false) : m_debug(debug) {}
FRITOS::~FRITOS () {}

void FRITOS::enableXDCS(void)   { LPC_GPIO1->FIOCLR = (1 << XDCS); }
void FRITOS::disableXDCS(void)  { LPC_GPIO1->FIOSET = (1 << XDCS); }

void FRITOS::enableSDCS(void)   { LPC_GPIO1->FIOCLR = (1 << SDCS); }
void FRITOS::disableSDCS(void)  { LPC_GPIO1->FIOSET = (1 << SDCS); }

void FRITOS::enableXCS(void)    { LPC_GPIO1->FIOCLR = (1 << XCS); }
void FRITOS::disableXCS(void)   { LPC_GPIO1->FIOSET = (1 << XCS); }

void FRITOS::enableRST(void)    { LPC_GPIO1->FIOCLR = (1 << RST); }
void FRITOS::disableRST(void)   { LPC_GPIO1->FIOSET = (1 << RST); }

void FRITOS::initMP3(void)
{
    if(m_debug) printf("Initializing SJOne...\n");

    LPC_SC->PCONP |= (1 << 10);     // SPI1 Power Enable
    LPC_SC->PCLKSEL0 &= ~(3 << 20); // Reset PCLK_SPI1 = PCLK_peripheral = CCLK/4 = 24 MHz
    LPC_SC->PCLKSEL0 |=  (1 << 20); // Set PCLK_SPI1 = PCLK_peripheral = CCLK/1 = 96 MHz

    // Select MISO1, MOSI1, and SCK1 pin-select functionality
    LPC_PINCON->PINSEL0 &= ~( (3 << 14) | (3 << 16) | (3 << 18) );
    LPC_PINCON->PINSEL0 |=  ( (2 << 14) | (2 << 16) | (2 << 18) );

    LPC_SSP1->CR0 = 7;      // Data Size Select -> 8-bit data transfer
    LPC_SSP1->CR1 = 2;      // SPI Enable
    LPC_SSP1->CPSR = 56;    // SPI1 Bus Speed = PCLK_SPI1 / 56 = 1.714 MHz
                            // Max SCI reads are CLKI/7. Input clock is 12.288 MHz. Max SPI speed is 1.75 MHz

    // Select GPIO Port 1.23, 1.28, 1.29, 1.30, 1.31 pin-select functionality
    LPC_PINCON->PINSEL3 &= ~( (3 << 14) | (3 << 24) | (3 << 26) | (3 << 28) | (3 << 30) );
    LPC_PINCON->PINSEL3 |=  ( (0 << 14) | (0 << 24) | (0 << 26) | (0 << 28) | (0 << 30) );

    LPC_GPIO1->FIODIR &= ~(1 << DREQ);  // P1.23 <- DREQ
    LPC_GPIO1->FIODIR |= (1 << XDCS);   // P1.28 -> XDCS
    LPC_GPIO1->FIODIR |= (1 << SDCS);   // P1.29 -> SDCS
    LPC_GPIO1->FIODIR |= (1 << XCS);    // P1.30 -> XCS
    LPC_GPIO1->FIODIR |= (1 << RST);    // P1.31 -> RST

    if(m_debug) printf("SJOne initialized!\n\n");

    vTaskDelay(1000);

    setupMP3();
}

bool FRITOS::mp3Ready(void)
{
    return (LPC_GPIO1->FIOPIN & (1 << DREQ));
}

void FRITOS::setupMP3()
{
    if(m_debug) printf("Initializing VS1053B...\n");

    disableSDCS();
    vTaskDelay(10);
    disableXCS();
    vTaskDelay(10);
    disableXDCS();
    vTaskDelay(10);
    enableRST();
    vTaskDelay(10);
    transfer(0xFF);
    vTaskDelay(10);
    disableRST();
    vTaskDelay(10);
    setVolume(10, 10);
    vTaskDelay(10);
//    setBass(20, 20);
//    vTaskDelay(10);

    writeRegister(SCI_CLOCKF, 0x80, 0x00);  // Set clock multiplier to 3.5 * 12.288 MHz = 43 MHz
                                            // Max SCI reads are CLKI/7. Input clock is 43 MHz. Max SPI speed is 6.14 MHz
    LPC_SSP1->CPSR = 16;                    // SPI1 Bus Speed = PCLK_SPI1 / 16 = 6 MHz

    if(m_debug)
    {
        int MP3Mode = readRegister(SCI_MODE);
        int MP3Status = readRegister(SCI_STATUS);
        int MP3Clock = readRegister(SCI_CLOCKF);
        int vsVersion = (MP3Status >> 4) & 0x000F;
        printf("SCI_Mode (0x4800) = 0x%x\n", MP3Mode);
        printf("SCI_Status (0x48) = 0x%x\n", MP3Status);
        printf("VS Version (VS1053 is 4) = %d\n", vsVersion);
        printf("SCI_ClockF = 0x%x\n", MP3Clock);
    }

    if(m_debug) printf("VS1053B initialized!\n\n");
}

uint8_t FRITOS::transfer(uint8_t byte)
{
    LPC_SSP1->DR = byte;
    while((LPC_SSP1->SR & (1 << 4)));
    return LPC_SSP1->DR;
}

void FRITOS::writeRegister(uint8_t addressbyte, uint8_t highbyte, uint8_t lowbyte)
{
    if(xSemaphoreTake(xMutex, portMAX_DELAY))
    {
        while(!mp3Ready());
        enableXCS();
        transfer(SPI_WRITE);
        transfer(addressbyte);
        transfer(highbyte);
        transfer(lowbyte);
        while(!mp3Ready());
        disableXCS();
        xSemaphoreGive(xMutex);
    }
}

int FRITOS::readRegister(uint8_t addressbyte)
{
    int resultvalue = 0xFF;
    uint8_t response1 = 0xFF;
    uint8_t response2 = 0xFF;

    if(xSemaphoreTake(xMutex, portMAX_DELAY))
    {
        while(!mp3Ready());
        enableXCS();
        transfer(SPI_READ);
        transfer(addressbyte);
        response1 = transfer(0xFF);
        while(!mp3Ready());
        response2 = transfer(0xFF);
        while(!mp3Ready());
        disableXCS();
        xSemaphoreGive(xMutex);
    }

    resultvalue = response1 << 8;
    resultvalue |= response2;
    return resultvalue;
}

void FRITOS::playMP3(const char* trackName)
{
    unsigned int bytesRead = 0;
    char filePath[128] = {0};
    uint8_t mp3DataBuffer[32];
    volatile bool need_data = true;

    snprintf (filePath, sizeof(filePath), "1:%s", trackName);
    if(FR_OK != f_open(&mp3File, filePath, FA_OPEN_EXISTING | FA_READ))
    {
        if(m_debug) printf("ERROR: Failed to open: %s\n", trackName);
        return;
    }
    if(m_debug) printf("PLAYING: %s\n", trackName);

    while(1)
    {
        while(!mp3Ready())
        {
            if(need_data == true)
            {
                if(FR_OK != f_read(&mp3File, mp3DataBuffer, sizeof(mp3DataBuffer), &bytesRead))
                {
                    f_close(&mp3File);
                    if(m_debug) printf("ERROR: Could not read mp3 file or no data left to read\n");
                    break;
                }
                need_data = false;
            }
        }

        if(need_data == true)
        {
            if(FR_OK != f_read(&mp3File, mp3DataBuffer, sizeof(mp3DataBuffer), &bytesRead))
            {
                f_close(&mp3File);
                if(m_debug) printf("ERROR: Could not read mp3 file or no data left to read\n");
                break;
            }
            need_data = false;
        }

        if(xSemaphoreTake(xMutex, portMAX_DELAY))
        {
            enableXDCS();   // Select Data
            for(uint8_t y = 0; y < sizeof(mp3DataBuffer); y++)
            {
                transfer(mp3DataBuffer[y]);
            }
            disableXDCS();  // Deselect Data
            xSemaphoreGive(xMutex);
        }
        need_data = true;
        if(bytesRead == 0) break;
    }

    while(!mp3Ready());
    disableXDCS();  // Deselect Data

    f_close(&mp3File);

    if(m_debug) printf("PLAYING DONE: %s\n", trackName);
}

void FRITOS::setVolume(uint8_t left, uint8_t right)
{
    writeRegister(SCI_VOL, left, right);
}

void FRITOS::setBass(uint8_t left, uint8_t right)
{
    writeRegister(SCI_BASS, left, right);
}

void FRITOS::startPlaylist(const char* playlistName)
{
    unsigned int bytesRead = 0;
    char playlistDataBuffer[512] = {0};
    char filePath[128] = {0};
    char trackName[64] = {0};

    snprintf (filePath, sizeof(filePath), "1:%s", playlistName);
    if(FR_OK != f_open(&mp3File, filePath, FA_OPEN_EXISTING | FA_READ))
    {
        if(m_debug) printf("ERROR: Failed to open %s\n", playlistName);
        return;
    }
    if(m_debug) printf("OPENED PLAYLIST: %s\n", playlistName);
    if(FR_OK != f_read(&mp3File, playlistDataBuffer, sizeof(playlistDataBuffer), &bytesRead))
    {
        f_close(&mp3File);
        if(m_debug) printf("ERROR: Could not read %s\n", playlistName);
        return;
    }
    f_close(&mp3File);

    uint8_t j = 0;
    for(uint8_t i = 0; i < bytesRead; ++i)
    {
        if(playlistDataBuffer[i] == '\n')
        {
            trackName[j] = '\0';
            playMP3(trackName);
            memset(trackName, 0, sizeof(trackName)*sizeof(trackName[0]));
            j = 0;
            vTaskDelay(1000);
        }
        else
        {
            trackName[j] = playlistDataBuffer[i];
            j++;
        }
    }

    if(m_debug) printf("PLAYLIST DONE: %s\n", playlistName);
}

void FRITOS::initRGB(void)
{
    if(m_debug) printf("Initializing SJOne...\n");

//    LPC_SC->PCONP |= (1 << 21);         // SPI0 Power Enable
//    LPC_SC->PCLKSEL0 &= ~(3 << 10);     // Reset PCLK_SPI0 = PCLK_peripheral = CCLK/4 = 24 MHz
//    LPC_SC->PCLKSEL0 |=  (1 << 10);     // Set PCLK_SPI0 = PCLK_peripheral = CCLK/3 = 12 MHz

    // Select GPIO Port 0.0, 0.1 pin-select functionality
    LPC_PINCON->PINSEL0 &= ~(0x4);
    LPC_GPIO0->FIODIR |= (1 << LAT);    // P0.1 -> LAT
    LPC_GPIO0->FIODIR |= (1 << OE);     // P0.0 -> OE
    LPC_GPIO0->FIOSET = (1 << OE);
    LPC_GPIO0->FIOCLR = (1 << LAT);

    // Select GPIO Port 2.0, 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8 pin-select functionality
    LPC_PINCON->PINSEL4 &= ~(0x3FFFF);
    LPC_GPIO2->FIODIR |= (1 << R1);     // P2.0 -> R1
    LPC_GPIO2->FIODIR |= (1 << G1);     // P2.1 -> G1
    LPC_GPIO2->FIODIR |= (1 << B1);     // P2.2 -> B1
    LPC_GPIO2->FIODIR |= (1 << R2);     // P2.3 -> R2
    LPC_GPIO2->FIODIR |= (1 << G2);     // P2.4 -> G2
    LPC_GPIO2->FIODIR |= (1 << B2);     // P2.5 -> B2

    // Select GPIO Port 1.20 pin-select functionality
    LPC_PINCON->PINSEL3 &= ~(3 << 8);
    LPC_GPIO1->FIODIR |= (1 << CLK);      // P1.20 -> CLK
    LPC_GPIO1->FIOCLR = (1 << CLK);

    LPC_GPIO2->FIODIR |= (1 << A);      // P2.6 -> A
    LPC_GPIO2->FIODIR |= (1 << B);      // P2.7 -> B
    LPC_GPIO2->FIODIR |= (1 << C);      // P2.8 -> C
    LPC_GPIO2->FIOPIN &= ~(0x1FF);

    m_matrixWidth = 32;
    m_matrixHeight = 16;
    for(uint8_t i = 0; i < m_matrixHeight; ++i)
    {
        for(uint8_t j = 0; j < m_matrixWidth; ++j)
        {
            //set which row displays which color         (0bBGR)
            if(i<1)                 m_matrixBuffer[i][j] = 2;   //0     =   7       G
            else if(i<4)            m_matrixBuffer[i][j] = 3;   //1-3   =   4-6     Y
            else if(i<8)            m_matrixBuffer[i][j] = 1;   //5-7   =   0-3     R
            else if(i<10)           m_matrixBuffer[i][j] = 5;   //8,9   =   14,15   P
            else if(i<12)           m_matrixBuffer[i][j] = 4;   //10,11 =   12,13   B
            else if(i<14)           m_matrixBuffer[i][j] = 6;   //12-13 =   10-11   L
            else                    m_matrixBuffer[i][j] = 2;   //14,15 =   8,9     G
        }
    }

    dropping = 7;

    if(m_debug) printf("SJOne initialized!\n");

    vTaskDelay(1000);
}

void FRITOS::DisplayColorMode(uint8_t mode)
{
    m_matrixWidth = 32;
    m_matrixHeight = 16;
    for(uint8_t i = 0; i < m_matrixHeight; ++i)
    {
        for(uint8_t j = 0; j < m_matrixWidth; ++j)
        {
            //set which row displays which color         (0bBGR)
            switch(mode)
            {
                case 0:
                    if(i<1)                 m_matrixBuffer[i][j] = 2;   //0     =   7       G
                    else if(i<4)            m_matrixBuffer[i][j] = 3;   //1-3   =   4-6     Y
                    else if(i<8)            m_matrixBuffer[i][j] = 1;   //5-7   =   0-3     R
                    else if(i<10)           m_matrixBuffer[i][j] = 5;   //8,9   =   14,15   P
                    else if(i<12)           m_matrixBuffer[i][j] = 4;   //10,11 =   12,13   B
                    else if(i<14)           m_matrixBuffer[i][j] = 6;   //12-13 =   10-11   L
                    else                    m_matrixBuffer[i][j] = 2;   //14,15 =   8,9     G
                    break;
                case 1:
                    if(j<5)                 m_matrixBuffer[i][j] = 7;   //W
                    else if(j<10)           m_matrixBuffer[i][j] = 5;   //P
                    else if(j<14)           m_matrixBuffer[i][j] = 4;   //B
                    else if(j<18)           m_matrixBuffer[i][j] = 6;   //L
                    else if(j<22)           m_matrixBuffer[i][j] = 2;   //G
                    else if(j<27)           m_matrixBuffer[i][j] = 3;   //Y
                    else                    m_matrixBuffer[i][j] = 1;   //R
                    break;
                case 2:
                    if(i<2)                 m_matrixBuffer[i][j] = 3;   //Y
                    else if(i<8)            m_matrixBuffer[i][j] = 1;   //R
                    else if(i<13)           m_matrixBuffer[i][j] = 2;   //G
                    else                    m_matrixBuffer[i][j] = 3;   //Y
                    break;
                default:
                    break;
            }
        }
    }
    switch(mode)
    {
        case 0:
            dropping = 7;
            break;
        case 1:
            dropping = 7;
            break;
        case 2:
            dropping = 1;
            break;
        default:
            break;
    }
}

void FRITOS::splashscreen(void)
{
    char splash[16][64] =
    {
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    };
    for(uint8_t move = 0; move < 32; ++move)
    {
        for(uint8_t row = 0; row < 8; ++row)
        {
            disableOE();
            set_color_top(0);
            set_color_bottom(0);
            set_row(row);
            for(uint8_t col = 0; col < 32; ++col)
            {
                set_color_bottom(splash[row][col+move]);
                set_color_top(splash[row+8][col+move]);
                clockTick();
            }
            latchReset();
            enableOE();
            vTaskDelay(5);
        }
        vTaskDelay(100);
    }

}

//https://github.com/2dom/PxMatrix/blob/master/PxMatrix.cpp

void FRITOS::enableOE(void)
{
    LPC_GPIO0->FIOCLR = (1 << OE);
}

void FRITOS::disableOE(void)
{
    LPC_GPIO0->FIOSET = (1 << OE);
}

void FRITOS::clockTick(void)
{
    LPC_GPIO1->FIOSET = (1 << CLK);
    LPC_GPIO1->FIOCLR = (1 << CLK);
}

void FRITOS::latchReset(void)
{
    LPC_GPIO0->FIOSET = (1 << LAT);
    LPC_GPIO0->FIOCLR = (1 << LAT);
}

void FRITOS::set_row(uint8_t row)
{
    uint8_t a_bit = row & 1;
    uint8_t b_bit = (row >> 1) & 1;
    uint8_t c_bit = (row >> 2) & 1;
    a_bit ? LPC_GPIO2->FIOSET = (1 << A) : LPC_GPIO2->FIOCLR = (1 << A);
    b_bit ? LPC_GPIO2->FIOSET = (1 << B) : LPC_GPIO2->FIOCLR = (1 << B);
    c_bit ? LPC_GPIO2->FIOSET = (1 << C) : LPC_GPIO2->FIOCLR = (1 << C);
}

void FRITOS::set_color_bottom(uint32_t color)
{
    uint8_t r1_bit = color & 1;
    uint8_t g1_bit = (color >> 1) & 1;
    uint8_t b1_bit = (color >> 2) & 1;
    r1_bit ? LPC_GPIO2->FIOSET = (1 << R1) : LPC_GPIO2->FIOCLR = (1 << R1);
    g1_bit ? LPC_GPIO2->FIOSET = (1 << G1) : LPC_GPIO2->FIOCLR = (1 << G1);
    b1_bit ? LPC_GPIO2->FIOSET = (1 << B1) : LPC_GPIO2->FIOCLR = (1 << B1);
}

void FRITOS::set_color_top(uint32_t color)
{
    uint8_t r2_bit = color & 1;
    uint8_t g2_bit = (color >> 1) & 1;
    uint8_t b2_bit = (color >> 2) & 1;
    r2_bit ? LPC_GPIO2->FIOSET = (1 << R2) : LPC_GPIO2->FIOCLR = (1 << R2);
    g2_bit ? LPC_GPIO2->FIOSET = (1 << G2) : LPC_GPIO2->FIOCLR = (1 << G2);
    b2_bit ? LPC_GPIO2->FIOSET = (1 << B2) : LPC_GPIO2->FIOCLR = (1 << B2);
}

void FRITOS::set_color_all(uint32_t color)
{
    uint8_t r1_byte = (color >> 16) & 0xFF;
    uint8_t g1_byte = (color >> 8) & 0xFF;
    uint8_t b1_byte = color & 0xFF;
    uint8_t r1_bit = 0;
    uint8_t g1_bit = 0;
    uint8_t b1_bit = 0;

    uint8_t r2_byte = (color >> 16) & 0xFF;
    uint8_t g2_byte = (color >> 8) & 0xFF;
    uint8_t b2_byte = color & 0xFF;
    uint8_t r2_bit = 0;
    uint8_t g2_bit = 0;
    uint8_t b2_bit = 0;

    for (uint8_t i = 0; i < 8; ++i)
    {
        r1_bit = (r1_byte >> i) & 0x01;
        g1_bit = (g1_byte >> i) & 0x01;
        b1_bit = (b1_byte >> i) & 0x01;
        r1_bit ? LPC_GPIO2->FIOSET = (1 << R1) : LPC_GPIO2->FIOCLR = (1 << R1);
        g1_bit ? LPC_GPIO2->FIOSET = (1 << G1) : LPC_GPIO2->FIOCLR = (1 << G1);
        b1_bit ? LPC_GPIO2->FIOSET = (1 << B1) : LPC_GPIO2->FIOCLR = (1 << B1);

        r2_bit = (r2_byte >> i) & 0x01;
        g2_bit = (g2_byte >> i) & 0x01;
        b2_bit = (b2_byte >> i) & 0x01;
        r2_bit ? LPC_GPIO2->FIOSET = (1 << R2) : LPC_GPIO2->FIOCLR = (1 << R2);
        g2_bit ? LPC_GPIO2->FIOSET = (1 << G2) : LPC_GPIO2->FIOCLR = (1 << G2);
        b2_bit ? LPC_GPIO2->FIOSET = (1 << B2) : LPC_GPIO2->FIOCLR = (1 << B2);

        clockTick();
    }

}

void FRITOS::fill_rectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint32_t color)
{
    for(uint8_t x = x1; x <= x2; ++x)
    {
        for(uint8_t y = y1; y <= y2; ++y)
        {
            m_matrixBuffer[y][x] = color;
        }
    }
}

void FRITOS::set_pixel(uint8_t x, uint8_t y, uint32_t color)
{
    m_matrixBuffer[y][x] = color;
}

void FRITOS::drawPixelwithAudio(AudioAnalyzer audio, LabGPIO sw0, LabGPIO sw1, LabGPIO sw2, LabGPIO sw3)
{
//    fill_rectangle(0, 0, 12, 12, 1);
//    fill_rectangle(20, 4, 30, 15, 2);
//    fill_rectangle(15, 0, 19, 7, 7);

//    set_pixel(0, 0, 0xFF0000);
//    set_pixel(0, 1, 0x00FF00);
//    set_pixel(0, 2, 0x0000FF);

//    vTaskDelay(100);


    uint64_t count=0;
    uint8_t top[32]={0};
    uint8_t now[32]={0};
    uint8_t mode=0;
    while(1)
    {
        static uint64_t current_time_us = 0;
        static bool isInitialized = false;

        if(!isInitialized)
        {
            current_time_us = sys_get_uptime_us() + 36;
            while(sys_get_uptime_us() < current_time_us);
            audio.set_pin(RESET_PIN);
            current_time_us = sys_get_uptime_us() + 36;
            while(sys_get_uptime_us() < current_time_us);
            audio.clear_pin(RESET_PIN);
            audio.set_pin(STROBE_PIN);
            isInitialized = true;
//            printf("   63Hz    160Hz   400Hz   1KHz    2.5KHz  6.25KHz 16KHz\n");
        }
        /////////////////////////////////
//        vTaskDelay(20);

        while(sw3.getLevel())
        {
            vTaskDelay(50);
            if(!sw3.getLevel())
            {
                if(mode>1) mode=0;
                else mode++;
                DisplayColorMode(mode);
            }
        }

        uint16_t values[7] = {0};
        for(int j = 0; j<6; j++)
        {
            for(int i = 0; i<7; i++)
            {
                audio.clear_pin(STROBE_PIN);
                current_time_us = sys_get_uptime_us() + 36;
                while(sys_get_uptime_us() < current_time_us);
                values[i] = audio.read_pin_value();
                if(j==0)                                    audio.values[i] = values[i];
                else if(j>0 && values[i] < audio.values[i]) audio.values[i] = values[i];
//                printf("%6d",audio.values[i]);
//                if(i<6)
//                {
//                    printf("  ");
//                }
//                else
//                {
//                    printf("\r\n");
//                }
                audio.set_pin(STROBE_PIN);
                current_time_us = sys_get_uptime_us() + 36;
                while(sys_get_uptime_us() < current_time_us);
            }
        }
        uint16_t freq=0;
        for(uint8_t row = 0; row < 8; ++row)
        {
            disableOE();
            set_color_top(0);
            set_color_bottom(0);
            set_row(row);
            for(uint8_t col = 0; col < 32; ++col)
            {
                if(col<5)       freq=audio.values[6];
                else if(col<10) freq=audio.values[5];
                else if(col<14) freq=audio.values[4];
                else if(col<18) freq=audio.values[3];
                else if(col<22) freq=audio.values[2];
                else if(col<27) freq=audio.values[1];
                else            freq=audio.values[0];

                if(freq<496)
                {
                    now[col] = 0;
                    if(now[col] > top[col]) top[col] = now[col];
                    set_color_top(0);
                    set_color_bottom(0);
                }
                else if(freq>=496 && freq <736)
                {
                    now[col] = 1;
                    if(now[col] > top[col]) top[col] = now[col];
                    set_color_top(0);
                    if(row>0)   set_color_bottom(0);
                    else if(row==0 && col<5 && freq<650)  set_color_bottom(0);
                    else if(row==0 && col<10 && freq<600)  set_color_bottom(0);
                    else if(row==0 && col<14 && freq<550)  set_color_bottom(0);
                    else        set_color_bottom(m_matrixBuffer[row+8][col]);

                }
                else if(freq>=736 && freq <976)
                {
                    now[col] = 2;
                    if(now[col] > top[col]) top[col] = now[col];
                    set_color_top(0);
                    if(row>1)   set_color_bottom(0);
                    else        set_color_bottom(m_matrixBuffer[row+8][col]);
                }
                else if(freq>=976 && freq <1216)
                {
                    now[col] = 3;
                    if(now[col] > top[col]) top[col] = now[col];
                    set_color_top(0);
                    if(row>2)   set_color_bottom(0);
                    else        set_color_bottom(m_matrixBuffer[row+8][col]);
                }
                else if(freq>=1216 && freq <1456)
                {
                    now[col] = 4;
                    if(now[col] > top[col]) top[col] = now[col];
                    set_color_top(0);
                    if(row>3)   set_color_bottom(0);
                    else        set_color_bottom(m_matrixBuffer[row+8][col]);
                }
                else if(freq>=1456 && freq <1696)
                {
                    now[col] = 5;
                    if(now[col] > top[col]) top[col] = now[col];
                    set_color_top(0);
                    if(row>4)   set_color_bottom(0);
                    else        set_color_bottom(m_matrixBuffer[row+8][col]);

                }
                else if(freq>=1696 && freq <1936)
                {
                    now[col] = 6;
                    if(now[col] > top[col]) top[col] = now[col];
                    set_color_top(0);
                    if(row>5)   set_color_bottom(0);
                    else        set_color_bottom(m_matrixBuffer[row+8][col]);
                }
                else if(freq>=1936 && freq <2176)
                {
                    now[col] = 7;
                    if(now[col] > top[col]) top[col] = now[col];
                    set_color_top(0);
                    if(row>6)   set_color_bottom(0);
                    else        set_color_bottom(m_matrixBuffer[row+8][col]);
                }
                else if(freq>=2176 && freq <2416)
                {
                    now[col] = 8;
                    if(now[col] > top[col]) top[col] = now[col];
                    set_color_bottom(m_matrixBuffer[row+8][col]);
                }
                else if(freq>=2416 && freq <2656)
                {
                    now[col] = 9;
                    if(now[col] > top[col]) top[col] = now[col];
                    set_color_bottom(m_matrixBuffer[row+8][col]);
                    if(row>0)   set_color_top(0);
                    else        set_color_top(m_matrixBuffer[row][col]);
                }
                else if(freq>=2656 && freq <2896)
                {
                    now[col] = 10;
                    if(now[col] > top[col]) top[col] = now[col];
                    set_color_bottom(m_matrixBuffer[row+8][col]);
                    if(row>1)   set_color_top(0);
                    else        set_color_top(m_matrixBuffer[row][col]);
                }
                else if(freq>=2896 && freq <3136)
                {
                    now[col] = 11;
                    if(now[col] > top[col]) top[col] = now[col];
                    set_color_bottom(m_matrixBuffer[row+8][col]);
                    if(row>2)   set_color_top(0);
                    else        set_color_top(m_matrixBuffer[row][col]);
                }
                else if(freq>=3136 && freq <3376)
                {
                    now[col] = 12;
                    if(now[col] > top[col]) top[col] = now[col];
                    set_color_bottom(m_matrixBuffer[row+8][col]);
                    if(row>3)   set_color_top(0);
                    else        set_color_top(m_matrixBuffer[row][col]);
                }
                else if(freq>=3376 && freq <3616)
                {
                    now[col] = 13;
                    if(now[col] > top[col]) top[col] = now[col];
                    set_color_bottom(m_matrixBuffer[row+8][col]);
                    if(row>4)   set_color_top(0);
                    else        set_color_top(m_matrixBuffer[row][col]);
                }
                else if(freq>=3616 && freq <3856)
                {
                    now[col] = 14;
                    if(now[col] > top[col]) top[col] = now[col];
                    set_color_bottom(m_matrixBuffer[row+8][col]);
                    if(row>5)   set_color_top(0);
                    else        set_color_top(m_matrixBuffer[row][col]);
                }
                else if(freq>=3856 && freq <4095)
                {
                    now[col] = 15;
                    if(now[col] > top[col]) top[col] = now[col];
                    set_color_bottom(m_matrixBuffer[row+8][col]);
                    if(row>6)   set_color_top(0);
                    else        set_color_top(m_matrixBuffer[row][col]);
                }
                else if(freq==4095)
                {
                    now[col] = 15;
                    if(now[col] > top[col]) top[col] = now[col];
                    set_color_top(m_matrixBuffer[row][col]);
                    set_color_bottom(m_matrixBuffer[row+8][col]);
                }

                if(row == top[col] && top[col] >1)
                {
                    set_color_bottom(dropping);
                    if(count%10 == 0) top[col]--;
                }
                else if(row == top[col]-8 && top[col] >0)
                {
                    set_color_top(dropping);
                    if(count%10 == 0) top[col]--;
                }
                clockTick();
            }
            latchReset();
            enableOE();
            vTaskDelay(1);
        }
        count++;
    }
}
