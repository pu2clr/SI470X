/*
   Test and validation of SI4703 on ATtiny84 device.
   It is FM receiver with  
  
   ATtiny84 and Si470X wireup  

    | Si470x pin      | Attiny84 REF pin | Physical pin  | 
    | ----------------| -----------------| ------------- | 
    | RESET /RST      |     7            |     6         |
    | SEEK_UP         |     3            |    10         | 
    | SEEK_DOWN       |     5            |     8         |
    | ENCODER_PIN_A   |     0            |    13         |
    | ENCODER_PIN_B   |     1            |    12         |  
    | SDIO / SDA      |     SDA          |     7         |
    | SCLK / CLK      |     SCL          |     9         |

   By Ricardo Lima Caratti, 2020.
*/

#include <SI470X.h>
#include <Tiny4kOLED.h>


// Please, check the ATtiny84 physical pins 
#define RESET_PIN           7    // On Arduino Atmega328 based board, this pin is labeled as A0 (14 means digital pin instead analog)
#define SDA_PIN             SDA  //  
#define SEEK_UP             3       
#define SEEK_DOWN           5
#define ENCODER_PIN_A       0
#define ENCODER_PIN_B       1  
  
unsigned char encoder_pin_a;
unsigned char encoder_prev = 0;
unsigned char encoder_pin_b;
long elapsedTimeEncoder = millis();

SI470X rx;

void setup()
{
  pinMode(SEEK_UP, INPUT_PULLUP);
  pinMode(SEEK_DOWN, INPUT_PULLUP);
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);

  oled.begin();
  oled.clear();
  oled.on();
  oled.setFont(FONT8X16);
  oled.setCursor(0, 0);
  oled.print("Si470X-Attiny84A");
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
  if ((millis() - elapsedTimeEncoder) > 5)
  {
    encoder_pin_a = digitalRead(ENCODER_PIN_A);
    encoder_pin_b = digitalRead(ENCODER_PIN_B);
    if ((!encoder_pin_a) && (encoder_prev)) // has ENCODER_PIN_A gone from high to low?
    {                                       // if so,  check ENCODER_PIN_B. It is high then clockwise (1) else counter-clockwise (-1)
      if (encoder_pin_b) 
         rx.setFrequencyUp();
      else
         rx.setFrequencyDown();
      showStatus();   
    }
    encoder_prev = encoder_pin_a;
    elapsedTimeEncoder = millis(); // keep elapsedTimeEncoder updated
  }
 
  if (digitalRead(SEEK_UP) == LOW ) {
    rx.seek(SI470X_SEEK_WRAP,SI470X_SEEK_UP);
    showStatus();
  }
  if (digitalRead(SEEK_DOWN) == LOW ) {
    rx.seek(SI470X_SEEK_WRAP,SI470X_SEEK_DOWN);
    showStatus();
  }
  
 }
