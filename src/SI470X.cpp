/**
 * @mainpage SI470X Arduino Library implementation 
 * @details SI470X Arduino Library implementation. This is an Arduino library for the SI470X, BROADCAST RECEIVER.  
 * @details It works with I2C protocol and can provide an easier interface to control the SI470X device.<br>
 * @details This library was built based on "AN230 - Si4700/01/02/03 PROGRAMMING GUIDE" and  "Si4702/03-C19 - BROADCAST FM RADIO TUNER FOR PORTABLE APPLICATIONS"
 * @details This library can be freely distributed using the MIT Free Software model.
 * @copyright Copyright (c) 2020 Ricardo Lima Caratti. 
 * @author Ricardo LIma Caratti (pu2clr@gmail.com)
 */

#include <SI470X.h>

/** 
 * @defgroup GA03 Basic Functions
 * @section GA03 Basic
 */

/**
 * @ingroup GA03
 * @brief Gets all current register content of the device
 * @details For read operations, the device acknowledge is followed by an eight bit data word shifted out on falling SCLK edges. An internal address counter automatically increments to allow continuous data byte reads, starting with the upper byte of register 0Ah, followed by the lower byte of register 0Ah, and onward until the lower byte of the last register is reached. The internal address counter then automatically wraps around to the upper byte of register 00h and proceeds from there until continuous reads cease. 
 *
 * @see BROADCAST FM RADIO TUNER FOR PORTABLE APPLICATIONS; page 19.
 * @see shadowRegisters;  
 */
void SI470X::getAllRegisters()
{
    word16_to_bytes aux;
    int i;

    Wire.requestFrom(this->deviceAddress, 32);
    delayMicroseconds(300);
    // while (Wire.available() < 32); // It did not work on Attiny Core

    // The registers from 0x0A to 0x0F come first
    for (i = 0x0A; i <= 0x0F; i++)
    {
        aux.refined.highByte = Wire.read();
        aux.refined.lowByte = Wire.read();
        shadowRegisters[i] = aux.raw;
    }

    for (i = 0x00; i <= 0x09; i++)
    {
        aux.refined.highByte = Wire.read();
        aux.refined.lowByte = Wire.read();
        shadowRegisters[i] = aux.raw;
    }
}

/**
 * @ingroup GA03
 * @brief   Sets values to the device registers from 0x02 to 0x07
 * @details For write operations, the device acknowledge is followed by an eight bit data word latched internally on rising edges of SCLK. The device acknowledges each byte of data written by driving SDIO low after the next falling SCLK edge, for 1 cycle.
 * @details An internal address counter automatically increments to allow continuous data byte writes, starting with the upper byte of register 02h, followed by the lower byte of register 02h, and onward until the lower byte of the last register is reached. The internal address counter then automatically wraps around to the upper byte of register 00h and proceeds from there until continuous writes end.
 *  @details The registers from 0x2 to 0x07 are used to setup the device. This method writes the array  shadowRegisters, elements 8 to 14 (corresponding the registers 0x2 to 0x7 respectively)  into the device. See Device registers map  in SI470X.h file.
 * @details To implement this, a register maping was created to deal with each register structure. For each type of register, there is a reference to the array element. 
 *  
 * @see BROADCAST FM RADIO TUNER FOR PORTABLE APPLICATIONS; pages 18 and 19.
 * @see shadowRegisters; 
 */
void SI470X::setAllRegisters(uint8_t limit)
{
    word16_to_bytes aux;
    Wire.beginTransmission(this->deviceAddress);
    for (int i = 0x02; i <= limit; i++)
    {
        aux.raw = shadowRegisters[i];
        Wire.write(aux.refined.highByte);
        Wire.write(aux.refined.lowByte);
    }
    Wire.endTransmission();
}

/**
 * @ingroup GA03
 * @brief Gets the value of the 0x0A register
 * @details This function also updates the value of shadowRegisters[0];
 * @return si470x_reg0a 
 */
void SI470X::getStatus()
{
    word16_to_bytes aux;
    Wire.requestFrom(this->deviceAddress, 2);
    delayMicroseconds(300);
    // while (Wire.available() < 2) ; // It did not work on Attiny Core

    aux.refined.highByte = Wire.read();
    aux.refined.lowByte = Wire.read();
    shadowRegisters[0x0A] = aux.raw;
}

/**
 * @ingroup GA03
 * @brief   Wait STC (Seek/Tune Complete) status becomes 0
 * @details Should be used before processing Tune or Seek.
 * @details The STC bit being cleared indicates that the TUNE or SEEK bits may be set again to start another tune or seek operation. Do not set the TUNE or SEEK bits until the Si470x clears the STC bit. 
 */
void SI470X::waitAndFinishTune()
{
    do
    {
        getStatus();
    } while (reg0a->refined.STC == 0);

    getAllRegisters();
    reg02->refined.SEEK = 0;
    reg03->refined.TUNE = 0;
    setAllRegisters();
    do
    {
        getStatus();
    } while (reg0a->refined.STC != 0);
}

/**
 * @ingroup GA03
 * @brief Resets the device
 */
void SI470X::reset()
{
    pinMode(this->resetPin, OUTPUT);
    digitalWrite(this->resetPin, LOW);
    delay(1);
    digitalWrite(this->resetPin, HIGH);
    delay(1);
}

/**
 * @ingroup GA03
 * @brief Powers the receiver on 
 * @details Starts the receiver with some default configurations 
 */
void SI470X::powerUp()
{
    getAllRegisters();
    reg07->refined.XOSCEN = this->oscillatorType; // Sets the Crustal
    reg07->refined.AHIZEN = 0;
    setAllRegisters();
    delay(this->maxDelayAftarCrystalOn); // You can set this value. See inline function setDelayAfterCrystalOn

    getAllRegisters();

    reg02->refined.DMUTE = 1; // Mutes the device;
    reg02->refined.MONO = 0;
    reg02->refined.DSMUTE = 1;

    reg02->refined.RDSM = 0;
    reg02->refined.SKMODE = 0;
    reg02->refined.SEEKUP = 0;
    reg02->refined.SEEK = 0;
    reg02->refined.ENABLE = 1; // Power up
    reg02->refined.DISABLE = 0;

    reg04->refined.RDSIEN = 0;
    reg04->refined.STCIEN = 0;
    reg04->refined.RDS = 0;
    reg04->refined.DE = 0;
    reg04->refined.AGCD = 1;
    reg04->refined.BLNDADJ = 1;
    // TODO - link the GPIO with interrupt pins
    reg04->refined.GPIO1 = reg04->refined.GPIO2 = reg04->refined.GPIO3 = 0;

    reg05->refined.SEEKTH = 0; // RSSI Seek Threshold;
    this->currentFMBand = reg05->refined.BAND = 0;
    this->currentFMSpace = reg05->refined.SPACE = 1;
    this->currentVolume = reg05->refined.VOLUME = 0;

    reg06->refined.SMUTER = 0;
    reg06->refined.SMUTEA = 0;

    reg06->refined.VOLEXT = 0;
    reg06->refined.SKSNR = 0;
    reg06->refined.SKCNT = 0;

    setAllRegisters();
    delay(60);
    getAllRegisters(); // Gets All registers (current status after powerup)
    delay(60);
}

/**
 * @ingroup GA03
 * @brief Powers the receiver off
 */
void SI470X::powerDown()
{
    getAllRegisters();
    reg07->refined.AHIZEN = 1;
    // reg07->refined.RESERVED = 0x0100;
    reg04->refined.GPIO1 = reg04->refined.GPIO2 = reg04->refined.GPIO3 = 0;
    reg02->refined.ENABLE = 1;
    reg02->refined.DISABLE = 1;
    setAllRegisters();
    delay(100);
}

/**
 * @ingroup GA03
 * @brief Starts the device 
 * @details sets the reset pin, interrupt pins and oscillator type you are using in your project.
 * @details You have to inform at least two parameters: RESET pin and I2C SDA pin of your MCU
 * @param resetPin         // Arduino pin used to reset control. 
 * @param sdaPin           // I2C data bus pin (SDA). For some reason the SI4703 does not start with this pin HIGH      
 * @param rdsInterruptPin  // optional. Sets the Interrupt Arduino pin used to RDS function control.
 * @param seekInterruptPin // optional. Sets the Arduino pin used to Seek function control. 
 * @param oscillator_type  // optional. Sets the Oscillator type used Crystal (default) or Ref. Clock. 
 */
void SI470X::setup(int resetPin, int sdaPin, int rdsInterruptPin, int seekInterruptPin, uint8_t oscillator_type)
{

    if (sdaPin >= 0)
    {
        pinMode(sdaPin, OUTPUT);
        digitalWrite(sdaPin, LOW);
    }

    this->resetPin = resetPin;
    if (rdsInterruptPin >= 0)
        this->rdsInterruptPin = rdsInterruptPin;
    if (seekInterruptPin >= 0)
        this->seekInterruptPin = seekInterruptPin;

    this->oscillatorType = oscillator_type;

    reset();
    Wire.begin();
    delay(1);
    powerUp();
}

/**
 * @ingroup GA03
 * @brief Starts the device 
 * @details Use this if you are not using interrupt pins in your project
 * @param resetPin         // Arduino pin used to reset control.     
 * @param rdsInterruptPin  // optional. Sets the Arduino pin used to RDS function control.
 * @param seekInterruptPin // optional. Sets the Arduino pin used to Seek function control. 
 */
void SI470X::setup(int resetPin, int sdaPin, uint8_t oscillator_type)
{
    setup(resetPin, sdaPin, -1, -1, oscillator_type);
}



/**
 * @ingroup GA03
 * @brief Sets the channel 
 * @param channel 
 */
void SI470X::setChannel(uint16_t channel)
{
    reg03->refined.CHAN = channel;
    reg03->refined.TUNE = 1;
    setAllRegisters();
    delayMicroseconds(60000);
    waitAndFinishTune();
}

/**
 * @ingroup GA03
 * @brief Sets the FM frequency 
 * @details If you want to select 106.5 MHz, send the integer number 10650 (frequency 106.5MHz multiplied by 100).
 * @param frequency  7600 to 1080 (means 76Mhz to 108Mhz)
 */
void SI470X::setFrequency(uint16_t frequency)
{
    uint16_t channel;
    channel = (frequency - this->startBand[this->currentFMBand]) / this->fmSpace[this->currentFMSpace];
    setChannel(channel);
    this->currentFrequency = frequency;
}

/**
 * @ingroup GA03
 * @brief Increments the current frequency
 * @details The increment uses the band space as step. See array: uint16_t fmSpace[4] = {20, 10, 5, 1};
 */
void SI470X::setFrequencyUp()
{
    if (this->currentFrequency < this->endBand[this->currentFMBand])
        this->currentFrequency += this->fmSpace[currentFMSpace];
    else
        this->currentFrequency = this->startBand[this->currentFMBand];

    setFrequency(this->currentFrequency);
}

/**
 * @ingroup GA03
 * @brief Decrements the current frequency
 * @details The drecrement uses the band space as step. See array: uint16_t fmSpace[4] = {20, 10, 5, 1};
 */
void SI470X::setFrequencyDown()
{
    if (this->currentFrequency > this->startBand[this->currentFMBand])
        this->currentFrequency -= this->fmSpace[currentFMSpace];
    else
        this->currentFrequency = this->endBand[this->currentFMBand];

    setFrequency(this->currentFrequency);
}

/**
 * @ingroup GA03
 * @brief Gets the current frequency. 
 * @return uint16_t 
 */
uint16_t SI470X::getFrequency()
{
    return this->currentFrequency;
}

/**
 * @ingroup GA03
 * @brief Gets the current channel stored in register 0x0B
 * @details This method is useful to query the current channel during the seek operations. 
 * @return uint16_t 
 */
uint16_t SI470X::getRealChannel()
{
    getAllRegisters();
    return reg0b->refined.READCHAN;
}

/**
 * @ingroup GA03
 * @brief Gets the frequency based on READCHAN register (0x0B)
 * @details Unlike getFrequency method, this method queries the device. 
 * 
 * @return uint16_t 
 */
uint16_t SI470X::getRealFrequency()
{
    return getRealChannel() * this->fmSpace[this->currentFMSpace] + this->startBand[this->currentFMBand];
}

/**
 * @ingroup GA03
 * @brief Seek function
 * @details Seeks a station up or down.
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
    getAllRegisters();
    reg03->refined.TUNE = 1;
    reg02->refined.SEEK = 1; // Enable seek
    reg02->refined.SKMODE = seek_mode;
    reg02->refined.SEEKUP = direction;
    setAllRegisters();
    waitAndFinishTune();
    setFrequency(getRealFrequency());
}

/**
 * @ingroup GA03
 * @brief Seek function
 * @details Seeks a station up or down.
 * @details Seek up or down a station and call a function defined by the user to show the frequency during the seek process. 
 * @details Seek begins at the current channel, and goes in the direction specified with the SEEKUP bit. Seek operation stops when a channel is qualified as valid according to the seek parameters, the entire band has been searched (SKMODE = 0), or the upper or lower band limit has been reached (SKMODE = 1).
 * @details The STC bit is set high when the seek operation completes and/or the SF/BL bit is set high if the seek operation was unable to find a channel qualified as valid according to the seek parameters. The STC and SF/BL bits must be set low by setting the SEEK bit low before the next seek or tune may begin.
 * @details Seek performance for 50 kHz channel spacing varies according to RCLK tolerance. Silicon Laboratories recommends ±50 ppm RCLK crystal tolerance for 50 kHz seek performance.
 * @details A seek operation may be aborted by setting SEEK = 0.
 * @details It is important to say you have to implement a show frequency function. This function have to get the frequency via getFrequency function.  
 * @details Example:
 * @code
 * 
 * SI470X rx;
 * 
 * void showFrequency() {
 *    uint16_t freq = rx.getFrequency();
 *    Serial.print(freq); 
 *    Serial.println("MHz ");
 * }
 * 
 * void loop() {
 *  .
 *  .
 *      rx.seek(SI470X_SEEK_WRAP, SI470X_SEEK_UP, showFrequency); // Seek Up
 *  .
 *  .
 * }
 * @endcode
 * @param seek_mode  Seek Mode; 0 = Wrap at the upper or lower band limit and continue seeking (default); 1 = Stop seeking at the upper or lower band limit.
 * @param direction  Seek Direction; 0 = Seek down (default); 1 = Seek up.
 * @param showFunc  function that you have to implement to show the frequency during the seeking process. Set NULL if you do not want to show the progress. 
 */
void SI470X::seek(uint8_t seek_mode, uint8_t direction, void (*showFunc)())
{
    getAllRegisters();
    do
    {
        reg03->refined.TUNE = 1;
        reg02->refined.SEEK = 1; // Enable seek
        reg02->refined.SKMODE = seek_mode;
        reg02->refined.SEEKUP = direction;
        setAllRegisters();
        delay(60);
        if (showFunc != NULL)
        {
            showFunc();
        }
        getStatus();
        this->currentFrequency = getRealFrequency(); // gets the current seek frequency
    } while (reg0a->refined.STC == 0);
    waitAndFinishTune();
    setFrequency(getRealFrequency()); // Fixes station found.
}

/**
 * @ingroup GA03
 * @brief Sets RSSI Seek Threshold
 * @details SEEKTH presents the logarithmic RSSI threshold for the seek operation. 
 * @details The Si4702/03-C19 will not validate channels with RSSI below the SEEKTH value. 
 * @details SEEKTH is one of multiple parameters that can be used to validate channels.
 * @details For more information, see "AN284: Si4700/01 Firmware 15 Seek Adjustability and Set- tings."
 * @param  value between 0 and 127
 */
void SI470X::setSeekThreshold(uint8_t value)
{
    reg05->refined.SEEKTH = value;
    setAllRegisters();
}

/**
 * @ingroup GA03
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
 * @ingroup GA03
 * @brief Sets the FM Space  
 * @details The SI470x device supports 3 different settings as shown below
 * | BAND value     | Description | 
 * | ----------     | ----------- | 
 * |    0           | 00 - 200 kHz (US / Australia, Default) |
 * |    1 (default) | 01 - 100 kHz (Europe / Japan) | 
 * |    2           | 02 - 50 kHz | 
 * |    3           | 03 - Reserved (Do not use)| 
 */
void SI470X::setSpace(uint8_t space)
{
    this->currentFMBand = reg05->refined.SPACE = space;
    setAllRegisters();
}

/**
 * @ingroup GA03
 * @brief Gets the Rssi
 * 
 * @return int 
 */
int SI470X::getRssi()
{
    getStatus();
    return reg0a->refined.RSSI;
}

/**
 * @ingroup GA03
 * @brief Sets the Softmute true or false
 * @param value  TRUE or FALSE
 */
void SI470X::setSoftmute(bool value)
{
    reg02->refined.DSMUTE = !value; // If true, it has not be disabled
    setAllRegisters();
}

/**
 * @ingroup GA03
 * @brief Sets Softmute Attack/Recover Rate.
 * 
 *  | Value | Description |
 *  | ----- | ----------- | 
 *  |   0   | fastest (default) |
 *  |   1   | fast |
 *  |   2   | slow | 
 *  |   3   | slowest |       
 * 
 * @param value See table above
 */
void SI470X::setSoftmuteAttack(uint8_t value)
{
    reg06->refined.SMUTER = value;
    setAllRegisters();
}

/**
 * @ingroup GA03
 * @brief Sets  Softmute Attenuation..
 * 
 *  | Value | Description |
 *  | ----- | ----------- | 
 *  |   0   | 16 dB (default) |
 *  |   1   | 14 dB |
 *  |   2   | 12 dB | 
 *  |   3   | 10 dB |       
 * 
 * @param value See table above
 */
void SI470X::setSoftmuteAttenuation(uint8_t value)
{
    reg06->refined.SMUTEA = value;
    setAllRegisters();
}

/**
 * @ingroup GA03
 * @brief Sets the AGC enable or disable
 * @param value true = enable; fale = disable
 */
void SI470X::setAgc(bool value)
{
    reg04->refined.AGCD = !value;
    setAllRegisters();
}

/**
 * @ingroup GA03
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
 * @ingroup GA03
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
 * @ingroup GA03
 * @brief Checks stereo / mono status
 * 
 * @param value TRUE if stereo
 */
bool SI470X::isStereo()
{
    getStatus();
    return reg0a->refined.ST;
}

/**
 * @ingroup GA03
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
 * @ingroup GA03
 * @brief Gets the current audio volume level
 * 
 * @return uint8_t  0 to 15
 */
uint8_t SI470X::getVolume()
{
    return this->currentVolume;
}

/**
 * @ingroup GA03
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
 * @ingroup GA03
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
 * @ingroup GA03
 * @brief Sets Extended Volume Range.
 * @details This bit attenuates the output by 30 dB. With the bit set to 0, the 15 volume settings adjust the volume between 0 and –28 dBFS. With the bit set to 1, the 15 volume set- tings adjust the volume between –30 and –58 dBFS.
 * @param value false = disabled (default); true = enabled.
 */
void SI470X::setExtendedVolumeRange(bool value)
{
    reg06->refined.VOLEXT = value;
    setAllRegisters();
}

/**
 * @ingroup GA03
 * @brief Gets the Part Number
 * @details If it returns 0x01, so the device is: Si4702/03
 * @return the part number 
 */
uint8_t SI470X::getPartNumber()
{
    return reg00->refined.PN;
}

/**
 * @ingroup GA03
 * @brief Gets the Manufacturer ID
 * @return number  
 */
uint16_t SI470X::getManufacturerId()
{
    return reg00->refined.MFGID;
}

/**
 * @ingroup GA03
 * @brief Gets the Firmware Version
 * @details The return velue before powerup will be 0. After powerup should be 010011 (19)
 * @return number 
 */
uint8_t SI470X::getFirmwareVersion()
{
    return reg01->refined.FIRMWARE;
}

/**
 * @ingroup GA03
 * @brief Gets the Device identification
 * @return number 
 */
uint8_t SI470X::getDeviceId()
{
    return reg01->refined.DEV;
}

/**
 * @ingroup GA03
 * @brief Gets the Chip Version
 * @return number 
 */
uint8_t SI470X::getChipVersion()
{
    return reg01->refined.REV;
}

/**
 * @ingroup GA03
 * @brief Sets De-emphasis.
 * @details 75 μs. Used in USA (default); 50 μs. Used in Europe, Australia, Japan.
 * 
 * @param de  0 = 75 μs; 1 = 50 μs
 */
void SI470X::setFmDeemphasis(uint8_t de)
{
    reg04->refined.DE = de;
    setAllRegisters();
}

/** 
 * @defgroup GA04 RDS Functions
 * @section GA04 RDS/RBDS
 */

/**
 * @ingroup GA04
 * @brief Gets the RDS registers information
 * @details Gets the value of the registers from 0x0A to 0x0F
 * @details This function also updates the value of shadowRegisters[0];
 * @return si470x_reg0a 
 */
void SI470X::getRdsStatus()
{
    word16_to_bytes aux;
    int i;

    Wire.requestFrom(this->deviceAddress, 12);
    delayMicroseconds(300);
    // while (Wire.available() < 32); // It did not work on Attiny Core
    // The registers from 0x0A to 0x0F come first
    for (i = 0x0A; i <= 0x0F; i++)
    {
        aux.refined.highByte = Wire.read();
        aux.refined.lowByte = Wire.read();
        shadowRegisters[i] = aux.raw;
    }
}

/**
 * @ingroup GA04
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
 * @ingroup GA04
 * @brief Sets the RDS operation 
 * @details Enable or Disable the RDS
 * 
 * @param true = turns the RDS ON; false  = turns the RDS OFF
 */
void SI470X::setRds(bool value)
{
    reg04->refined.RDS = value;
    setAllRegisters();
}

/**
 * @ingroup GA04
 * @brief Returns true if RDS Ready
 * @details Read address 0Ah and check the bit RDSR.
 * @details If in verbose mode, the BLERA bits indicate how many errors were corrected in block A. If BLERA indicates 6 or more errors, the data in RDSA should be discarded.
 * @details When using the polling method, it is best not to poll continuously. The data will appear in intervals of ~88 ms and the RDSR indicator will be available for at least 40 ms, so a polling rate of 40 ms or less should be sufficient.
 * @details ATTENTION:  You most call this function before quering other RDS functions. Call it before calling a set of RDS functions. 
 * @return true or false
 */
bool SI470X::getRdsReady()
{
    getStatus();
    return reg0a->refined.RDSR;
};

/**
 * @ingroup GA04
 * 
 * @brief Returns the current Text Flag A/B  
 * @details It is useful to check the flag A/B switching.  
 * @return uint8_t current Text Flag A/B  
 */
uint8_t SI470X::getRdsFlagAB(void)
{
    si470x_rds_blockb blkb;
    blkb.blockB = shadowRegisters[0x0D];
    return blkb.refined.textABFlag;
}

/**
 * @ingroup GA04
 * @brief Return the group type 
 * @details Please, check if getRdsReady was called before.
 * @see getRdsReady
 * @return uint16_t 
 */
uint16_t SI470X::getRdsGroupType()
{
    si470x_rds_blockb blkb;
    blkb.blockB = shadowRegisters[0x0D];
    return blkb.group0.groupType;
}

/**
 * @ingroup GA04
 * @brief Gets the version code (extracted from the Block B)
 * @details Please, check if getRdsReady was called before.
 * @returns  0=A or 1=B
 */
uint8_t SI470X::getRdsVersionCode(void)
{
    si470x_rds_blockb blkb;
    blkb.blockB = shadowRegisters[0x0D];
    return blkb.refined.versionCode;
}

/**  
 * @ingroup GA04  
 * @brief Returns the Program Type (extracted from the Block B)
 * @details Please, check if getRdsReady was called before.
 * @see https://en.wikipedia.org/wiki/Radio_Data_System
 * @return program type (an integer betwenn 0 and 31)
 */
uint8_t SI470X::getRdsProgramType(void)
{
    si470x_rds_blockb blkb;
    blkb.blockB = shadowRegisters[0x0D];
    return blkb.refined.programType;
}

/**
 * @ingroup GA04
 * 
 * @brief Process data received from group 2B
 * @details Please, check if getRdsReady was called before.
 * @param c  char array reference to the "group 2B" text 
 */
void SI470X::getNext2Block(char *c)
{
    word16_to_bytes blk;
    blk.raw = shadowRegisters[REG0F];

    c[1] = blk.refined.lowByte;
    c[0] = blk.refined.highByte;
}

/**
 * @ingroup GA04
 * 
 * @brief Process data received from group 2A
 * @details Please, check if getRdsReady was called before.
 * @param c  char array reference to the "group  2A" text 
 */
void SI470X::getNext4Block(char *c)
{
    word16_to_bytes blk_c, blk_d;

    blk_c.raw = shadowRegisters[REG0E];
    blk_d.raw = shadowRegisters[REG0F];

    c[0] = blk_c.refined.highByte;
    c[1] = blk_c.refined.lowByte;
    c[2] = blk_d.refined.highByte;
    c[3] = blk_d.refined.lowByte;
}

/**
 * @ingroup GA04
 * @brief Gets the RDS Text when the message is of the Group Type 2 version A
 * @details Please, check if getRdsReady was called before.
 * @return char*  The string (char array) with the content (Text) received from group 2A 
 */
char *SI470X::getRdsText(void)
{
    static int rdsTextAdress2A;
    si470x_rds_blockb blkb;

    getRdsStatus();

    blkb.blockB = shadowRegisters[0x0D];
    rdsTextAdress2A = blkb.group2.address;

    if (rdsTextAdress2A >= 16)
        rdsTextAdress2A = 0;

    getNext4Block(&rds_buffer2A[rdsTextAdress2A * 4]);
    rdsTextAdress2A += 4;
    return rds_buffer2A;
}

/**
 * @ingroup GA04
 * @todo RDS Dynamic PS or Scrolling PS support
 * @details Please, check if getRdsReady was called before.
 * @brief Gets the station name and other messages. 
 * 
 * @return char* should return a string with the station name. 
 *         However, some stations send other kind of messages
 */
char *SI470X::getRdsText0A(void)
{
    static int rdsTextAdress0A;
    si470x_rds_blockb blkb;

    getRdsStatus();
    blkb.blockB = shadowRegisters[0x0D];

    if (blkb.group0.groupType == 0)
    {
        // Process group type 0
        rdsTextAdress0A = blkb.group0.address;
        if (rdsTextAdress0A >= 0 && rdsTextAdress0A < 4)
        {
            getNext2Block(&rds_buffer0A[rdsTextAdress0A * 2]);
            rds_buffer0A[8] = '\0';
            return rds_buffer0A;
        }
    }
    return NULL;
}

/**
 * @ingroup @ingroup GA04
 * 
 * @brief Gets the Text processed for the 2A group
 * 
 * @return char* string with the Text of the group A2  
 */
char *SI470X::getRdsText2A(void)
{
    static int rdsTextAdress2A;
    si470x_rds_blockb blkb;

    getRdsStatus();

    blkb.blockB = shadowRegisters[0x0D];
    rdsTextAdress2A = blkb.group2.address;
    if (blkb.group2.groupType == 2)
    {
        // Process group 2A
        // Decode B block information
        if (rdsTextAdress2A >= 0 && rdsTextAdress2A < 16)
        {
            getNext4Block(&rds_buffer2A[rdsTextAdress2A * 4]);
            rds_buffer2A[63] = '\0';
            return rds_buffer2A;
        }
    }
    return NULL;
}

/**
 * @ingroup GA04
 * @brief Gets the Text processed for the 2B group
 * @return char* string with the Text of the group AB  
 */
char *SI470X::getRdsText2B(void)
{
    static int rdsTextAdress2B;
    si470x_rds_blockb blkb;

    getRdsStatus();
    blkb.blockB = shadowRegisters[0x0D];
    if (blkb.group2.groupType == 2)
    {
        // Process group 2B
        rdsTextAdress2B = blkb.group2.address;
        if (rdsTextAdress2B >= 0 && rdsTextAdress2B < 16)
        {
            getNext2Block(&rds_buffer2B[rdsTextAdress2B * 2]);
            return rds_buffer2B;
        }
    }
    return NULL;
}

/**
 * @ingroup GA04 
 * @brief Gets the RDS time and date when the Group type is 4 
 * @return char* a string with hh:mm +/- offset
 */
char *SI470X::getRdsTime()
{
    // Under Test and construction
    // Need to check the Group Type before.
    si470x_rds_date_time dt;
    word16_to_bytes blk_b, blk_c, blk_d;
    si470x_rds_blockb blkb;

    getRdsStatus();

    blk_b.raw = blkb.blockB = shadowRegisters[REG0D];
    blk_c.raw = shadowRegisters[REG0E];
    blk_d.raw = shadowRegisters[REG0F];

    uint16_t minute;
    uint16_t hour;

    if ( blkb.group0.groupType  == 4)
    {
        char offset_sign;
        int offset_h;
        int offset_m;

        // uint16_t y, m, d;

        dt.raw[4] = blk_b.refined.lowByte;
        dt.raw[5] = blk_b.refined.highByte;

        dt.raw[2] = blk_c.refined.lowByte;
        dt.raw[3] = blk_c.refined.highByte;

        dt.raw[0] = blk_d.refined.lowByte;
        dt.raw[1] = blk_d.refined.highByte;

        minute =  dt.refined.minute;
        hour = dt.refined.hour;

        offset_sign = (dt.refined.offset_sense == 1) ? '+' : '-';
        offset_h = (dt.refined.offset * 30) / 60;
        offset_m = (dt.refined.offset * 30) - (offset_h * 60);

        // If wrong time, return NULL
        if (offset_h > 12 || offset_m > 60 || hour > 24 || minute > 60)
            return NULL;

        this->convertToChar(hour, rds_time, 2, 0, ' ', false);
        rds_time[2] = ':';
        this->convertToChar(minute, &rds_time[3], 2, 0, ' ', false);
        rds_time[5] = ' ';
        rds_time[6] = offset_sign;
        this->convertToChar(offset_h, &rds_time[7], 2, 0, ' ', false);
        rds_time[9] = ':';
        this->convertToChar(offset_m, &rds_time[10], 2, 0, ' ', false);
        rds_time[12] = '\0';

        return rds_time;
    }

    return NULL;
}


/**
 * @ingroup GA04
 * @brief Gets the RDS time converted to local time.
 * @details ATTENTION: You must call getRdsReady before calling this function.
 * @details ATTENTION: Some stations broadcast wrong time.
 * @return char* a string with hh:mm
 * @see getRdsReady
 */
char *SI470X::getRdsLocalTime()
{
    // Under Test and construction
    // Need to check the Group Type before.
    si470x_rds_date_time dt;
    word16_to_bytes blk_b, blk_c, blk_d;
    si470x_rds_blockb blkb;

    getRdsStatus();

    blk_b.raw = blkb.blockB = shadowRegisters[REG0D];
    blk_c.raw = shadowRegisters[REG0E];
    blk_d.raw = shadowRegisters[REG0F];

    uint16_t minute;
    uint16_t hour;
    uint16_t localTime;

    if ( blkb.group0.groupType  == 4)
    {
        int offset_h;
        int offset_m;

        dt.raw[4] = blk_b.refined.lowByte;
        dt.raw[5] = blk_b.refined.highByte;

        dt.raw[2] = blk_c.refined.lowByte;
        dt.raw[3] = blk_c.refined.highByte;

        dt.raw[0] = blk_d.refined.lowByte;
        dt.raw[1] = blk_d.refined.highByte;

        minute =  dt.refined.minute;
        hour = dt.refined.hour;

        offset_h = (dt.refined.offset * 30) / 60;
        offset_m = (dt.refined.offset * 30) - (offset_h * 60);

        localTime = (hour * 60 + minute);
        if (dt.refined.offset_sense == 1)
            localTime -= (offset_h * 60 + offset_m);
        else
            localTime += (offset_h * 60 + offset_m);

        hour = localTime / 60;
        minute = localTime - (hour * 60);

        if (hour > 24 || minute > 60)
            return NULL;

        this->convertToChar(hour, rds_time, 2, 0, ' ', false);
        rds_time[2] = ':';
        this->convertToChar(minute, &rds_time[3], 2, 0, ' ', false);
        rds_time[5] = '\0';

        return rds_time;
    }

    return NULL;
}


/**
 * @ingroup GA04
 * @brief Gets Station Name, Station Information, Program Information and utcTime
 * @details This function populates four char pointer variable parameters with Station Name, Station Information, Programa Information and UTC time.
 * @details You must call  setRDS(true), setRdsFifo(true) before calling getRdsAllData(...)
 * @details ATTENTION: the parameters below are point to point to array of char.
 * @details the right way to call this function is shown below.
 * @code {.cpp}
 *
 * char *stationName, *stationInfo, *programInfo, *rdsTime;
 * // The char pointers above will be populate by the call below. So, the char pointers need to be passed by reference (pointer to pointer).
 * if (rx.getRdsAllData(&stationName, &stationInfo , &programInfo, &rdsTime) ) {
 *     showProgramaInfo(programInfo);
 *     showStationName(stationName);
 *     showStationInfo(stationInfo);
 *     showUtcTime(rdsTime);
 * }
 * @endcode
 * @param stationName (reference)  - if NOT NULL,  point to Name of the Station (char array -  9 bytes)
 * @param stationInformation (reference)  - if NOT NULL, point to Station information (char array - 33 bytes)
 * @param programInformation (reference)  - if NOT NULL, point to program information (char array - 65 nytes)
 * @param utcTime  (reference)  - if NOT NULL, point to char array containing the current UTC time (format HH:MM:SS +HH:MM)
 * @return True if found at least one valid data
 * @see setRDS, setRdsFifo, getRdsAllData
 */
bool SI470X::getRdsAllData(char **stationName, char **stationInformation, char **programInformation, char **utcTime)
{

    if (!this->getRdsReady())
        return false;
    *stationName = this->getRdsText0A();        // returns NULL if no information
    *stationInformation = this->getRdsText2B(); // returns NULL if no information
    *programInformation = this->getRdsText2A(); // returns NULL if no information
    *utcTime = this->getRdsTime();              // returns NULL if no information

    return (bool)stationName | (bool)stationInformation | (bool)programInformation | (bool)utcTime;
}


/**
 * @ingroup GA04
 * @brief Clear RDS Information (Station Name, Station Information, Program Information and Time)
 * @details Clear the buffer with latest RDS information
 */
void SI470X::clearRdsBuffer()
{
    memset(rds_buffer0A, 0, sizeof(rds_buffer0A));
    memset(rds_buffer2A, 0, sizeof(rds_buffer2A));
    memset(rds_buffer2B, 0, sizeof(rds_buffer2B));
    memset(rds_time, 0, sizeof(rds_time));
}


/**
 * @ingroup GA04 
 * @brief Get the Rds Sync 
 * @details Returns true if RDS currently synchronized.
 * @return true or false
 */
bool SI470X::getRdsSync()
{
    getStatus();
    return reg0a->refined.RDSS;
}


/**
 * @brief Replace unwanted ASCII character to space
 * @param text char array text to be processed 
 * @param size size of char array
 */
void SI470X::adjustRdsText(char *text, int size) {
    size--;
    for (int i = 0; i < size; i++ ) 
        if ( text[i] < 32 ) text[i] = ' ';
    text[size ] = 0;   // ends the string char array with '\0'; 
}


/**
 * @defgroup GA05 Tools
 * @section GA05 Tools / Helper
 */

/**
 * @ingroup GA05 Check the I2C buss address
 * @brief Check the I2C bus address
 * @details For some reason, the BK1088 device does not work with the standard Wire.h library of Arduino. 
 * @details The checkI2C function is only used to test the circuit. 
 * @details In practice, no function from the Wire.h library is utilized in a real application with the BK1088 in this project. 
 * @param uint8_t address Array - this array will be populated with the I2C bus addresses found (minimum three elements)
 * @return 0 if no i2c device is found; -1 if error is found or n > 0, where n is the number of I2C bus address found
 */
int SI470X::checkI2C(uint8_t *addressArray)
{
    Wire.begin();
    int error, address;
    int idx = 0;
    for (address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0)
        {
            addressArray[idx] = address;
            idx++;
        }
        else if (error == 4)
            return -1;
    }
    Wire.end();
    delay(200);
    return idx;
}

/**
 * @ingroup GA05 Covert numbers to char array
 * @brief Converts a number to a char array
 * @details It is useful to mitigate memory space used by functions like sprintf or othetr generic similar functions
 * @details You can use it to format frequency using decimal or tousand separator and also to convert smalm numbers.
 *
 * @param value  value to be converted
 * @param strValue char array that will be receive the converted value
 * @param len final string size (in bytes)
 * @param dot the decimal or tousand separator position
 * @param separator symbol "." or ","
 * @param remove_leading_zeros if true removes up to two leading zeros (default is true)
 */
void SI470X::convertToChar(uint16_t value, char *strValue, uint8_t len, uint8_t dot, uint8_t separator, bool remove_leading_zeros)
{
    char d;
    for (int i = (len - 1); i >= 0; i--)
    {
        d = value % 10;
        value = value / 10;
        strValue[i] = d + 48;
    }
    strValue[len] = '\0';
    if (dot > 0)
    {
        for (int i = len; i >= dot; i--)
        {
            strValue[i + 1] = strValue[i];
        }
        strValue[dot] = separator;
    }

    if (remove_leading_zeros)
    {
        if (strValue[0] == '0')
        {
            strValue[0] = ' ';
            if (strValue[1] == '0')
                strValue[1] = ' ';
        }
    }
}



