# ATTiny84 and ATTiny85 setup

It is possible to use the Arduino IDE platform with the PU2CLR SI470X Arduino Library to implement receivers with the ATTiny84 and ATTiny85. You will find details on how to do this below.


## Programming Interface (USBAsp 6 pin)

The table below shows the pin functions of the ATTiny84 and ATTiny85  needed to upload sketch via USBAsp interface.


| Function / USBAsp   | ATTiny85 Pin |  ATTiny84 Pin | 
| ------------------- | ------------ | ------------- |
| RESET               | 1 (PB5)      | 4 (PB3)       |   
| MOSI                | 5 (PB0)      | 7 (PA6)       |
| MISO                | 6 (PB1)      | 8 (PA5)       |
| SCK                 | 7 (PB2)      | 9 (PA4)       |
| GND                 | 4            | 14            |
| VCC                 | 8            | 1             |




## ATTiny84 wireup

| SI4703 pin      | ATTiny84 REF pin | Physical pin  | 
| ----------------| -----------------| ------------- | 
| SEEK_UP         |     3            |    10         | 
| SEEK_DOWN       |     5            |     8         |
| ENCODER_PIN_A   |     0            |    13         |
| ENCODER_PIN_B   |     1            |    12         |  
| SDIO / SDA      |     SDA          |     7         |
| SCLK / CLK      |     SCL          |     9         |
| RESET /RST      |     7            |     6         |



### ATTiny84 pinout

![ATTiny84 pinout](../../extras/images/attiny84.png)


### ATTiny84 schematic

![ATTiny84 schematic](../../extras/images/circuit_attiny84.png)


## ATTiny85 and SI4703 wireup  

| SI4703 pin    | ATTiny85 REF pin | Physical pin  | 
| --------------| -----------------| ------------- | 
| SEEK_UP       |     PB1          |     6         | 
| SEEK_DOWN     |     PB4          |     3         |
| RESET / RST   |     PB3          |     2         | 
| SDIO / SDA    |     SDA          |     5         |
| SCLK / CLK    |     SCL          |     7         |
   
<BR>

## ATTiny85 pinout

![Schematic - ATTiny85 pinout](../../extras/images/attiny85_pinout.jpg)



## ATTiny85 schematic

![ATTiny85 schematic](../../extras/images/circuit_attiny85.png)


