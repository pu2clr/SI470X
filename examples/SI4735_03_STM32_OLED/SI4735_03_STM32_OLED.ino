/*
  SI4735 all in one with SSB Support

  This sketch has been successfully tested on STM32F103 Bluepill

  The table below shows the Si4735 and STM32F103C8 pin connections

  | Si4735 pin      |  Arduino Pin  |
  | ----------------| ------------  |
  | RESET (pin 15)  |     PA12      |
  | SDIO (pin 18)   |     PB7 (B7)  |
  | SCLK (pin 17)   |     PB6 (B6)  |

  This sketch uses I2C OLED/I2C, buttons and  Encoder.

  This sketch uses the Rotary Encoder Class implementation from Ben Buxton (the source code is included
  together with this sketch) and Library Adafruit libraries to control the OLED.

  Prototype documentation: https://pu2clr.github.io/SI470X/
  PU2CLR Si47XX API documentation: https://pu2clr.github.io/SI470X/extras/apidoc/html/

  By Ricardo Lima Caratti, Nov 2019.
*/

#include <SI470X.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "Rotary.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    4 // Reset pin # (or -1 if sharing Arduino reset pin)


// Si4735 and receiver constants
#define RESET_PIN PA12
#define SDA_PIN   PB7

// Enconder PINs
#define ENCODER_PIN_A PA0
#define ENCODER_PIN_B PA1

// Buttons controllers
#define VOL_UP PA4           // Volume Up
#define VOL_DOWN PA5         // Volume Down
#define SEEK_UP  PA6         // Seek Up
#define SEEK_DOWN PA7        // Seek Down

#define MIN_ELAPSED_TIME 100
#define MIN_ELAPSED_RSSI_TIME 200

char oldFreq[15];
char oldRssi[15];
char oldVolume[15];

// Encoder control variables
volatile int encoderCount = 0;


long elapsedButton =  millis();
long elapsedRssi   =  millis();

// Some variables to check the SI4735 status
uint16_t currentFrequency;

// Devices class declarations
Rotary encoder = Rotary(ENCODER_PIN_A, ENCODER_PIN_B);

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

SI470X rx;


// Use Rotary.h and  Rotary.cpp implementation to process encoder via interrupt
void rotaryEncoder()
{ // rotary encoder events
  uint8_t encoderStatus = encoder.process();
  if (encoderStatus)
  {
    if (encoderStatus == DIR_CW)
    {
      encoderCount = 1;
    }
    else
    {
      encoderCount = -1;
    }
  }
}

/*
    Writes just the changed information on Display
    Prevents blinking on display and also noise.
    Erases the old digits if it has changed and print the new digit values.
*/
void printValue(int col, int line, char *oldValue, char *newValue, int space, int textSize ) {
  int c = col;
  char * pOld;
  char * pNew;

  pOld = oldValue;
  pNew = newValue;

  oled.setTextSize(textSize);

  // prints just changed digits
  while (*pOld && *pNew)
  {
    if (*pOld != *pNew)
    {
      oled.setTextColor(SSD1306_BLACK);
      oled.setCursor(c, line);
      oled.write(*pOld);
      oled.setTextColor(SSD1306_WHITE);
      oled.setCursor(c, line);
      oled.write(*pNew);
    }
    pOld++;
    pNew++;
    c += space;
  }
  // Is there anything else to erase?
  oled.setTextColor(SSD1306_BLACK);
  while (*pOld)
  {
    oled.setCursor(c, line);
    oled.write(*pOld);
    pOld++;
    c += space;
  }
  // Is there anything else to print?
  oled.setTextColor(SSD1306_WHITE);
  while (*pNew)
  {
    oled.setCursor(c, line);
    oled.write(*pNew);
    pNew++;
    c += space;
  }

  // Save the current content to be tested next time
  strcpy(oldValue, newValue);
}

/*
   Shows the volume level on LCD
*/
void showVolume()
{
  char sVolume[10];
  sprintf(sVolume, "V%2.2u", rx.getVolume());
  printValue(105, 30, oldVolume, sVolume, 6, 1);
  oled.display();
}

void showFrequency()
{
  char freq[15];
  char tmp[15];

  currentFrequency = rx.getFrequency();
  sprintf(tmp, "%5.5u", currentFrequency);

  freq[0] = (tmp[0] == '0') ? ' ' : tmp[0];
  freq[1] = tmp[1];
  freq[2] = tmp[2];
  freq[3] = '.';
  freq[4] = tmp[3];
  freq[5] = tmp[4];
  freq[6] = '\0';

  printValue(23, 0, oldFreq, freq, 12, 2);
  oled.display();
}

/* *******************************
   Shows RSSI status
*/
void showRSSI()
{
  char rssi[10];
  sprintf(rssi, "%i dBuV", rx.getRssi());
  // oled.setFont(&Serif_plain_14);
  oled.setTextSize(1);
  printValue(5, 55, oldRssi, rssi, 11, SSD1306_WHITE);

  oled.display();
}

/*
    Show some basic information on display
*/
void showStatus()
{

  oldFreq[0] = oldRssi[0] = oldVolume[0] = 0;

  oled.clearDisplay();

  oled.drawLine(0, 17, 130, 17, SSD1306_WHITE );
  oled.drawLine(0, 52, 130, 52, SSD1306_WHITE );

  showFrequency();

  showVolume();
  showRSSI();

  oled.display();
}


/*
   Button - Volume control
*/
void volumeButton ( byte d) {

  if ( d == 1 )
    rx.setVolumeUp();
  else
    rx.setVolumeDown();
  showVolume();
  delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
}


void setup()
{
  // Encoder pins
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);

  pinMode(VOL_UP, INPUT_PULLUP);
  pinMode(VOL_DOWN, INPUT_PULLUP);
  pinMode(SEEK_UP, INPUT_PULLUP);
  pinMode(SEEK_DOWN, INPUT_PULLUP);

  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);

  // Splash - Change it for your introduction text.
  oled.setTextSize(1); // Draw 2X-scale text
  oled.setCursor(40, 0);
  oled.print("SI470X");
  oled.setCursor(20, 10);
  oled.print("Arduino Library");
  oled.display();
  delay(500);
  oled.setCursor(15, 20);
  oled.print("Si4703 FM Receiver");
  oled.display();
  delay(500);
  oled.setCursor(30, 35);
  oled.print("SMT32 - OLED");
  oled.setCursor(40, 50);
  oled.print("By PU2CLR");

  oled.display();
  delay(2000);
  // end Splash

  // Encoder interrupt
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), rotaryEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), rotaryEncoder, CHANGE);

  rx.setup(RESET_PIN,  SDA_PIN);
  rx.setVolume(3);   // Minimum 0; Maximum 15
  rx.setMono(false); // Force stereo

  rx.setFrequency(10650);  // It is the frequency you want to select in MHz multiplied by 100. In this case 106.5Mhz.
  rx.setSeekThreshold(30); // Sets RSSI Seek Threshold (0 to 127)

  delay(300);

  currentFrequency = rx.getFrequency();

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

  // Check button commands
  if ((millis() - elapsedButton) > MIN_ELAPSED_TIME)
  {
    // check if some button is pressed
    if (digitalRead(VOL_UP) == LOW)
      volumeButton(1);
    else if (digitalRead(VOL_DOWN) == LOW)
      volumeButton(-1);
    else if (digitalRead(SEEK_UP) == LOW)
      rx.seek(SI470X_SEEK_WRAP, SI470X_SEEK_UP, showFrequency);
    else if (digitalRead(SI470X_SEEK_DOWN) == LOW)
      rx.seek(SI470X_SEEK_WRAP, SEEK_DOWN, showFrequency);
  }

  // Show RSSI status only if this condition has changed
  if ((millis() - elapsedRssi) > MIN_ELAPSED_RSSI_TIME * 40)
  {
    showRSSI();
    elapsedRssi = millis();
  }

  delay(10);
}
