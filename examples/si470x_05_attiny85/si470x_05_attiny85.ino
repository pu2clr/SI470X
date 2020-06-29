/*
   Test and validation of SI4703 on ATtiny85 device.
   It is a FM receiver with mini OLED and and two push buttons  
   
   ATiny85 and Si470X wireup  

    | Si470x pin      | Attiny85 REF pin | Physical pin  | 
    | ----------------| -----------------| ------------- | 
    | RESET /RST      |     PB3          |     2         |
    | SEEK_UP         |     PB1          |     6         | 
    | SEEK_DOWN       |     PB4          |     3         |
    | SDIO / SDA      |     SDA          |     5         |
    | SCLK / CLK      |     SCL          |     7         |
   

   By Ricardo Lima Caratti, 2020.
*/

#include <SI470X.h>
#include <Tiny4kOLED.h>



#define RESET_PIN       PB3       // On Arduino Atmega328 based board, this pin is labeled as A0 (14 means digital pin instead analog)
#define SDA_PIN         SDA       //  

#define SEEK_UP   PB1     
#define SEEK_DOWN PB4    

SI470X rx;

void setup()
{
  pinMode(SEEK_UP, INPUT_PULLUP);
  pinMode(SEEK_DOWN, INPUT_PULLUP);

  oled.begin();
  oled.clear();
  oled.on();
  oled.setFont(FONT8X16);
  oled.setCursor(0, 0);
  oled.print("Si470X-Attiny85");
  oled.setCursor(0, 2);
  oled.print("   By PU2CLR   ");
  delay(3000);
  oled.clear();

  rx.setup(RESET_PIN, SDA_PIN);
  rx.setVolume(6);
  rx.setFrequency(10650); // It is the frequency you want to select in MHz multiplied by 100.
  showStatus();
 
}

void showStatus() {
  oled.setCursor(0, 0);
  oled.print("FM ");
  oled.setCursor(38, 0);
  oled.print("      ");
  oled.setCursor(38, 0);
  oled.print(rx.getFrequency() / 100.0);
  oled.setCursor(95, 0);
  oled.print("MHz");
}

void loop()
{
  if (digitalRead(SEEK_UP) == LOW ) {
    rx.seek(SI470X_SEEK_WRAP,SI470X_SEEK_UP);
    showStatus();
  }
  if (digitalRead(SEEK_DOWN) == LOW ) {
    rx.seek(SI470X_SEEK_WRAP,SI470X_SEEK_DOWN);
    showStatus();
  }
  delay(200);
}
