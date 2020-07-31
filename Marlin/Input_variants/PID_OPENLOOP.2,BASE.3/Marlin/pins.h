
#define PINS_H 

/****************************************************************************************
* Arduino pin assignment
*
*                  ATMega168
*                   +-\/-+
*             PC6  1|    |28  PC5 (AI 5 / D19)
*       (D 0) PD0  2|    |27  PC4 (AI 4 / D18)
*       (D 1) PD1  3|    |26  PC3 (AI 3 / D17)
*       (D 2) PD2  4|    |25  PC2 (AI 2 / D16)
*  PWM+ (D 3) PD3  5|    |24  PC1 (AI 1 / D15)
*       (D 4) PD4  6|    |23  PC0 (AI 0 / D14)
*             VCC  7|    |22  GND
*             GND  8|    |21  AREF
*             PB6  9|    |20  AVCC
*             PB7 10|    |19  PB5 (D 13)
*  PWM+ (D 5) PD5 11|    |18  PB4 (D 12)
*  PWM+ (D 6) PD6 12|    |17  PB3 (D 11) PWM
*       (D 7) PD7 13|    |16  PB2 (D 10) PWM
*       (D 8) PB0 14|    |15  PB1 (D 9)  PWM
*                   +----+
****************************************************************************************/












































/****************************************************************************************
* Sanguino/RepRap Motherboard with direct-drive extruders
*
*                        ATMega644P
*
*                        +---\/---+
*            (D 0) PB0  1|        |40  PA0 (AI 0 / D31)
*            (D 1) PB1  2|        |39  PA1 (AI 1 / D30)
*       INT2 (D 2) PB2  3|        |38  PA2 (AI 2 / D29)
*        PWM (D 3) PB3  4|        |37  PA3 (AI 3 / D28)
*        PWM (D 4) PB4  5|        |36  PA4 (AI 4 / D27)
*       MOSI (D 5) PB5  6|        |35  PA5 (AI 5 / D26)
*       MISO (D 6) PB6  7|        |34  PA6 (AI 6 / D25)
*        SCK (D 7) PB7  8|        |33  PA7 (AI 7 / D24)
*                  RST  9|        |32  AREF
*                  VCC 10|        |31  GND 
*                  GND 11|        |30  AVCC
*                XTAL2 12|        |29  PC7 (D 23)
*                XTAL1 13|        |28  PC6 (D 22)
*       RX0 (D 8)  PD0 14|        |27  PC5 (D 21) TDI
*       TX0 (D 9)  PD1 15|        |26  PC4 (D 20) TDO
*  INT0 RX1 (D 10) PD2 16|        |25  PC3 (D 19) TMS
*  INT1 TX1 (D 11) PD3 17|        |24  PC2 (D 18) TCK
*       PWM (D 12) PD4 18|        |23  PC1 (D 17) SDA
*       PWM (D 13) PD5 19|        |22  PC0 (D 16) SCL
*       PWM (D 14) PD6 20|        |21  PD7 (D 15) PWM
*                        +--------+
*
****************************************************************************************/














































/****************************************************************************************
* RepRap Motherboard  ****---NOOOOOO RS485/EXTRUDER CONTROLLER!!!!!!!!!!!!!!!!!---*******
*
****************************************************************************************/























































/****************************************************************************************
* Arduino Mega pin assignment
*
****************************************************************************************/









 
 
 












































































  
  
  


  
  
  










  
  
  

  




/****************************************************************************************
* Duemilanove w/ ATMega328P pin assignment
*
****************************************************************************************/










































/****************************************************************************************
* Gen6 pin assignment
*
****************************************************************************************/

#define KNOWN_BOARD 1


    

//x axis pins
    #define X_STEP_PIN 15
    #define X_DIR_PIN 18
    #define X_ENABLE_PIN 19
    #define X_MIN_PIN 20
    #define X_MAX_PIN -1
    
    //y axis pins
    #define Y_STEP_PIN 23
    #define Y_DIR_PIN 22
    #define Y_ENABLE_PIN 24
    #define Y_MIN_PIN 25
    #define Y_MAX_PIN -1
    
    //z axis pins
    #define Z_STEP_PIN 27
    #define Z_DIR_PIN 28
    #define Z_ENABLE_PIN 29
    #define Z_MIN_PIN 30
    #define Z_MAX_PIN -1
    
    //extruder pins
    #define E_STEP_PIN 4
    #define E_DIR_PIN 2
    #define E_ENABLE_PIN 3
    #define TEMP_0_PIN 5
    #define HEATER_0_PIN 14
    #define HEATER_1_PIN -1
    
    
    #define SDPOWER -1
    #define SDSS 17
    #define LED_PIN -1
    #define TEMP_1_PIN -1
    #define FAN_PIN -1
    #define PS_ON_PIN -1
    //our pin for debugging.
    
    #define DEBUG_PIN 0
    
    //our RS485 pins
    #define TX_ENABLE_PIN 12
    #define RX_ENABLE_PIN 13


/****************************************************************************************
* Sanguinololu pin assignment
*
****************************************************************************************/



































































