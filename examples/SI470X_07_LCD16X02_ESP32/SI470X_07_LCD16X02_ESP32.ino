/*

  ESP32 Dev Modeule version.

  This sketch uses an ESP32 with LCD16X02 DISPLAY
  It is also a FM receiver capable to tune your local FM stations.
  This sketch saves the latest status of the receiver into the Atmega328 eeprom.

  TO RESET the EEPROM: Turn your receiver on with the encoder push button pressed.

  Read more on https://pu2clr.github.io/SI470X/

  Wire up ESP32 Dev Module, SI470X and LCD16x02 or LCD16x04

  | Device name               | Device Pin / Description  |  Arduino Pin  |
  | --------------------------| --------------------      | ------------  |
  |    LCD 16x2 or 20x4       |                           |               |
  |                           | D4                        |  GPIO18       |
  |                           | D5                        |  GPIO17       |
  |                           | D6                        |  GPIO16       |
  |                           | D7                        |  GPIO15       |
  |                           | RS                        |  GPIO19       |
  |                           | E/ENA                     |  GPIO23       |
  |                           | RW & VSS & K (16)         |  GND          |
  |                           | A (15) & VDD              |  +Vcc         |
  | --------------------------| ------------------------- | --------------|
  | SI470X                   |                            |               | 
  |                           | VCC                       |  3.3V         |
  |                           | SDIO / SDA                |  GPI21        |
  |                           | SCLK                      |  GPI22        |
  |                           | RST (RESET)               |  GPI12        |     
  | Buttons                   |                           |               |
  |                           | Volume Up                 |  GPIO32       |
  |                           | Volume Down               |  GPIO33       |
  |                           | Stereo/Mono               |  GPIO25       |
  |                           | RDS ON/off                |  GPIO26       |
  | --------------------------| --------------------------| --------------|
  | Encoder                   |                           |               |
  |                           | A                         |  GPIO13       |
  |                           | B                         |  GPIO14       |
  |                           | PUSH BUTTON (encoder)     |  GPIO27       |
  | 

  Prototype documentation: https://pu2clr.github.io/SI470X/
  PU2CLR SI470X API documentation: https://pu2clr.github.io/SI470X/extras/apidoc/html/

  By PU2CLR, Ricardo,  Feb  2023.
*/


#include <SI470X.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>


#include "Rotary.h"

// LCD 16x02 or LCD20x4 PINs
#define LCD_D7 15
#define LCD_D6 16
#define LCD_D5 17
#define LCD_D4 18
#define LCD_RS 19
#define LCD_E 23

#define RST_PIN 12
#define SDIO_PIN 21

#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xFFFF

// Enconder PINs
#define ENCODER_PIN_A 13
#define ENCODER_PIN_B 14

// Buttons controllers
#define VOLUME_UP 32      // Volume Up
#define VOLUME_DOWN 33    // Volume Down
#define SWITCH_RDS 25     // SDR ON/OFF
#define SWITCH_STEREO 26  // Stereo ON/OFF
#define SEEK_FUNCTION 27  // Seek function

#define POLLING_TIME 1900
#define RDS_MSG_TYPE_TIME 25000
#define POLLING_RDS 40

#define STORE_TIME 10000  // Time of inactivity to make the current receiver status writable (10s / 10000 milliseconds).
#define PUSH_MIN_DELAY 300

#define EEPROM_SIZE 512

const uint8_t app_id = 47;  // Useful to check the EEPROM content before processing useful data
const int eeprom_address = 0;
long storeTime = millis();

uint8_t seekDirection = 1;  // 0 = Down; 1 = Up. This value is set by the last encoder direction.

long pollin_elapsed = millis();

int maxX1;
int maxY1;

// Encoder control variables
volatile int encoderCount = 0;
uint16_t currentFrequency;
uint16_t previousFrequency;

// Encoder control
Rotary encoder = Rotary(ENCODER_PIN_A, ENCODER_PIN_B);

// LCD display
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

SI470X rx;

void setup() {

  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);

  // Push button pin
  pinMode(VOLUME_UP, INPUT_PULLUP);
  pinMode(VOLUME_DOWN, INPUT_PULLUP);
  pinMode(SWITCH_STEREO, INPUT_PULLUP);
  pinMode(SWITCH_RDS, INPUT_PULLUP);
  pinMode(SEEK_FUNCTION, INPUT_PULLUP);

  // Start LCD display device
  lcd.begin(16, 2);
  showSplash();

  EEPROM.begin(EEPROM_SIZE);

  // If you want to reset the eeprom, keep the ENCODER PUSH BUTTON  pressed during statup
  if (digitalRead(SEEK_FUNCTION) == LOW) {
    EEPROM.write(eeprom_address, 0);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RESET");
    delay(1500);
    showSplash();
  }

  // Encoder interrupt
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), rotaryEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), rotaryEncoder, CHANGE);

  rx.setup(RST_PIN, SDIO_PIN);

  delay(100);

  // Checking the EEPROM content
  if (EEPROM.read(eeprom_address) == app_id) {
    readAllReceiverInformation();
  } else {
    // Default values
    rx.setVolume(6);
    currentFrequency = previousFrequency = 10390;
  }

  rx.setMono(false);  // Force stereo
  rx.setRDS(true);
  rx.setRdsMode(1);
  
  rx.setFrequency(currentFrequency);  // It is the frequency you want to select in MHz multiplied by 100.

  lcd.clear();
  showStatus();
}


void saveAllReceiverInformation() {
  EEPROM.begin(EEPROM_SIZE);

  // The write function/method writes data only if the current data is not equal to the stored data.
  EEPROM.write(eeprom_address, app_id);
  EEPROM.write(eeprom_address + 1, rx.getVolume());           // stores the current Volume
  EEPROM.write(eeprom_address + 2, currentFrequency >> 8);    // stores the current Frequency HIGH byte for the band
  EEPROM.write(eeprom_address + 3, currentFrequency & 0xFF);  // stores the current Frequency LOW byte for the band
  EEPROM.end();
}

void readAllReceiverInformation() {
  rx.setVolume(EEPROM.read(eeprom_address + 1));
  currentFrequency = EEPROM.read(eeprom_address + 2) << 8;
  currentFrequency |= EEPROM.read(eeprom_address + 3);
}


/*
   To store any change into the EEPROM, it is needed at least STORE_TIME  milliseconds of inactivity.
*/
void resetEepromDelay() {
  delay(PUSH_MIN_DELAY);
  storeTime = millis();
  previousFrequency = 0;
}


/*
    Reads encoder via interrupt
    Use Rotary.h and  Rotary.cpp implementation to process encoder via interrupt
*/
void rotaryEncoder() {  // rotary encoder events
  uint8_t encoderStatus = encoder.process();
  if (encoderStatus)
    encoderCount = (encoderStatus == DIR_CW) ? 1 : -1;
}

void showSplash() {
  lcd.setCursor(0, 0);
  lcd.print("PU2CLR-SI470X");
  lcd.setCursor(0, 1);
  lcd.print("Arduino Library");
  lcd.display();
  delay(1000);
}

/*
   Shows the static content on  display
*/
void showTemplate() {
}


/*
   Shows frequency information on Display
*/
void showFrequency() {
  currentFrequency = rx.getFrequency();
  lcd.setCursor(4, 1);
  lcd.print(rx.formatCurrentFrequency());
  lcd.display();
}

void showFrequencySeek() {
  clearLcdLine(1);
  showFrequency();
}

/*
    Show some basic information on display
*/
void showStatus() {
  clearLcdLine(1);
  showFrequency();
  showStereoMono();
  showRSSI();

  lcd.display();
}

/* *******************************
   Shows RSSI status
*/
void showRSSI() {
  char rssi[12];
  rx.convertToChar(rx.getRssi(), rssi, 3, 0, '.');
  strcat(rssi, "dB");
  lcd.setCursor(13, 1);
  lcd.print(rssi);
}

void showStereoMono() {
  lcd.setCursor(0,1);
  if (rx.isStereo()) {
    lcd.print("ST");
  } else {
    lcd.print("MO");
  }
}

/*********************************************************
   RDS
 *********************************************************/
char *programInfo;
char *stationName;
char *rdsTime;
int currentMsgType = 0;
long polling_rds = millis();
long timeTextType = millis();  // controls the type of each text will be shown (Message, Station Name or time)

int progInfoIndex = 0;  // controls the part of the rdsMsg text will be shown on LCD 16x2 Display

long delayProgramInfo = millis();
long delayStationName = millis();
long delayRdsTime = millis();


/**
  showProgramInfo - Shows the Program Information
*/
void showProgramInfo() {
  char txtAux[17];

  if (programInfo == NULL || strlen(programInfo) < 2 || (millis() - delayProgramInfo) < 1000) return;
  delayProgramInfo = millis();
  clearLcdLine(0);
  programInfo[61] = '\0';  // Truncate the message to fit on display line
  strncpy(txtAux, &programInfo[progInfoIndex], 16);
  rx.adjustRdsText(txtAux,sizeof(txtAux));
  txtAux[16] = '\0';
  progInfoIndex += 3;
  if (progInfoIndex > 60) progInfoIndex = 0;
  lcd.setCursor(0, 0);
  lcd.print(txtAux);
}

/**
   showRDSStation - Shows the 
*/
void showRDSStation() {
  if (stationName == NULL || strlen(stationName) < 2 || (millis() - delayStationName) < 3000) return;
  delayStationName = millis();
  clearLcdLine(0);
  lcd.setCursor(0, 0);
  lcd.print(stationName);
}

void showRDSTime() {
  char txtAux[17];

  if (rdsTime == NULL || strlen(rdsTime) < 2 || (millis() - delayRdsTime) < 60000) return;
  delayRdsTime = millis();
  clearLcdLine(0);
  rdsTime[16] = '\0';
  strncpy(txtAux, rdsTime, 16);
  txtAux[16] = '\0';
  lcd.setCursor(0, 0);
  lcd.print(txtAux);
}



void clearLcdLine(uint8_t line) {
  for (int i = 0; i < 16; i++) {
    lcd.setCursor(i, line);
    lcd.print(' ');
  }
}

void clearRds() {
  programInfo = NULL;
  stationName = NULL;
  rdsTime = NULL;
  progInfoIndex = currentMsgType = 0;
  rx.clearRdsBuffer();
  clearLcdLine(0);
}

void checkRDS() {
  // You must call getRdsReady before calling any RDS query function.

  if (rx.getRdsReady()) {
      programInfo = rx.getRdsProgramInformation();
      stationName = rx.getRdsStationName();
      rdsTime = rx.getRdsTime();

      if (currentMsgType == 0) // Time to show program information
        showProgramInfo();
      else if (currentMsgType == 1) // Time to show Station Name
        showRDSStation();
      else if (currentMsgType == 2) // Time to show UTC time - Some stations broadcast wrong information
         showRDSTime();
  }
}

void showRdsIndicator() {

}

/*********************************************************

 *********************************************************/


void doStereo() {

}

void doRds() {

}

/**
   Process seek command.
   The seek direction is based on the last encoder direction rotation.
*/
void doSeek() {
  rx.seek(SI470X_SEEK_WRAP, seekDirection, showFrequencySeek);  // showFrequency will be called by the seek function during the process.
  delay(200);
  showStatus();
}

void loop() {

  // Check if the encoder has moved.
  if (encoderCount != 0) {
    if (encoderCount == 1) {
      rx.setFrequencyUp();
      seekDirection = SI470X_SEEK_UP;
    } else {
      rx.setFrequencyDown();
      seekDirection = SI470X_SEEK_DOWN;
    }
    showStatus();
    encoderCount = 0;
    storeTime = millis();
  }

  if (digitalRead(VOLUME_UP) == LOW) {
    rx.setVolumeUp();
    resetEepromDelay();
  } else if (digitalRead(VOLUME_DOWN) == LOW) {
    rx.setVolumeDown();
    resetEepromDelay();
  } else if (digitalRead(SWITCH_STEREO) == LOW)
    doStereo();
  else if (digitalRead(SWITCH_RDS) == LOW)
    doRds();
  else if (digitalRead(SEEK_FUNCTION) == LOW)
    doSeek();

  if ((millis() - pollin_elapsed) > POLLING_TIME) {
    showStatus();
    pollin_elapsed = millis();
  }

  if ((millis() - polling_rds) > POLLING_RDS) {
    checkRDS();
    polling_rds = millis();
  }

  if ((millis() - timeTextType) > RDS_MSG_TYPE_TIME) {
    // Toggles the type of message to be shown - See showRds function
    currentMsgType++;
    progInfoIndex = 0;
    if (currentMsgType > 2) currentMsgType = 0;
    timeTextType = millis();
  }

  // Show the current frequency only if it has changed
  if ((currentFrequency = rx.getFrequency()) != previousFrequency) {
    clearRds();
    if ((millis() - storeTime) > STORE_TIME) {
      saveAllReceiverInformation();
      storeTime = millis();
      previousFrequency = currentFrequency;
    }
  }

  delay(5);
}
