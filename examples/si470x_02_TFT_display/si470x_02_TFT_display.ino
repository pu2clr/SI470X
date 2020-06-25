/*
  This sketch uses an Arduino Pro Mini, 3.3V (8MZ) with a SPI TFT from MICROYUM (2" - 176 x 220).
  It is also a FM receiver capable to tune your local FM stations.

  Read more about SSB patch documentation on https://pu2clr.github.io/SI470X/

  Wire up on Arduino UNO, Pro mini

  | Device name               | Device Pin / Description  |  Arduino Pin  |
  | ----------------          | --------------------      | ------------  |
  | Display TFT               |                           |               |
  |                           | RST (RESET)               |      8        |
  |                           | RS  or DC                 |      9        |
  |                           | CS  or SS                 |     10        |
  |                           | SDI                       |     11        |
  |                           | CLK                       |     13        |
  |     Si470X                |                           |               |
  |                           | RESET (pin 15)            |     12        |
  |                           | SDIO (pin 18)             |     A4        |
  |                           | SCLK (pin 17)             |     A5        |
  |     Buttons               |                           |               |
  |                           | Volume Up                 |      4        |
  |                           | Volume Down               |      5        |
  |                           | Stereo/Mono               |      6        |
  |                           | RDS ON/off                |      7        |
  |    Encoder                |                           |               |
  |                           | A                         |       2       |
  |                           | B                         |       3       |

  Prototype documentation: https://pu2clr.github.io/SI470X/
  PU2CLR Si47XX API documentation: https://pu2clr.github.io/SI470X/extras/apidoc/html/

  By PU2CLR, Ricardo,  Feb  2020.
*/

#include <SI470X.h>

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/Tiny3x3a2pt7b.h>
#include <SPI.h>

#include "Rotary.h"


// TFT MICROYUM or ILI9225 based device pin setup
#define TFT_RST 8
#define TFT_DC 9
#define TFT_CS 10  // SS
#define TFT_SDI 11 // MOSI
#define TFT_CLK 13 // SCK
#define TFT_LED 0  // 0 if wired to +3.3V directly
#define TFT_BRIGHTNESS 200


#define COLOR_BLACK    0x0000
#define COLOR_YELLOW   0xFFE0
#define COLOR_WHITE    0xFFFF
#define COLOR_RED      0xF800
#define COLOR_BLUE     0x001F


#define RESET_PIN 14
#define SDA_PIN A4 // SDA pin used by your Arduino Board

// Enconder PINs
#define ENCODER_PIN_A 2
#define ENCODER_PIN_B 3

// Buttons controllers
#define VOLUME_UP 4     // Volume Up
#define VOLUME_DOWN 5   // Volume Down
#define SWITCH_STEREO 6 // Select Mono or Stereo
#define SWITCH_RDS 7    // SDR ON or OFF

char oldFreq[10];


// Encoder control variables
volatile int encoderCount = 0;

uint16_t currentFrequency;

// Encoder control
Rotary encoder = Rotary(ENCODER_PIN_A, ENCODER_PIN_B);

// TFT control
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

SI470X rx;


/*
    Reads encoder via interrupt
    Use Rotary.h and  Rotary.cpp implementation to process encoder via interrupt
*/
void rotaryEncoder()
{ // rotary encoder events
  uint8_t encoderStatus = encoder.process();
  if (encoderStatus)
    encoderCount = (encoderStatus == DIR_CW) ? 1 : -1;
}


/*
   Shows the static content on  display
*/
void showTemplate()
{

    int maxX1 = tft.width() - 2;
    int maxY1 = tft.height() - 5;

    tft.fillScreen(COLOR_BLACK);        

    tft.drawRect(2,  2, maxX1, maxY1, COLOR_YELLOW);
    tft.drawLine(2, 40, maxX1,40, COLOR_YELLOW);
    tft.drawLine(2, 60, maxX1,60, COLOR_YELLOW);

}

/*
  Prevents blinking during the frequency display.
  Erases the old digits if it has changed and print the new digit values.
*/
void printValue(int col, int line, char *oldValue, char *newValue, uint8_t space, uint16_t color)
{
  int c = col;
  char *pOld;
  char *pNew;

  pOld = oldValue;
  pNew = newValue;

  // prints just changed digits
  while (*pOld && *pNew)
  {
    if (*pOld != *pNew)
    {
      // Erases olde value
      tft.setTextColor(COLOR_BLACK);
      tft.setCursor(c, line);
      tft.print(*pOld);
      // Writes new value
      tft.setTextColor(color);
      tft.setCursor(c, line);
      tft.print(*pNew);
    }
    pOld++;
    pNew++;
    c += space;
  }

  // Is there anything else to erase?
  tft.setTextColor(COLOR_BLACK);
  while (*pOld)
  {
    tft.setCursor(c, line);
    tft.print(*pOld);
    pOld++;
    c += space;
  }

  // Is there anything else to print?
  tft.setTextColor(color);
  while (*pNew)
  {
    tft.setCursor(c, line);
    tft.print(*pNew);
    pNew++;
    c += space;
  }

  // Save the current content to be tested next time
  strcpy(oldValue, newValue);
}

/*
   Shows frequency information on Display
*/
void showFrequency()
{
  char freq[10];
  char tmp[10];

  currentFrequency = rx.getFrequency();
  sprintf(tmp, "%5.5u", currentFrequency);

  freq[0] = (tmp[0] == '0')? ' ': tmp[0];
  freq[1] = tmp[1];
  freq[2] = tmp[2];
  freq[3] = '.';
  freq[4] = tmp[3];
  freq[5] = tmp[4];
  freq[6] = '\0';

  tft.setFont(&FreeSerif9pt7b);
  tft.setTextSize(2);
  printValue(15,33, oldFreq, freq,22, COLOR_YELLOW);

}

/*
    Show some basic information on display
*/
void showStatus()
{
  oldFreq[0] = 0;

  showFrequency();

}

/* *******************************
   Shows RSSI status
*/
void showRSSI()
{
}


void showSplash() {
  // Splash
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(45, 23);
  tft.print("SI470X");
  tft.setCursor(10, 50);
  tft.print("Arduino Library");
  tft.setCursor(25, 80);
  tft.print("By PU2CLR");
  tft.setFont(&Tiny3x3a2pt7b);
  tft.setTextSize(2);
  tft.setCursor(20, 110);
  tft.print("Ricardo Lima Caratti");
  delay(4000);
  
}

void setup()
{

  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);

  // Push button pin
  pinMode(VOLUME_UP, INPUT_PULLUP);
  pinMode(VOLUME_DOWN, INPUT_PULLUP);
  pinMode(SWITCH_STEREO, INPUT_PULLUP);
  pinMode(SWITCH_RDS, INPUT_PULLUP);

  tft.initR(INITR_BLACKTAB);
  tft.setFont(&FreeSerif9pt7b);
  tft.fillScreen(COLOR_BLACK);
  tft.setTextColor(COLOR_BLUE);
  tft.setRotation(1);

  showSplash();
  showTemplate();

  // Encoder interrupt
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), rotaryEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), rotaryEncoder, CHANGE);

  rx.setup(RESET_PIN, A4 /* SDA pin  for Arduino ATmega328 */);
  rx.setVolume(6);
  rx.setFrequency(10650); // It is the frequency you want to select in MHz multiplied by 100.

  showStatus();
}



void loop()
{

    // Check if the encoder has moved.
    if (encoderCount != 0)
    {
      if (encoderCount == 1)
          rx.setFrequencyUp();
      else
         rx.setFrequencyDown();
      showFrequency();
      encoderCount = 0;
    }
   delay(100); 
}
