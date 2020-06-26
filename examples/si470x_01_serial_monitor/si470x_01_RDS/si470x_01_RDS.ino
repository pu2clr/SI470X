/*
   Test and validation of SI4703 RDS feature.
    
   ATTENTION:  
   Please, avoid using the computer connected to the mains during testing. Used just the battery of your computer. 
   This sketch was tested on ATmega328 based board. If you are not using a ATmega328, please check the pins of your board. 

   By Ricardo Lima Caratti, 2020.
*/

#include <SI470X.h>

#define RESET_PIN 14 // On Arduino Atmega328 based board, this pin is labeled as A0 (14 means digital pin instead analog)

#define MAX_DELAY_RDS 40   // 40ms - polling method

long rds_elapsed = millis();

SI470X rx;

void setup()
{

    Serial.begin(9600);
    while (!Serial) ;

    rx.setup(RESET_PIN, A4 /* SDA pin  for Arduino ATmega328 */);

    rx.setVolume(6);

    delay(500);

    // Select a station with RDS service in your place
    Serial.print("\nEstacao 106.5MHz");
    rx.setFrequency(10650); // It is the frequency you want to select in MHz multiplied by 100.

    // Enables SDR
    rx.setRds(true);
    rx.setRdsMode(0); 
  
}

si470x_rds_blockb b;
char aux[100];
void loop()
{
    if ((millis() - rds_elapsed) > MAX_DELAY_RDS ) {
        if (rx.getRdsReady()) {
            b.blockB = rx.getRdsGroupType();
            // Serial.print("\nYou have RDS");
            sprintf(aux,"\nVersion: %i; Group Type: %i", b.refined.versionCode,b.refined.groupType);
            Serial.print(aux);
        }
    }
}
