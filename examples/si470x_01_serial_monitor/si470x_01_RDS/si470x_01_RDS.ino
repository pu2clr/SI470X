/*
   Under construction...  See si570x_02_TFT_display

   Test and validation of SI4703 RDS feature.

    Arduino Pro Mini and SI4703 wire up

    | Device  Si470X |  Arduino Pin  |
    | ---------------| ------------  |
    | RESET          |     14/A0     |
    | SDIO           |     A4        |
    | SCLK           |     A5        |


  ATTENTION:
   Please, avoid using the computer connected to the mains during testing. Used just the battery of your computer.
   This sketch was tested on ATmega328 based board. If you are not using a ATmega328, please check the pins of your board.

   By Ricardo Lima Caratti, 2020.
*/

#include <SI470X.h>

#define RESET_PIN 14 // On Arduino Atmega328 based board, this pin is labeled as A0 (14 means digital pin instead analog)

#define MAX_DELAY_RDS 80   // 40ms - polling method
#define MAX_DELAY_STATUS   2000 

long rds_elapsed = millis();
long status_elapsed = millis();


SI470X rx;

void setup()
{

  Serial.begin(9600);
  while (!Serial) ;
  Serial.println("\nPU2CLR SI470X Arduino Library.");


  rx.setup(RESET_PIN, A4 /* SDA pin  for Arduino ATmega328 */);

  rx.setRDS(true); // Turns RDS on

  rx.setVolume(6);

  delay(500);

  // Select a station with RDS service in your place
  Serial.print("\nTuning 106.5MHz");
  rx.setFrequency(10650); // It is the frequency you want to select in MHz multiplied by 100.

  // Enables SDR
  rx.setRds(true);
  rx.setRdsMode(1);
  rx.setMono(false);

  showHelp();

}

void showHelp()
{
  Serial.println("Type U to increase and D to decrease the frequency");
  Serial.println("Type S or s to seek station Up or Down");
  Serial.println("Type + or - to volume Up or Down");
  Serial.println("Type 0 to show current status");
  Serial.println("Type ? to this help.");
  Serial.println("==================================================");
  delay(5000);
}

// Show current frequency
void showStatus()
{
  char aux[80];
  sprintf(aux, "\nYou are tuned on %u MHz | RSSI: %3.3u dbUv | Vol: %2.2u | Stereo: %s\n", rx.getFrequency(), rx.getRssi(), rx.getVolume(), (rx.isStereo()) ? "Yes" : "No" );
  Serial.print(aux);
  status_elapsed = millis();
}



/*********************************************************
   RDS
 *********************************************************/
char *rdsMsg;
char *stationName;
char *rdsTime;

void checkRDS()
{
  if (rx.getRdsReady())
  {
     if ( (rdsMsg = rx.getRdsText2A()) != NULL) 
        Serial.println(rdsMsg);
     else if ( (stationName = rx.getRdsText0A()) != NULL)
        Serial.println(stationName);
     else if ( (rdsTime = rx.getRdsTime()) != NULL )
        Serial.println(rdsTime);
  }
}

void showRds()
{
  if ( rx.getRdsReady() )
    checkRDS();
}


void loop()
{
  if ((millis() - rds_elapsed) > MAX_DELAY_RDS ) {
    if ( rx.getRdsReady() )  checkRDS();
    rds_elapsed = millis();
  }

  if ((millis() - status_elapsed) > MAX_DELAY_STATUS ) {
    showStatus();
    status_elapsed = millis();
  }

  if (Serial.available() > 0)
  {
    char key = Serial.read();
    switch (key)
    {
      case '+':
        rx.setVolumeUp();
        break;
      case '-':
        rx.setVolumeDown();
        break;
      case 'U':
      case 'u':
        rx.setFrequencyUp();
        break;
      case 'D':
      case 'd':
        rx.setFrequencyDown();
        break;
      case 'S':
        rx.seek(SI470X_SEEK_WRAP, SI470X_SEEK_UP);
        break;
      case 's':
        rx.seek(SI470X_SEEK_WRAP, SI470X_SEEK_DOWN);
        break;
      case '?':
        showHelp();
        break;
      default:
        break;
    }
    showStatus();
  }
}
