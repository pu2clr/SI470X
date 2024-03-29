/*
   It is a SI4703 receiver controlled by an ATtiny85 device using I2C OLED 96" (128x64).
   It is FM receiver with RDS support. You can monitor Station Name, Station Info, Program info and UTC (time) using a display I2C OLED 0,96".
   You can also explore other RDS information by adding more functions.     
   

   ATtiny85 and Si470X wireup  

    | Si470x pin      | Attiny85 REF pin | Physical pin  | 
    | ----------------| -----------------| ------------- | 
    | RESET /RST      |     PB3          |     2         |
    | SEEK_UP         |     PB1          |     6         | 
    | SEEK_DOWN       |     PB4          |     3         |
    | SDIO / SDA      |     SDA          |     5         |
    | SCLK / CLK      |     SCL          |     7         |
   
    Compiling and uploading: 
    1) Install the ATtiny Core in Arduino IDE - Insert the URL http://drazzy.com/package_drazzy.com_index.json on board manager. 
                                                To do that, go to Preferences, enter the above URL in "Additional Boards Manager URLs. 
                                                To setup ATtiny85 on Arduino IDE, go to Tools Menu, Board, Board Manager and install 
                                                "ATTinyCore by Spence Konde". 
    2) Setup: Chip: ATtiny85;  Clock Source: 4MHz (Internal); LTO Enabled; millis() / macros() Enabled; 

    ATTENTION: if you select Clock source 8 MHz, for some reason, the system will work very slow. Maybe a bug. 

   By Ricardo Lima Caratti, 2023.
*/

#include <SI470X.h>
#include <EEPROM.h>  // The ATtiny85 contains 512 bytes of data EEPROM memory. The EEPROM has an endurance of at least 100,000 write/erase cycles.
#include <Tiny4kOLED.h>
#include <5x5_font.h>

#define SEEK_UP PB1
#define SEEK_DOWN PB4
#define TIME_RDS_ON_DISPLAY 2300

#define RESET_PIN       PB3      
#define SDA_PIN         SDA    


#define VALID_DATA 85




char *stationName;
char *stationInfo;
char *programInfo;
char *utcTime;

uint8_t idxProgInfo = 0;
uint8_t idxStationInfo = 0;

long delayStationName = millis();
long delayStationInfo = millis();
long delayProgramInfo = millis();
long delayUtcTime = millis();

uint16_t currentFrequency;


SI470X rx;

void setup() {
  pinMode(SEEK_UP, INPUT_PULLUP);
  pinMode(SEEK_DOWN, INPUT_PULLUP);
  pinMode(RESET_PIN, INPUT_PULLUP);

  oled.begin();
  oled.on();
  oled.clear();
  oled.setFont(FONT6X8);
  oled.setCursor(0, 0);
  oled.print(F("SI470X-ATTiny85"));
  oled.setCursor(0, 2);
  oled.print(F("   By PU2CLR   "));
  delay(2000);
  oled.clear();
  // End Splash
  rx.setup(RESET_PIN, SDA_PIN);
  rx.setVolume(8);

  // Restores the latest frequency and audio mute statis saved into the EEPROM
  if (EEPROM.read(0) == VALID_DATA) {
    currentFrequency = EEPROM.read(1) << 8;
    currentFrequency |= EEPROM.read(2);
  } else {
    currentFrequency = 10390;  // default value
  }

  // RDS setup
  rx.setRDS(true);
  rx.setRdsMode(1);
  rx.setMono(false);

  rx.setFrequency(currentFrequency);


  showStatus();
}

void showStatus() {
  oled.clear();
  oled.setCursor(0, 0);
  oled.print(rx.formatCurrentFrequency());
  rx.clearRdsBuffer();
}

void showRdsText(uint8_t col, uint8_t lin, char *rdsInfo) {
  oled.setCursor(col, lin);
  oled.clearToEOL();
  oled.setCursor(col, lin);
  oled.print(rdsInfo);
}

/**
 * Processes the RDS Station Name, Station Information, Program information and UTC
 * Runs scrolling to show Station Information and Program information
 */
void processRdsInfo() {

  long currentmillis = millis();
  char aux[22];

  // Shows station name on Display each three seconds.
  if (stationName != NULL && (currentmillis - delayStationName) > 3000) {
    showRdsText(60, 0, stationName);
    delayStationName = currentmillis;
  }

  // Shows, with scrolling, station info on display each five seconds.
  if (stationInfo != NULL && strlen(stationInfo) > 1 && (currentmillis - delayStationInfo) > 1000) {
    strncpy(aux, &stationInfo[idxStationInfo], 20);
    aux[20] = 0;
    showRdsText(0, 1, aux);
    idxStationInfo += 2;  // shift left two character
    if (idxStationInfo > 31) idxStationInfo = 0;
    delayStationInfo = currentmillis;
  }

  // Shows, with scrolling, the  program information each a half seconds.
  if (programInfo != NULL && strlen(programInfo) > 1 && (currentmillis - delayProgramInfo) > 500) {
    // Process scrolling
    strncpy(aux, &programInfo[idxProgInfo], 20);
    aux[20] = 0;
    idxProgInfo += 2;  // shift left two character
    showRdsText(0, 2, aux);
    if (idxProgInfo > 60) idxProgInfo = 0;
    delayProgramInfo = currentmillis;
  }
  // Some stations broadcast spurious information. In this case, the displayed time may not make sense.
  if (utcTime != NULL && strlen(utcTime) > 8 && (currentmillis - delayUtcTime) > 60000) {
    showRdsText(0, 3, utcTime);
    delayUtcTime = currentmillis;
  }
}

void loop() {
  uint8_t bkey;
  bkey = (digitalRead(SEEK_UP) << 1) | (digitalRead(SEEK_DOWN) );   
  if (bkey != 0b11) {                                                                            
    if (bkey == 0b01)                                                                             
      rx.seek(SI470X_SEEK_WRAP, SI470X_SEEK_UP, showStatus);
    else 
      rx.seek(SI470X_SEEK_WRAP, SI470X_SEEK_DOWN, showStatus);
    showStatus();
    delay(200);
    // Saves the current frequency if it has changed.
    currentFrequency = rx.getFrequency();
    EEPROM.update(0, VALID_DATA);               // Says that a valid frequency will be saved
    EEPROM.update(1, currentFrequency >> 8);    // stores the current Frequency HIGH byte
    EEPROM.update(2, currentFrequency & 0xFF);  // stores the current Frequency LOW byte

  }

  // Queries RDS information.
  if (rx.getRdsAllData(&stationName, &stationInfo, &programInfo, &utcTime))
    processRdsInfo();

  delay(40);
}
