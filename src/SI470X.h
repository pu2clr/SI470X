
/**
 * @mainpage PU2CLR SI470X Arduino Library
 * @brief PU2CLR SI470X Arduino Library implementation. <br>
 * @details This is an Arduino library for the SI4702 and SI4703, BROADCAST RECEIVER.<br>
 * @details It works with I2C protocol and can provide an easier interface for controlling the SI4702/03 devices.<br>
 *
 * This library can be freely distributed using the MIT Free Software model.
 * Copyright (c) 2020 Ricardo Lima Caratti.
 * Contact: pu2clr@gmail.com
 */

#include <Arduino.h>
#include <Wire.h>

#define MAX_DELAY_AFTER_OSCILLATOR 500 // Max delay after the crystal oscilator becomes active

#define I2C_DEVICE_ADDR 0x10
#define OSCILLATOR_TYPE_CRYSTAL 1 // Crystal
#define OSCILLATOR_TYPE_REFCLK 0  // Reference clock

#define RDS_STANDARD 0     //!< RDS Mode.
#define RDS_VERBOSE 1      //!< RDS Mode.
#define SI470X_SEEK_DOWN 0 //!< Seek Down  Direction
#define SI470X_SEEK_UP 1   //!< Seek Up  Direction
#define SI470X_SEEK_WRAP 0 //
#define SI470X_SEEK_STOP 1

#define FM_BAND_USA_EU 0     //!< 87.5–108 MHz (US / Europe, Default)
#define FM_BAND_JAPAN_WIDE 1 //!< 76–108 MHz (Japan wide band)
#define FM_BAND_JAPAN 2      //!< 76–90 MHz (Japan)
#define FM_BAND_RESERVED 3   //!< Reserved

#define REG00 0x00
#define REG01 0x01
#define REG02 0x02
#define REG03 0x03
#define REG04 0x04
#define REG05 0x05
#define REG06 0x06
#define REG07 0x07
#define REG08 0x08
#define REG09 0x09
#define REG0A 0x0A
#define REG0B 0x0B
#define REG0C 0x0C
#define REG0D 0x0D
#define REG0E 0x0E
#define REG0F 0x0F

/**
 * @defgroup GA01 Union, Structure and Defined Data Types
 * @brief   SI470X Defined Data Types
 * @details Defined Data Types is a way to represent the SI470X registers information
 * @details Some information appears to be inaccurate due to translation problems from Chinese to English.
 * @details The information shown here was extracted from Datasheet:
 * @details SI470X stereo FM digital tuning radio documentation.
 * @details Other information seems incomplete even in the original Chinese Datasheet.
 * @details For example: Reg 10 (0x0A). There is no information about it. The Reg11 and 12 seem wrong
 */

/**
 * @ingroup GA01
 * @brief Device ID
 *
 */
typedef union
{
    struct
    {
        uint16_t MFGID : 12; //!< Manufacturer ID.
        uint16_t PN : 4;     //!< Part Number.
    } refined;
    uint16_t raw;
} si470x_reg00;

/**
 * @ingroup GA01
 * @brief Chip ID
 *
 */
typedef union
{
    struct
    {
        uint16_t FIRMWARE : 6; //!< Firmware Version.
        uint16_t DEV : 4;      //!< 0 before powerup; 0001 after powerup = Si4702; 1000 before powerup = Si4703; 1001 after powerup = Si4703.
        uint16_t REV : 6;      //!< Chip Version; 0x04 = Rev C
    } refined;
    uint16_t raw;
} si470x_reg01;

/**
 * @ingroup GA01
 * @brief Power Configuratio
 *
 */
typedef union
{
    struct
    {
        uint8_t ENABLE : 1;    //!< Powerup Enable; Refer to “4.9. Reset, Powerup, and Powerdown”; Default = 0.
        uint8_t RESERVED1 : 5; //!< Reserved; Always write to 0.
        uint8_t DISABLE : 1;   //!< Powerup Disable; Refer to “4.9. Reset, Powerup, and Powerdown”; Default = 0.
        uint8_t RESERVED2 : 1; //!< Reserved; Always write to 0.
        uint8_t SEEK : 1;      //!< 0 = Disable (default); 1 = Enable;
        uint8_t SEEKUP : 1;    //!< Seek Direction; 0 = Seek down (default); 1 = Seek up.
        uint8_t SKMODE : 1;    //!< Seek Mode; 0 = Wrap at the upper or lower band limit and continue seeking (default); 1 = Stop seeking at the upper or lower band limit.
        uint8_t RDSM : 1;      //!< RDS Mode; 0 = Standard (default); 1 = Verbose; Refer to “4.4. RDS/RBDS Processor and Functionality”.
        uint8_t RESERVED3 : 1; //!< Reserved; Always write to 0.
        uint8_t MONO : 1;      //!< Mono Select; 0 = Stereo (default); 1 = Force mono.
        uint8_t DMUTE : 1;     //!< Mute Disable; 0 = Mute enable (default); 1 = Mute disable.
        uint8_t DSMUTE : 1;    //!< Softmute Disable; 0 = Softmute enable (default); 1 = Softmute disable.
    } refined;
    uint16_t raw;
} si470x_reg02;

/**
 * @ingroup GA01
 * @brief Channe
 * @details Channel value for tune operation. If BAND 05h[7:6] = 00, then Freq (MHz) = Spacing (MHz) x Channel + 87.5 MHz.
 * @details If BAND 05h[7:6] = 01, BAND 05h[7:6] = 10, then Freq (MHz) = Spacing (MHz) x Channel + 76 MHz.
 * @details CHAN[9:0] is not updated during a seek operation. READCHAN[9:0] provides the current tuned channel and is updated during a seek operation and after a seek or tune operation completes. Channel spacing is set with the bits SPACE 05h[5:4].
 * @details The tune operation begins when the TUNE bit is set high. The STC bit is set high when the tune operation completes. The STC bit must be set low by setting the TUNE bit low before the next tune or seek may begin.
 */
typedef union
{
    struct
    {
        uint16_t CHAN : 10;    //!< Channel Select;
        uint16_t RESERVED : 5; //!< Reserved; Always write to 0;
        uint16_t TUNE : 1;     //!< Tune. 0 = Disable (default); 1 = Enable.
    } refined;
    uint16_t raw;
} si470x_reg03;

/**
 * @ingroup GA01
 * @brief  System Configuration 1
 * @details Setting STCIEN = 1 will generate a 5 ms low pulse on GPIO2 when the STC 0Ah[14] bit is set.
 * @details Setting RDSIEN = 1 will generate a 5 ms low pulse on GPIO2 when the RDSR 0Ah[15] bit is set.
 * @details Setting STCIEN = 1 and GPIO2[1:0] = 01 will generate a 5 ms low pulse on GPIO2 when the STC 0Ah[14] bit is set.
 * | BLNDADJ value | Description |
 * | ------------  | ----------- |
 * |      0        | 31–49 RSSI dBμV (default) |
 * |      1        | 37–55 RSSI dBμV (+6 dB) |
 * |      2        | 19–37 RSSI dBμV (–12 dB) |
 * |      3        | 25–43 RSSI dBμV (–6 dB) |
 *
 */
typedef union
{
    struct
    {
        uint8_t GPIO1 : 2;     //!< General Purpose I/O 1; 00 = High impedance (default); 01 = Reserved; 10 = Low; 11 = High.
        uint8_t GPIO2 : 2;     //!< General Purpose I/O 2. 00 = High impedance (default); 01 = Reserved; 10 = Low; 11 = High.
        uint8_t GPIO3 : 2;     //!< General Purpose I/O 2. 00 = High impedance (default); 01 = Reserved; 10 = Low; 11 = High.
        uint8_t BLNDADJ : 2;   //!< Stereo/Mono Blend Level Adjustment. Sets the RSSI range for stereo/mono blend. See table above.
        uint8_t RESERVED1 : 2; //!< Reserved; Always write to 0.
        uint8_t AGCD : 1;      //!< AGC Disable; 0 = AGC enable (default); 1 = AGC disable.
        uint8_t DE : 1;        //!< De-emphasis; 0 = 75 μs. Used in USA (default); 1 = 50 μs. Used in Europe, Australia, Japan.
        uint8_t RDS : 1;       //!< RDS Enable; 0 = Disable (default); 1 = Enable.
        uint8_t RESERVED2 : 1; //!< Reserved; Always write to 0.
        uint8_t STCIEN : 1;    //!< Seek/Tune Complete Interrupt Enable; 0 = Disable Interrupt (default); 1 = Enable Interrupt.
        uint8_t RDSIEN : 1;    //!< RDS Interrupt Enable; 0 = Disable Interrupt (default); 1 = Enable Interrupt.
    } refined;
    uint16_t raw;
} si470x_reg04;

/**
 * @ingroup GA01
 * @brief System Configuration 2
 * @details SEEKTH presents the logarithmic RSSI threshold for the seek operation. The Si4702/03-C19 will not validate channels with RSSI below the SEEKTH value. SEEKTH is one of multiple parameters that can be used to validate channels. For more information, see "AN284: Si4700/01 Firmware 15 Seek Adjustability and Set- tings."
 *
 * | BAND value | Description |
 * | ---------- | ----------- |
 * |    0       | 00 = 87.5–108 MHz (USA, Europe) (Default) |
 * |    1       | 01 = 76–108 MHz (Japan wide band) |
 * |    2       | 10 = 76–90 MHz (Japan) |
 * |    3       | 11 = Reserved |
 */
typedef union
{
    struct
    {
        uint8_t VOLUME : 4; //!< Relative value of volume is shifted –30 dBFS with the VOLEXT 06h[8] bit; 0000 = mute (default);
        uint8_t SPACE : 2;  //!< Channel Spacing; 00 = 200 kHz (USA, Australia) (default); 01 = 100 kHz (Europe, Japan); 10 = 50 kHz.
        uint8_t BAND : 2;   //!< Band Select. See table above.
        uint8_t SEEKTH;     //!< RSSI Seek Threshold. 0x00 = min RSSI (default); 0x7F = max RSSI.
    } refined;
    uint16_t raw;
} si470x_reg05;

/**
 * @ingroup GA01
 * @brief  System Configuration 3
 * @details The VOLEXT  bit attenuates the output by 30 dB. With the bit set to 0, the 15 volume settings adjust the volume between 0 and –28 dBFS. With the bit set to 1, the 15 volume set- tings adjust the volume between –30 and –58 dBFS.
 * @details Refer to 4.5. "Stereo Audio Processing" on page 16.
 *
 * | Softmute Attenuation   | Description |
 * |            0           | 16 dB (default) |
 * |            1           | 14 dB |
 * |            2           | 12 dB |
 * |            3           | 10 dB |
 *
 * | Softmute Attack/Recover Rate   | Description |
 * |            0           | fastest (default) |
 * |            1           | fast |
 * |            2           | slow |
 * |            3           | slowest |
 *
 */
typedef union
{
    struct
    {
        uint8_t SKCNT : 4;    //!< Seek FM Impulse Detection Threshold; 0000 = disabled (default); 0001 = max (most stops); 1111 = min (fewest stops). Allowable number of FM impulses for a valid seek channel.
        uint8_t SKSNR : 4;    //!< Seek SNR Threshold; 0000 = disabled (default); 0001 = min (most stops); 0111 = max (fewest stops); Required channel SNR for a valid seek channel.
        uint8_t VOLEXT : 1;   //!< Extended Volume Range; 0 = disabled (default); 1 = enabled.
        uint8_t RESERVED : 3; //!< Reserved; Always write to zero.
        uint8_t SMUTEA : 2;   //!< Softmute Attenuation; See table above.
        uint8_t SMUTER : 2;   //!< Softmute Attack/Recover Rate; See table above
    } refined;
    uint16_t raw;
} si470x_reg06;

/**
 * @ingroup GA01
 * @brief Test 1
 * @details Setting AHIZEN maintains a dc bias of 0.5 x VIO on the LOUT and ROUT pins to pre- vent the ESD diodes from clamping to the VIO or GND rail in response to the output swing of another device.
 * @details Register 07h containing the AHIZEN bit must not be written during the powerup sequence and high-Z only takes effect when in powerdown and VIO is supplied. Bits 13:0 of register 07h must be preserved as 0x0100 while in pow- erdown and as 0x3C04 while in powerup.
 * @details The internal crystal oscillator requires an external 32.768 kHz crystal as shown in "Typical Application Schematic" on page 14.
 * @details The oscillator must be enabled before powerup (ENABLE = 1) as shown in Figure 9, “Initialization Sequence,” on page 21. It should only be disabled after powerdown (ENABLE = 0).
 * @details Bits 13:0 of register 07h must be preserved as 0x0100 while in powerdown and as 0x3C04 while in powerup. Refer to Si4702/03 Internal Crystal Oscillator Errata.
 */
typedef union
{
    struct
    {
        uint16_t RESERVED : 14; //!< Reserved; If written, these bits should be read first and then written with their pre-existing val- ues. Do not write during powerup.
        uint16_t AHIZEN : 1;    //!< Audio High-Z Enable; 0 = Disable (default); 1 = Enable.
        uint16_t XOSCEN : 1;    //!< Crystal Oscillator Enable; 0 = Disable (default); 1 = Enable.
    } refined;
    uint16_t raw;
} si470x_reg07;

/**
 * @ingroup GA01
 * @brief Test 2
 * @details If written, these bits should be read first and then written with their pre-existing val- ues. Do not write during powerup.
 */
typedef union
{
    struct
    {
        uint8_t lowByte;  //!< Reserved
        uint8_t highByte; //!< Reserved
    } refined;
    uint16_t raw;
} si470x_reg08;

/**
 * @ingroup GA01
 * @brief Boot Configuration
 * @details If written, these bits should be read first and then written with their pre-existing val- ues. Do not write during powerup.
 */
typedef union
{
    struct
    {
        uint8_t lowByte;  //!< Reserved
        uint8_t highByte; //!< Reserved
    } refined;
    uint16_t raw;
} si470x_reg09;

/**
 * @ingroup GA01
 * @brief Status RSSI
 * @details RSSI is measured units of dBμV in 1 dB increments with a maximum of approximately 75 dBμV. Si4702/03-C19 does not report RSSI levels greater than 75 dBuV.
 * @details AFCRL is updated after a tune or seek operation completes and indicates a valid or invalid channel. During normal operation, AFCRL is updated to reflect changing RF envi- ronments.
 * @details The SF/BL flag is set high when SKMODE 02h[10] = 0 and the seek operation fails to find a channel qualified as valid according to the seek parameters.
 * @details The SF/BL flag is set high when SKMODE 02h[10] = 1 and the upper or lower band limit has been reached. The SEEK 02h[8] bit must be set low to clear SF/BL.
 * @details The seek/tune complete flag is set when the seek or tune operation completes. Setting the SEEK 02h[8] or TUNE 03h[15] bit low will clear STC.
 *
 * | RDS Block A Errors | Description |
 * | ------------------ | ----------- |
 * |          0         | 0 errors requiring correction |
 * |          1         | 1–2 errors requiring correction |
 * |          2         | 3–5 errors requiring correction |
 * |          3         | 6+ errors or error in checkword, correction not possible |
 *
 */
typedef union
{
    struct
    {
        uint8_t RSSI;      //!< RSSI (Received Signal Strength Indicator).
        uint8_t ST : 1;    //!< Stereo Indicator; 0 = Mono; 1 = Stereo.
        uint8_t BLERA : 2; //!< RDS Block A Errors; See table above.
        uint8_t RDSS : 1;  //!< RDS Synchronized; 0 = RDS decoder not synchronized (default); 1 = RDS decoder synchronized.
        uint8_t AFCRL : 1; //!< AFC Rail; 0 = AFC not railed; 1 = AFC railed, indicating an invalid channel. Audio output is softmuted when set.
        uint8_t SF_BL : 1; //!< Seek Fail/Band Limit; 0 = Seek successful; 1 = Seek failure/Band limit reached.
        uint8_t STC : 1;   //!< Seek/Tune Complete; 0 = Not complete (default); 1 = Complete.
        uint8_t RDSR : 1;  //!< RDS Ready; 0 = No RDS group ready (default); 1 = New RDS group ready.
    } refined;
    uint16_t raw;
} si470x_reg0a;

/**
 * @ingroup GA01
 * @brief Read Channel
 * @details If BAND 05h[7:6] = 00, then Freq (MHz) = Spacing (MHz) x Channel + 87.5 MHz. If BAND 05h[7:6] = 01, BAND 05h[7:6] = 10, then Freq (MHz) = Spacing (MHz) x Channel + 76 MHz.
 * @details READCHAN[9:0] provides the current tuned channel and is updated during a seek operation and after a seek or tune operation completes. Spacing and channel are set with the bits SPACE 05h[5:4] and CHAN 03h[9:0].
 *
 * | RDS block Errors | Description |
 * | -----------------| ----------- |
 * |          0       | 0 errors requiring correction |
 * |          1       | 1–2 errors requiring correction |
 * |          2       | 3–5 errors requiring correction |
 * |          3       | 6+ errors or error in checkword, correction not possible |
 *
 */
typedef union
{
    struct
    {
        uint16_t READCHAN : 10; //!< Read Channel.
        uint16_t BLERD : 2;     //!< RDS Block D Errors. See table above.
        uint16_t BLERC : 2;     //!< RDS Block C Errors. See table above.
        uint16_t BLERB : 2;     //!< RDS Block B Errors. See table above.
    } refined;
    uint16_t raw;
} si470x_reg0b;

/**
 * @ingroup GA01
 * @brief RDS Block A
 *
 */
typedef uint16_t si470x_reg0c; //!< RDS Block A Data.

/**
 * @ingroup GA01
 * @brief RDS Block B
 *
 */
typedef uint16_t si470x_reg0d; //!< RDS Block B Data.

/**
 * @ingroup GA01
 * @brief RDS Block C
 *
 */
typedef uint16_t si470x_reg0e; //!< RDS Block C Data.

/**
 * @ingroup GA01
 * @brief RDS Block D
 *
 */
typedef uint16_t si470x_reg0f; //!< RDS Block D Data.

/**
 * @ingroup GA01
 * @brief RDS Block B data type
 *
 * @details For GCC on System-V ABI on 386-compatible (32-bit processors), the following stands:
 *
 * 1) Bit-fields are allocated from right to left (least to most significant).
 * 2) A bit-field must entirely reside in a storage unit appropriate for its declared type.
 *    Thus a bit-field never crosses its unit boundary.
 * 3) Bit-fields may share a storage unit with other struct/union members, including members that are not bit-fields.
 *    Of course, struct members occupy different parts of the storage unit.
 * 4) Unnamed bit-fields' types do not affect the alignment of a structure or union, although individual
 *    bit-fields' member offsets obey the alignment constraints.
 *
 * @see also https://en.wikipedia.org/wiki/Radio_Data_System
 */
typedef union
{
    struct
    {
        uint16_t address : 2;            // Depends on Group Type and Version codes. If 0A or 0B it is the Text Segment Address.
        uint16_t DI : 1;                 // Decoder Controll bit
        uint16_t MS : 1;                 // Music/Speech
        uint16_t TA : 1;                 // Traffic Announcement
        uint16_t programType : 5;        // PTY (Program Type) code
        uint16_t trafficProgramCode : 1; // (TP) => 0 = No Traffic Alerts; 1 = Station gives Traffic Alerts
        uint16_t versionCode : 1;        // (B0) => 0=A; 1=B
        uint16_t groupType : 4;          // Group Type code.
    } group0;
    struct
    {
        uint16_t address : 4;            // Depends on Group Type and Version codes. If 2A or 2B it is the Text Segment Address.
        uint16_t textABFlag : 1;         // Do something if it chanhes from binary "0" to binary "1" or vice-versa
        uint16_t programType : 5;        // PTY (Program Type) code
        uint16_t trafficProgramCode : 1; // (TP) => 0 = No Traffic Alerts; 1 = Station gives Traffic Alerts
        uint16_t versionCode : 1;        // (B0) => 0=A; 1=B
        uint16_t groupType : 4;          // Group Type code.
    } group2;
    struct
    {
        uint16_t content : 4;            // Depends on Group Type and Version codes.
        uint16_t textABFlag : 1;         // Do something if it chanhes from binary "0" to binary "1" or vice-versa
        uint16_t programType : 5;        // PTY (Program Type) code
        uint16_t trafficProgramCode : 1; // (TP) => 0 = No Traffic Alerts; 1 = Station gives Traffic Alerts
        uint16_t versionCode : 1;        // (B0) => 0=A; 1=B
        uint16_t groupType : 4;          // Group Type code.
    } refined;
    si470x_reg0d blockB;
} si470x_rds_blockb;

/**
 * @ingroup GA01
 * Group RDS type 4A ( RDS Date and Time)
 * When group type 4A is used by the station, it shall be transmitted every minute according to EN 50067.
 * This Structure uses blocks 2,3 and 5 (B,C,D)
 *
 * ATTENTION:
 * To make it compatible with 8, 16 and 32 bits platforms and avoid Crosses boundary, it was necessary to
 * use uint32_t data type.
 */
typedef union
{
    struct
    {
        uint32_t offset : 5;       // Local Time Offset
        uint32_t offset_sense : 1; // Local Offset Sign ( 0 = + , 1 = - )
        uint32_t minute : 6;       // UTC Minutes - 2 bits less significant (void “Crosses boundary”).
        uint32_t hour : 5;         // UTC Hours - 4 bits less significant (void “Crosses boundary”)
        uint32_t mjd : 17;         // Modified Julian Day Code
    } refined;
    uint8_t raw[6];
} si470x_rds_date_time;

/**
 * @ingroup GA01
 * @brief Converts 16 bits word to two bytes
 */
typedef union
{
    struct
    {
        uint8_t lowByte;
        uint8_t highByte;
    } refined;
    uint16_t raw;
} word16_to_bytes;

/**
 * @ingroup GA01
 * @brief KT0915 Class
 * @details This class implements all functions that will help you to control the KT0915 devices.
 *
 * @author PU2CLR - Ricardo Lima Caratti
 */
class SI470X
{

private:
    uint16_t shadowRegisters[17]; //!< shadow registers

    // Device registers map - References to the shadow registers
    si470x_reg00 *reg00 = (si470x_reg00 *)&shadowRegisters[REG00];
    si470x_reg01 *reg01 = (si470x_reg01 *)&shadowRegisters[REG01];
    si470x_reg02 *reg02 = (si470x_reg02 *)&shadowRegisters[REG02];
    si470x_reg03 *reg03 = (si470x_reg03 *)&shadowRegisters[REG03];
    si470x_reg04 *reg04 = (si470x_reg04 *)&shadowRegisters[REG04];
    si470x_reg05 *reg05 = (si470x_reg05 *)&shadowRegisters[REG05];
    si470x_reg06 *reg06 = (si470x_reg06 *)&shadowRegisters[REG06];
    si470x_reg07 *reg07 = (si470x_reg07 *)&shadowRegisters[REG07];
    si470x_reg08 *reg08 = (si470x_reg08 *)&shadowRegisters[REG08];
    si470x_reg09 *reg09 = (si470x_reg09 *)&shadowRegisters[REG09];
    si470x_reg0a *reg0a = (si470x_reg0a *)&shadowRegisters[REG0A];
    si470x_reg0b *reg0b = (si470x_reg0b *)&shadowRegisters[REG0B];
    si470x_reg0c *reg0c = (si470x_reg0c *)&shadowRegisters[REG0C];
    si470x_reg0d *reg0d = (si470x_reg0d *)&shadowRegisters[REG0D];
    si470x_reg0e *reg0e = (si470x_reg0e *)&shadowRegisters[REG0E];
    si470x_reg0f *reg0f = (si470x_reg0f *)&shadowRegisters[REG0F];

    uint16_t startBand[4] = {8750, 7600, 7600, 6400};  //!< Start FM band limit
    uint16_t endBand[4] = {10800, 10800, 9000, 10800}; //!< End FM band limit
    uint16_t fmSpace[4] = {20, 10, 5, 1};              //!< FM channel space

protected:
    char rds_buffer2A[65]; //!<  RDS Radio Text buffer - Program Information
    char rds_buffer2B[33]; //!<  RDS Radio Text buffer - Station Informaation
    char rds_buffer0A[9];  //!<  RDS Basic tuning and switching information (Type 0 groups)
    char rds_time[20];     //!<  RDS date time received information

    int deviceAddress = I2C_DEVICE_ADDR;
    int resetPin;
    uint16_t currentFrequency;
    uint8_t currentFMBand = 0;
    uint8_t currentFMSpace = 0;
    uint8_t currentVolume = 0;
    int rdsInterruptPin = -1;
    int seekInterruptPin = -1;
    int oscillatorType = OSCILLATOR_TYPE_CRYSTAL;
    uint16_t maxDelayAftarCrystalOn = MAX_DELAY_AFTER_OSCILLATOR;

    char strFrequency[8]; // Used to store formated frequency
    
    void reset();
    void powerUp();
    void powerDown();
    void waitAndFinishTune();

public:
    /**
     * @ingroup GA03
     * @brief Sets the I2C bus address
     * @details This function must to be called before setup function if your device are not using 0x10 (default)
     * @param bus_addr I2C buss address
     */
    inline void setI2CAddress(int bus_addr) { this->deviceAddress = bus_addr; };

    /**
     * @ingroup GA03
     * @brief Set the Delay After Crystal On (default 500ms)
     *
     * @param ms_value  Value in milliseconds
     */
    inline void setDelayAfterCrystalOn(uint8_t ms_value) { maxDelayAftarCrystalOn = ms_value; };
    void getAllRegisters();
    void setAllRegisters(uint8_t limit = 0x07);
    void getStatus();

    /**
     * @ingroup GA03
     * @brief Get the Shadown Register object
     * @details if you want to get the current value of the device register, call getAllRegisters() before calling this function.
     * @details if you are dealing with the status register (0x0A), you can call getStatus() instead getAllRegisters().
     * @see setAllRegisters, getAllRegisters, getShadownRegister, getStatus
     * @param register_number
     * @return  16 bits word with the Shadown registert
     */
    inline uint16_t getShadownRegister(uint8_t register_number) { return shadowRegisters[register_number]; };

    /**
     * @ingroup GA03
     * @brief Sets a given value to the Shadown Register
     * @details You have to call setAllRegisters() after setting the Shadow Registers to store the value into the device.
     * @see setAllRegisters, getAllRegisters, getShadownRegister, getStatus
     * @param register_number  register index (from 0x00 to 0x0F)
     * @param value            16 bits word with the content of the register
     */
    void setShadownRegister(uint8_t register_number, uint16_t value)
    {
        if (register_number > 0x0F)
            return;
        shadowRegisters[register_number] = value;
    };

    void setup(int resetPin, int sdaPin, int rdsInterruptPin = -1, int seekInterruptPin = -1, uint8_t oscillator_type = OSCILLATOR_TYPE_CRYSTAL);
    void setup(int resetPin, int sdaPin, uint8_t oscillator_type);
    // void setupDebug(int resetPin, int sdaPin, int rdsInterruptPin, int seekInterruptPin, uint8_t oscillator_type, void (*showFunc)(byte v));
    void setFrequency(uint16_t frequency);
    void setFrequencyUp();
    void setFrequencyDown();
    uint16_t getFrequency();
    uint16_t getRealFrequency();
    uint16_t getRealChannel();
    void setChannel(uint16_t channel);
    void seek(uint8_t seek_mode, uint8_t direction);
    void seek(uint8_t seek_mode, uint8_t direction, void (*showFunc)());
    void setSeekThreshold(uint8_t value);

    void setBand(uint8_t band = 1);
    void setSpace(uint8_t space = 0);
    int getRssi();

    void setSoftmute(bool value);
    void setSoftmuteAttack(uint8_t value);
    void setSoftmuteAttenuation(uint8_t value);
    void setAgc(bool value);

    void setMono(bool value);

    bool isStereo();

    uint8_t getPartNumber();
    uint16_t getManufacturerId();
    uint8_t getFirmwareVersion();
    uint8_t getDeviceId();
    uint8_t getChipVersion();

    void setMute(bool value);
    /**
     * @ingroup GA03
     * @brief Gets Audio Mute Status
     * @return True if muted
     */
    inline bool isMuted() {return reg02->refined.DMUTE;};
    void setVolume(uint8_t value);
    uint8_t getVolume();
    void setVolumeUp();
    void setVolumeDown();
    void setExtendedVolumeRange(bool value);

    void setFmDeemphasis(uint8_t de);

    void getRdsStatus();
    void setRdsMode(uint8_t rds_mode = 0);
    void setRds(bool value);
    inline void setRDS(bool value) { setRds(value); };
    bool getRdsReady();
    bool getRdsAllData(char **stationName, char **stationInformation, char **programInformation, char **utcTime);
    uint8_t getRdsFlagAB(void);
    uint8_t getRdsVersionCode(void);
    uint16_t getRdsGroupType();
    uint8_t getRdsProgramType(void);
    void getNext2Block(char *c);
    void getNext4Block(char *c);
    char *getRdsText(void);
    char *getRdsText0A(void);

    /**
     * @ingroup @ingroup GA04
     * @brief Gets the Program Information (RT - Radio Text)
     * @details Process the program information data. Same getRdsText2A(). It is a alias for getRdsText2A.
     * @details ATTENTION: You must call getRdsReady before calling this function.
     * @return char array with the program information (63 bytes)
     * @see getRdsText2A
     */
    inline char *getRdsProgramInformation(void) { return getRdsText2A(); };

    /**
     * @ingroup GA04
     * @brief Gets the Station Information.
     * @details ATTENTION: You must call getRdsReady before calling this function.
     * @return char array with the Text of Station Information (33 bytes)
     * @see getRdsReady
     */
    inline char *getRdsStationInformation(void) { return getRdsText2B(); };

    /**
     * @ingroup GA04
     * @brief Gets the Station Name
     * @details Alias for getRdsText0A
     * @details ATTENTION: You must call getRdsReady before calling this function.
     * @return char* should return a string with the station name. However, some stations send other kind of messages
     * @see getRdsText0A
     */
    inline char *getRdsStationName(void) { return getRdsText0A(); };

    char *getRdsText2A(void);
    char *getRdsText2B(void);
    char *getRdsTime();
    char *getRdsLocalTime();
    bool getRdsSync();
    void clearRdsBuffer();
    void adjustRdsText(char *text, int size);

    // Tools / Helper functions
    int checkI2C(uint8_t *addressArray);
    void convertToChar(uint16_t value, char *strValue, uint8_t len, uint8_t dot, uint8_t separator, bool remove_leading_zeros = true);

    /**
     * @ingroup G05 Format the Frequency
     * @brief Convert the current frequency to a formated string (char *) frequency
     * @details The current frequency is the latest setted frequency by setFrequency, seek, setFrequencyUp and setFrequencyDown.
     * @param char decimalSeparator - the symbol that separates the decimal part (Exe: . or ,)
     * @return point char string strFrequency (member variable)
     * @see setFrequency, seek, setFrequencyUp and setFrequencyDown
     */
    inline char *formatCurrentFrequency(char decimalSeparator = ',')
    {
        this->convertToChar(this->currentFrequency, this->strFrequency, 5, 3, decimalSeparator, true);
        return this->strFrequency;
    };
};