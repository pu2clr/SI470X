
#include <SI470X.h>

/**
 * @ingroup GA03
 * @brief Sets the a value to a given SI470X register
 * 
 * @param reg        register number to be written (0x00  to 0x0F) - See #define REG* constants  
 * @param parameter  content you want to store 
 */
void SI470X::setRegister(uint16_t reg, uint16_t value)
{
    word16_to_bytes param;

    param.raw = parameter;
    Wire.beginTransmission(this->deviceAddress);
    Wire.write(reg);

    Wire.write(param.refined.highByte);
    Wire.write(param.refined.lowByte);

    Wire.endTransmission();
    delayMicroseconds(6000); // TODO: check it
}

/**
 * @brief Gets a given SI470X register content 
 * @details It is a basic function to get a value from a given SI470X device register
 * @param reg  register number to be read (0x00  to 0x0F) - See #define REG* constants 
 * @return the register content
 */
uint16_t SI470X::getRegister(uint16_t reg) {

    word16_to_bytes result;

    Wire.beginTransmission(this->deviceAddress);
    Wire.write(reg);
    Wire.endTransmission(false);
    delayMicroseconds(6000);                    // TODO: check it
    Wire.requestFrom(this->deviceAddress, 2);
    result.refined.highByte = Wire.read();
    result.refined.lowByte = Wire.read();
    Wire.endTransmission(true);
    delayMicroseconds(6000);                    // TODO: check it 

    return result.raw;
}

/**
 * @brief Resets the device
 * 
 */
void SI470X::reset() {
    pinMode(this->resetPin, OUTPUT);
    delay(10);
    digitalWrite(this->resetPin, LOW);
    delay(10);
    digitalWrite(this->resetPin, HIGH);
    delay(10);
}

/**
 * @brief Starts the device 
 * @details sets the reset pin, interrupt pins and oscillator type you are using in your project.
 * @param resetPin         // Arduino pin used to reset control.     
 * @param rdsInterruptPin  // optional. Sets the Arduino pin used to RDS function control.
 * @param seekInterruptPin // optional. Sets the Arduino pin used to Seek function control. 
 * @param oscillator_type  // optional. Sets the Oscillator type used (Default Crystal or Ref. Clock). 
 */
void SI470X::setup(int resetPin, int rdsInterruptPin, int seekInterruptPin, uint8_t oscillator_type) {

    si470x_reg07 r7;

    this->resetPin = resetPin;
    if ( rdsInterruptPin >= 0 ) this->rdsInterruptPin = rdsInterruptPin;
    if ( seekInterruptPin >= 0) this->seekInterruptPin = seekInterruptPin;

    reset();

    Wire.begin();

    r7.raw = getRegister(REG07);   // Gets the current value of the register 0x07;
    r7.refined.XOSCEN = this->oscillatorType = oscillator_type; // Update the oscillator type 
    setRegister(REG07,r7.raw); // Stores the new value. 

}

/**
 * @brief Starts the device 
 * @details Use this if you are not using interrupt pins in your project
 * @param resetPin         // Arduino pin used to reset control.     
 * @param rdsInterruptPin  // optional. Sets the Arduino pin used to RDS function control.
 * @param seekInterruptPin // optional. Sets the Arduino pin used to Seek function control. 
 */
SI470X::setup(int resetPin, uint8_t oscillator_type)
{
    setup(int resetPin, -1, -1, oscillator_type);
}

void SI470X::setFrequency(uint16_t frequency) {}

uint16_t SI470X::getFrequency(){}

void SI470X::seek(uint8_t seek_mode, uint8_t direction) {}

void setBand(uint8_t band) {}

int getRssi() {}

void setSoftmute(bool value) {}

void setMute(bool value) {}

void setMono(bool value) {}

void setRdsMode(uint8_t rds_mode) {}


void SI470X::setVolume(uint8_t value) {
    si470x_reg05 r5;
    if ( value > 31) return;
    r5.raw = getRegister(REG05); // Gets the current register value;
    r5.refined.VOLUME = this->currentVolume = value;
    setRegister(REG05, r5.raw);
}


uint8_t SI470X::getVolume(){
    return this->currentVolume;
}

void SI470X::setVolumeUp() {
    if (this->currentVolume < 31) {
        this->currentVolume++;
        setVolume(this->currentVolume);
    }
}

void SI470X::setVolumeDown() {
    if (this->currentVolume > 0)
    {
        this->currentVolume--;
        setVolume(this->currentVolume);
    }
}


uint8_t SI470X::getPartNumber() {


}

uint16_t SI470X::getManufacturerId() {

}

uint8_t SI470X::getFirmwareVersion() {

}

uint8_t SI470X::getDeviceId() {

}

uint8_t SI470X::getCipVersion() {

}