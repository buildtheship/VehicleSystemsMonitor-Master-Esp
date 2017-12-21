#include <OneWire.h>

// Supporting Arduino Forum Topics:
// Waveshare e-paper displays with SPI: http://forum.arduino.cc/index.php?topic=487007.0
// Good Dispay ePaper for Arduino : https://forum.arduino.cc/index.php?topic=436411.0

// mapping suggestion from Waveshare Paper to Wemos D1 mini
// BUSY -> D2, RST -> D4, DC -> D3, CS -> D8, CLK -> D5, DIN -> D7, GND -> GND, 3.3V -> 3.3V

// mapping suggestion for ESP32, e.g. LOLIN32, see .../variants/.../pins_arduino.h for your board
// NOTE: there are variants with different pins for SPI ! CHECK SPI PINS OF YOUR BOARD
// BUSY -> 4, RST -> 16, DC -> 17, CS -> SS(5), CLK -> SCK(18), DIN -> MOSI(23), GND -> GND, 3.3V -> 3.3V

// mapping suggestion for AVR, UNO, NANO etc.
// BUSY -> 7, RST -> 9, DC -> 8, CS-> 10, CLK -> 13, DIN -> 11

// include library, include base class, make path known
#include <GxEPD.h>

// select the display class to use, only one
//#include <GxGDEP015OC1/GxGDEP015OC1.cpp>    // 1.54" b/w
//#include <GxGDEW0154Z04/GxGDEW0154Z04.cpp>  // 1.54" b/w/r
//#include <GxGDE0213B1/GxGDE0213B1.cpp>      // 2.13" b/w
//#include <GxGDEW0213Z16/GxGDEW0213Z16.cpp>  // 2.13" b/w/r
//#include <GxGDEH029A1/GxGDEH029A1.cpp>      // 2.9" b/w
//#include <GxGDEW029Z10/GxGDEW029Z10.cpp>    // 2.9" b/w/r
//#include <GxGDEW027C44/GxGDEW027C44.cpp>    // 2.7" b/w/r
#include <GxGDEW042T2/GxGDEW042T2.cpp>      // 4.2" b/w
//#include <GxGDEW075T8/GxGDEW075T8.cpp>      // 7.5" b/w
//#include <GxGDEW075Z09/GxGDEW075Z09.cpp>    // 7.5" b/w/r

// uncomment next line for drawBitmap() test
#include GxEPD_BitmapExamples

// FreeFonts from Adafruit_GFX
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

#include <GxIO/GxIO_SPI/GxIO_SPI.cpp>
#include <GxIO/GxIO.cpp>

#if defined(ESP8266)

// generic/common.h
//static const uint8_t SS    = 15;
//static const uint8_t MOSI  = 13;
//static const uint8_t MISO  = 12;
//static const uint8_t SCK   = 14;
// pins_arduino.h
//static const uint8_t D8   = 15;
//static const uint8_t D7   = 13;
//static const uint8_t D6   = 12;
//static const uint8_t D5   = 14;

// GxIO_SPI(SPIClass& spi, int8_t cs, int8_t dc, int8_t rst = -1, int8_t bl = -1);
GxIO_Class io(SPI, SS, 0, 2); // arbitrary selection of D3(=0), D4(=2), selected for default of GxEPD_Class
                              // GxGDEP015OC1(GxIO& io, uint8_t rst = 2, uint8_t busy = 4);
GxEPD_Class display(io); // default selection of D4(=2), D2(=4)

#elif defined(ESP32)

// pins_arduino.h, e.g. LOLIN32
//static const uint8_t SS    = 5;
//static const uint8_t MOSI  = 23;
//static const uint8_t MISO  = 19;
//static const uint8_t SCK   = 18;

// GxIO_SPI(SPIClass& spi, int8_t cs, int8_t dc, int8_t rst = -1, int8_t bl = -1);
GxIO_Class io(SPI, SS, 17, 16); // arbitrary selection of 17, 16
                                // GxGDEP015OC1(GxIO& io, uint8_t rst = D4, uint8_t busy = D2);
GxEPD_Class display(io, 16, 4); // arbitrary selection of (16), 4

#endif


OneWire net(D1);

byte addr_DS2431[8];
bool DS2431_ok = false;

void ReadAndReportDS2431(OneWire* net, uint8_t* addr);
void WriteRow(OneWire* net, uint8_t* addr, byte row, byte* buffer);
void PrintBytes(uint8_t* addr, uint8_t count, bool newline=0);

void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.println("setup");

    display.init();

    while (!FindDs2431())
    {
        delay(2000);
    }

    Serial.println("setup done");
}

bool FindDs2431()
{
    if (!net.search(addr_DS2431)) {
        Serial.print("No more addresses.\n");
        net.reset_search();
        delay(1000);
    }
    else
    {
        if (OneWire::crc8(addr_DS2431, 7) != addr_DS2431[7]) {
            Serial.print("CRC is not valid!\n");
        }
        else
        {
            DS2431_ok = addr_DS2431[0] == 0x2D;
            if (DS2431_ok)
            {
                PrintBytes(addr_DS2431, 8);
                Serial.print(" is a DS2431.\n");
                ReadAndReportDS2431(&net, addr_DS2431);
                return true;
            }
            else
            {
                PrintBytes(addr_DS2431, 8);
                Serial.print(" is an unknown device.\n");
            }
        }
    }
    return false;
}

const GFXfont* f = &FreeMonoBold12pt7b;

void loop()
{

    uint32_t freq_reading = 0;  // frequency read from one-wire slave
    float freq_float = 0.0;

    freq_reading = (uint32_t)(ReadDS2431(&net, addr_DS2431, 0));
    freq_float = ((float)freq_reading) / 100; // cast to float and adjust fixed-point
    Serial.print("Read: ");
    Serial.println(freq_float);
    Serial.print(freq_float);
    Serial.println(" Hz");

    float bus_speed = freq_float*(52.0 / 14.2);
    String str_speed = String(bus_speed);
    str_speed = String(str_speed + "mi/hr");


    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(f);

    display.setCursor(10, 0);
    display.println();
    display.println(str_speed);

    display.update();

    delay(5000); // delay in ms
}

void EPDTest()
{
    showBitmapExample();
    drawCornerTest();
    showFont("FreeMonoBold9pt7b", &FreeMonoBold9pt7b);
    showFont("FreeMonoBold12pt7b", &FreeMonoBold12pt7b);
    showFont("FreeMonoBold18pt7b", &FreeMonoBold18pt7b);
    showFont("FreeMonoBold24pt7b", &FreeMonoBold24pt7b);
}

void showBitmapExample()
{
#ifdef _GxBitmapExamples_H_
    display.drawExampleBitmap(BitmapExample1, sizeof(BitmapExample1));
    delay(2000);
    display.drawExampleBitmap(BitmapExample2, sizeof(BitmapExample2));
    delay(2000);
    display.setRotation(0);
    display.fillScreen(GxEPD_WHITE);
    display.drawExampleBitmap(BitmapExample1, 0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_BLACK);
    display.update();
    delay(10000);
#endif
}

void showFont(const char name[], const GFXfont* f)
{
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(f);
    display.setCursor(0, 0);
    display.println();
    display.println(name);
    display.println(" !\"#$%&'()*+,-./");
    display.println("0123456789:;<=>?");
    display.println("@ABCDEFGHIJKLMNO");
    display.println("PQRSTUVWXYZ[\\]^_");

    display.println("`abcdefghijklmno");
    display.println("pqrstuvwxyz{|}~ ");
    display.update();
    delay(5000);
}

void drawCornerTest()
{
    uint8_t rotation = display.getRotation();
    for (uint16_t r = 0; r < 4; r++)
    {
        display.setRotation(r);
        display.fillScreen(GxEPD_WHITE);
        display.fillRect(0, 0, 8, 8, GxEPD_BLACK);
        display.fillRect(display.width() - 18, 0, 16, 16, GxEPD_BLACK);
        display.fillRect(display.width() - 25, display.height() - 25, 24, 24, GxEPD_BLACK);
        display.fillRect(0, display.height() - 33, 32, 32, GxEPD_BLACK);
        display.update();
        delay(5000);
    }
    display.setRotation(rotation); // restore
}



// EEPROM commands
const byte WRITE_SPAD = 0x0F;
const byte READ_SPAD = 0xAA;
const byte COPY_SPAD = 0x55;
const byte READ_MEMORY = 0xF0;

void ReadAndReportDS2431(OneWire* net, uint8_t* addr) {
    int i;
    net->reset();
    delayMicroseconds(1);
    net->select(addr);
    delayMicroseconds(1);
    net->write(READ_MEMORY, 1);  // Read Memory
    delayMicroseconds(1);
    net->write(0x00, 1);  //Read Offset 0000h
    delayMicroseconds(1);
    net->write(0x00, 1);

    for (i = 0; i < 24; i++) //whole mem is 144 
    {
        delayMicroseconds(1);
        Serial.print(net->read(), HEX);
        Serial.print(" ");
    }
    Serial.println();
}

uint32_t ReadDS2431(OneWire* net, uint8_t* addr, uint16_t wordAddr) {
    int i;
    net->reset();
    delayMicroseconds(1);
    net->select(addr);
    delayMicroseconds(1);
    net->write(READ_MEMORY, 1);  // Read Memory
    delayMicroseconds(1);
    net->write((wordAddr * 4) >> 8, 1);  //Read Offset 0000h
    delayMicroseconds(1);
    net->write((byte)(wordAddr * 4), 1);
    delayMicroseconds(1);

    int a = net->read(); // read occurs for succesive bytes for each call (EEPROM)
    delayMicroseconds(1);
    int b = net->read();
    delayMicroseconds(1);
    int c = net->read();
    delayMicroseconds(1);
    int d = net->read();
    Serial.println(a, HEX);
    uint32_t toReturn = 0;
    toReturn = a | (b << 8) | (d << 16) | (c << 24); // big-endian
    return toReturn;
}


void WriteReadScratchPad(OneWire* net, uint8_t* addr, byte TA1, byte TA2, byte* data)
{
    int i;
    net->reset();
    net->select(addr);
    net->write(WRITE_SPAD, 1);  // Write ScratchPad
    net->write(TA1, 1);
    net->write(TA2, 1);
    for (i = 0; i < 8; i++)
        net->write(data[i], 1);

    net->reset();
    net->select(addr);
    net->write(READ_SPAD);         // Read Scratchpad

    for (i = 0; i < 13; i++)
        data[i] = net->read();
}

void CopyScratchPad(OneWire* net, uint8_t* addr, byte* data)
{
    net->reset();
    net->select(addr);
    net->write(COPY_SPAD, 1);  // Copy ScratchPad
    net->write(data[0], 1);
    net->write(data[1], 1);  // Send TA1 TA2 and ES for copy authorization
    net->write(data[2], 1);
    delay(25); // Waiting for copy completion
               //Serial.print("Copy done!\n");
}

void WriteRow(OneWire* net, uint8_t* addr, byte row, byte* buffer)
{
    int i;
    if (row < 0 || row > 15) //There are 16 row of 8 bytes in the main memory
        return;                //The remaining are for the 64 bits register page

    WriteReadScratchPad(net, addr, row * 8, 0x00, buffer);

    /*  Print result of the ReadScratchPad
    for ( i = 0; i < 13; i++)
    {
    Serial.print(buffer[i], HEX);
    Serial.print(" ");
    }*/

    CopyScratchPad(net, addr, buffer);

}

void PrintBytes(uint8_t* addr, uint8_t count, bool newline) {
    for (uint8_t i = 0; i < count; i++) {
        Serial.print(addr[i] >> 4, HEX);
        Serial.print(addr[i] & 0x0f, HEX);
    }
    if (newline)
        Serial.println();
}
