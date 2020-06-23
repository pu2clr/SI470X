/*
   Test and validation of the circuit.
   This sketch will check the I2C communication bus and try to tune the receiver on a given frequency.
 
   ATTENTION:  Please, avoid using the computer connected to the mains during testing.

   The main advantages of using this sketch are: 
    1) It is a easy way to check if your circuit is working;
    2) You do not need to connect any display device to make your radio works;
    3) You do not need connect any push buttons or encoders to change volume and frequency;
    4) The Arduino IDE is all you need to check your circuit.  
   
    By Ricardo Lima Caratti, 2020.
*/

#include <SI470X.h>

#define RESET_PIN 10

SI470X si470x; 

// Set your FM station frequency. Default is 107.5MHz (107500KHz)
uint32_t fmFreq = 106500;

void setup() {

  Serial.begin(9600);
  while(!Serial);

  /*
  delay(500);
  
  if ( !checkI2C() ) {
    Serial.println("\nAgain..");
    if (!checkI2C() ) {
      Serial.println("\nCheck your circuit!");
    }
  } */

  si470x.setup(RESET_PIN);

  Serial.print("\nPN........................:");
  Serial.print(si470x.getPartNumber(),HEX);
  Serial.print("\nManufacturer..............:");
  Serial.print(si470x.getPartNumber(),HEX);
  Serial.print("\nCHIP Version..............:");
  Serial.print(si470x.getChipVersion(),HEX);    
  Serial.print("\nDevice....................:");
  Serial.print(si470x.getDeviceId(),BIN);
  Serial.print("\nFirmware..................:");
  Serial.print(si470x.getFirmwareVersion(),BIN);

  si470x.setVolume(6);
  si470x.setBand(FM_BAND_USA_EU);   

  si470x.setMute(false);
  si470x.setMono(false);
  
  
  delay(500);
  //****
  Serial.print("\nEstacao 106.5MHz");
  si470x.setFrequency(10650);
  
  Serial.print("\nCurrent Channel: ");
  Serial.println(si470x.getRealChannel());
  delay(5000);

  /*
  for (int i = 0; i < 7; i++ ) {
    Serial.print("\nSeek.");
    si470x.seek(1,0);
    Serial.print("\nCurrent Channel: ");
    Serial.println(si470x.getRealChannel());
    delay(5000);
  } */

  
}

void loop() {
  

}

/**
 * Returns true if device found
 */
bool checkI2C() {
  byte error, address;
  int nDevices;
  Serial.println("I2C bus Scanning...");
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("\nI2C device found at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      nDevices++;
    }
    else if (error==4) {
      Serial.print("\nUnknow error at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
    return false;
  }
  else {
    Serial.println("done\n");
    return true;
  }
}
