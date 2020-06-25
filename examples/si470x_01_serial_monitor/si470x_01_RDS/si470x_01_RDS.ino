/*
   Test and validation of SI4703 RDS feature
   
    By Ricardo Lima Caratti, 2020.
*/

#include <SI470X.h>

#define RESET_PIN 10

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

    // Select a station with RDS service
    Serial.print("\nEstacao 106.5MHz");
    rx.setFrequency(10650);

    // Enables SDR
    rx.setRds(true);
    rx.setRdsMode(0); 
  

}

si470x_rds_blockb b;
char aux[100];
void loop()
{
    if ((millis() - rds_elapsed) > MAX_DELAY_RDS ) {
        if (rx.getSdrStatus()) {
            b.blockB = rx.getRdsGroupType();
            // Serial.print("\nYou have RDS");
            sprintf(aux,"\nVersion: %i; Group Type: %i", b.refined.versionCode,b.refined.groupType);
            Serial.print(aux);
        }
    }
}
