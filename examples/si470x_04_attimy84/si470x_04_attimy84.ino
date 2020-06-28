/*
   Test and validation of SI4703 on ESP32 board.
    
   ATTENTION:  
   Please, avoid using the computer connected to the mains during testing. Used just the battery of your computer. 
   This sketch was tested on ATmega328 based board. If you are not using a ATmega328, please check the pins of your board. 
    
    | Si470x pin      |     STM32     |
    | ----------------| ------------  |
    | RESET /RST      |     PA12      |
    | SDIO / SDA      |     PB7 (B7)  |
    | SCLK / CLK      |     PB6 (B6)  |

   By Ricardo Lima Caratti, 2020.
*/

#include <SI470X.h>

#define RESET_PIN       PB0       // On Arduino Atmega328 based board, this pin is labeled as A0 (14 means digital pin instead analog)
#define SDA_PIN         PA6        //  

#define MAX_DELAY_RDS 40   // 40ms - polling method

long rds_elapsed = millis();

SI470X rx;


void showHelp()
{

}


// Show current frequency
void showStatus()
{

}


void setup()
{
    
    rx.setup(RESET_PIN, SDA_PIN);

    rx.setVolume(6);

    delay(500);
    rx.setFrequency(10650); // It is the frequency you want to select in MHz multiplied by 100.

    // Enables SDR
    rx.setRds(true);
    rx.setRdsMode(0); 
    rx.setSeekThreshold(30); // Sets RSSI Seek Threshold (0 to 127)

    showHelp();
    showStatus();
}

void loop()
{
  delay(5);
}
