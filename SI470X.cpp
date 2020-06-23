
#include <SI470X.h>

/**
 * @brief Gets all current register content of the device
 * @details For read operations, the device acknowledge is followed by an eight bit data word shifted out on falling SCLK edges. An internal address counter automatically increments to allow continuous data byte reads, starting with the upper byte of register 0Ah, followed by the lower byte of register 0Ah, and onward until the lower byte of the last register is reached. The internal address counter then automatically wraps around to the upper byte of register 00h and proceeds from there until continuous reads cease. 
 *
 * @see BROADCAST FM RADIO TUNER FOR PORTABLE APPLICATIONS; page 19.
 * @see deviceRegisters;  
 */
void SI470X::getAllRegisters()
{
    word16_to_bytes aux;
    int i;

    Wire.requestFrom(this->deviceAddress, 32);
    while (Wire.available() < 32)
        ;

    // The registers from 0x0A to 0x0F come first
    for (i = 0x0A; i <= 0XF; i++) {
        aux.refined.highByte = Wire.read();
        aux.refined.lowByte = Wire.read();
        deviceRegisters[i] = aux.raw;
    }

    for (i = 0x00; i <= 0x09; i++) {
        aux.refined.highByte = Wire.read();
        aux.refined.lowByte = Wire.read();
        deviceRegisters[i] = aux.raw;
    }

}

/**
 * @brief   Sets values to the device registers from 0x02 to 0x07
 * @details For write operations, the device acknowledge is followed by an eight bit data word latched internally on rising edges of SCLK. The device acknowledges each byte of data written by driving SDIO low after the next falling SCLK edge, for 1 cycle.
 * @details An internal address counter automatically increments to allow continuous data byte writes, starting with the upper byte of register 02h, followed by the lower byte of register 02h, and onward until the lower byte of the last register is reached. The internal address counter then automatically wraps around to the upper byte of register 00h and proceeds from there until continuous writes end.
 *  @details The registers from 0x2 to 0x07 are used to setup the device. This method writes the array  deviceRegisters, elements 8 to 14 (corresponding the registers 0x2 to 0x7 respectively)  into the device. See Device registers map  in SI470X.h file.
 * @details To implement this, a register maping was created to deal with each register structure. For each type of register, there is a reference to the array element. 
 *  
 * @see BROADCAST FM RADIO TUNER FOR PORTABLE APPLICATIONS; pages 18 and 19.
 * @see deviceRegisters; 
 */
void SI470X::setAllRegisters()
{
    word16_to_bytes aux;
    Wire.beginTransmission(this->deviceAddress);
    for (int i = 0x02; i < 0x08; i++)
    {
        aux.raw = deviceRegisters[i];
        Wire.write(aux.refined.highByte);
        Wire.write(aux.refined.lowByte);
    }
    Wire.endTransmission();
    delay(5);
}

/**
 * @brief Gets the value of the 0x0A register
 * @details This function also updates the value of deviceRegisters[0];
 * @return si470x_reg0a 
 */
void SI470X::getStatus()
{
    word16_to_bytes aux;
    Wire.requestFrom(this->deviceAddress, 2);
    while (Wire.available() < 2)
        ;
    aux.refined.highByte = Wire.read();
    aux.refined.lowByte = Wire.read();
    deviceRegisters[0x0A] = aux.raw;
}

/**
 * @brief Wait for Seet or Tune process 
 * 
 */
void SI470X::waitTune()
{
    do {
        delay(1);
        getStatus();
    } while ( !(reg0a->refined.STC) );
}

/**
 * @ingroup GA03
 * @brief Sets the a value to a given SI470X register
 * 
 * @param reg        register number to be written (0x00  to 0x0F) - See #define REG* constants  
 * @param parameter  content you want to store 
 */
void SI470X::setRegister(uint16_t reg, uint16_t parameter)
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
uint16_t SI470X::getRegister(uint16_t reg)
{

    word16_to_bytes result;

    Wire.beginTransmission(this->deviceAddress);
    Wire.write(reg);
    Wire.endTransmission();
    delayMicroseconds(300);
    Wire.requestFrom(this->deviceAddress, 2);
    while (Wire.available() < 2)
        ;
    result.refined.highByte = Wire.read();
    result.refined.lowByte = Wire.read();
    return result.raw;
}

/**
 * @brief Resets the device
 * 
 */
void SI470X::reset()
{
    pinMode(this->resetPin, OUTPUT);
    delay(10);
    digitalWrite(this->resetPin, LOW);
    delay(10);
    digitalWrite(this->resetPin, HIGH);
    delay(100);
}

void SI470X::powerUp()
{
    getAllRegisters();
    reg07->refined.XOSCEN = this->oscillatorType; // Sets the Crustal
    *reg0f = 0;
    setAllRegisters();
    delay(600);

    getAllRegisters();
    reg02->refined.DMUTE = 1;  // Mutes the device;
    reg02->refined.ENABLE = 1; // Power up
    reg02->refined.DISABLE = 0;

    setAllRegisters();
    delay(500);

    getAllRegisters(); // Gets All registers (current status after powerup)
}

void SI470X::powerDown()
{
    getAllRegisters();
    reg07->raw = 0x7C04;
    reg04->raw = 0x002A;
    reg02->raw = 0x0041;
}

/**
 * @brief Starts the device 
 * @details sets the reset pin, interrupt pins and oscillator type you are using in your project.
 * @param resetPin         // Arduino pin used to reset control.     
 * @param rdsInterruptPin  // optional. Sets the Arduino pin used to RDS function control.
 * @param seekInterruptPin // optional. Sets the Arduino pin used to Seek function control. 
 * @param oscillator_type  // optional. Sets the Oscillator type used (Default Crystal or Ref. Clock). 
 */
void SI470X::setup(int resetPin, int rdsInterruptPin, int seekInterruptPin, uint8_t oscillator_type)
{
    Wire.begin();

    this->resetPin = resetPin;
    if (rdsInterruptPin >= 0)
        this->rdsInterruptPin = rdsInterruptPin;
    if (seekInterruptPin >= 0)
        this->seekInterruptPin = seekInterruptPin;

    this->oscillatorType = oscillator_type;

    reset();

    delay(100);

    powerUp();

    reg04->refined.DE = 1;
    reg04->refined.RDS = 0;
    reg04->refined.STCIEN = 0;
    reg04->refined.AGCD = 0;
    reg05->refined.SEEKTH = 63; // RSSI Seek Threshold;
    this->currentFMBand =   reg05->refined.BAND = 1;
    this->currentFMSpace =  reg05->refined.SPACE = 1;

    setAllRegisters();
    delay(500);
}

/**
 * @brief Starts the device 
 * @details Use this if you are not using interrupt pins in your project
 * @param resetPin         // Arduino pin used to reset control.     
 * @param rdsInterruptPin  // optional. Sets the Arduino pin used to RDS function control.
 * @param seekInterruptPin // optional. Sets the Arduino pin used to Seek function control. 
 */
void SI470X::setup(int resetPin, uint8_t oscillator_type)
{
    setup(resetPin, -1, -1, oscillator_type);
}

void SI470X::setFrequency(uint16_t frequency)
{
    uint16_t channel;

    channel = (frequency - this->startBand[this->currentFMBand]) / this->fmSpace[this->currentFMSpace];

    channel = 190;

    getAllRegisters();
    reg03->refined.CHAN = channel;
    reg03->refined.TUNE = 1;
    reg03->refined.RESERVED = 0;
    setAllRegisters();
    waitTune();
    reg03->refined.TUNE = 0;
    setAllRegisters();
    delay(30);
    this->currentFrequency = frequency;
}

uint16_t SI470X::getFrequency()
{

    return this->currentFrequency;
}

uint16_t SI470X::getRealChannel()
{
    getAllRegisters();
    return reg0b->refined.READCHAN;
}

/**
 * @brief Seek function
 * 
 * @details Seek begins at the current channel, and goes in the direction specified with the SEEKUP bit. Seek operation stops when a channel is qualified as valid according to the seek parameters, the entire band has been searched (SKMODE = 0), or the upper or lower band limit has been reached (SKMODE = 1).
 * @details The STC bit is set high when the seek operation completes and/or the SF/BL bit is set high if the seek operation was unable to find a channel qualified as valid according to the seek parameters. The STC and SF/BL bits must be set low by setting the SEEK bit low before the next seek or tune may begin.
 * @details Seek performance for 50 kHz channel spacing varies according to RCLK tolerance. Silicon Laboratories recommends ±50 ppm RCLK crystal tolerance for 50 kHz seek performance.
 * @details A seek operation may be aborted by setting SEEK = 0.
 * 
 * @param seek_mode  Seek Mode; 0 = Wrap at the upper or lower band limit and continue seeking (default); 1 = Stop seeking at the upper or lower band limit.
 * @param direction  Seek Direction; 0 = Seek down (default); 1 = Seek up.
 */
void SI470X::seek(uint8_t seek_mode, uint8_t direction)
{
    reg03->refined.TUNE = 1;
    setAllRegisters();

    reg02->refined.SEEK = 1; // Enable seek
    reg02->refined.SKMODE = seek_mode;
    reg02->refined.SEEKUP = direction;
    setAllRegisters();
    waitTune();
    reg02->refined.SEEK = 0; // Enable seek
    reg03->refined.TUNE = 0;
    setAllRegisters();
    delay(30);
}

/**
 * @brief Sets the FM Band  
 * @details 
 * 
 * | BAND value     | Description | 
 * | ----------     | ----------- | 
 * |    0           | 00 = 87.5–108 MHz (USA, Europe) (Default) |
 * |    1 (default) | 01 = 76–108 MHz (Japan wide band) | 
 * |    2           | 10 = 76–90 MHz (Japan) | 
 * |    3           | 11 = Reserved | 
 * 
 * @param band  See the table above. If you do not set a parameter, will be considered 1 (76-108MHz)
 */
void SI470X::setBand(uint8_t band)
{
    this->currentFMBand = reg05->refined.BAND = band;
    setAllRegisters();
}

/**
 * @todo 
 * @brief Gets the Rssi
 * 
 * @return int 
 */
int getRssi()
{
    return 0;
}

/**
 * @brief Sets the Softmute true or false
 * @param value  TRUE or FALSE
 */
void SI470X::setSoftmute(bool value)
{
    reg02->refined.DSMUTE = !value; // If true, it has not be disabled
    setAllRegisters();
}

/**
 * @brief Sets the Mute true or false
 * 
 * @param value TRUE or FALSE
 */
void SI470X::setMute(bool value)
{
    reg02->refined.DMUTE = !value; // If true, it has not be disabled
    setAllRegisters();
}

/**
 * @brief Sets the Mono true or false (stereo)
 * 
 * @param value TRUE or FALSE
 */
void SI470X::setMono(bool value)
{
    reg02->refined.MONO = value; // If true, it has not be disabled
    setAllRegisters();
}

/**
 * @brief Sets the Rds Mode Standard or Verbose
 * 
 * @param rds_mode  0 = Standard (default); 1 = Verbose
 */
void SI470X::setRdsMode(uint8_t rds_mode)
{
    reg02->refined.RDSM = rds_mode; // If true, it has not be disabled
    setAllRegisters();
}

/**
 * @brief Sets the audio volume level
 * 
 * @param value  0 to 15 (if 0, mutes the audio)
 */
void SI470X::setVolume(uint8_t value)
{
    if (value > 15)
        return;
    this->currentVolume = reg05->refined.VOLUME = value;
    setAllRegisters();
}

/**
 * @brief Gets the current audio volume level
 * 
 * @return uint8_t  0 to 15
 */
uint8_t SI470X::getVolume()
{
    return this->currentVolume;
}

/**
 * @brief Increments the audio volume
 * 
 */
void SI470X::setVolumeUp()
{
    if (this->currentVolume < 15)
    {
        this->currentVolume++;
        setVolume(this->currentVolume);
    }
}

/**
 * @brief Decrements the audio volume
 * 
 */
void SI470X::setVolumeDown()
{
    if (this->currentVolume > 0)
    {
        this->currentVolume--;
        setVolume(this->currentVolume);
    }
}

/**
 * @brief Gets the Part Number
 * @details If it returns 0x01, so the device is: Si4702/03
 * @return the part number 
 */
uint8_t SI470X::getPartNumber()
{
    return reg00->refined.PN;
}

/**
 * @brief Gets the Manufacturer ID
 * @return number  
 */
uint16_t SI470X::getManufacturerId()
{
    return reg00->refined.MFGID;
}

/**
 * @brief Gets the Firmware Version
 * @details The return velue before powerup will be 0. After powerup should be 010011 (19)
 * @return number 
 */
uint8_t SI470X::getFirmwareVersion()
{
    si470x_reg01 r1;
    r1.raw = getRegister(REG01);
    return r1.refined.FIRMWARE;
}

/**
 * @brief Gets the Device identification
 * @return number 
 */
uint8_t SI470X::getDeviceId()
{
    return reg01->refined.DEV;
}

/**
 * @brief Gets the Chip Version
 * @return number 
 */
uint8_t SI470X::getChipVersion()
{
    return reg01->refined.REV;
}