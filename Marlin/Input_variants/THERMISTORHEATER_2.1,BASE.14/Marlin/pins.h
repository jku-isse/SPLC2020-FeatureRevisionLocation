
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




    



    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    



/****************************************************************************************
* Sanguinololu pin assignment
*
****************************************************************************************/


































































#define KNOWN_BOARD 
/*****************************************************************
* Ultimaker pin assignment
******************************************************************/


 
  


#define X_STEP_PIN 25
#define X_DIR_PIN 23
#define X_MIN_PIN 22
#define X_MAX_PIN 24
#define X_ENABLE_PIN 27

#define Y_STEP_PIN 31
#define Y_DIR_PIN 33
#define Y_MIN_PIN 26
#define Y_MAX_PIN 28
#define Y_ENABLE_PIN 29

#define Z_STEP_PIN 37
#define Z_DIR_PIN 39
#define Z_MIN_PIN 30
#define Z_MAX_PIN 32
#define Z_ENABLE_PIN 35

#define HEATER_1_PIN 4
#define TEMP_1_PIN 11

#define EXTRUDER_0_STEP_PIN 43
#define EXTRUDER_0_DIR_PIN 45
#define EXTRUDER_0_ENABLE_PIN 41
#define HEATER_0_PIN 2
#define TEMP_0_PIN 8

#define EXTRUDER_1_STEP_PIN 49
#define EXTRUDER_1_DIR_PIN 47
#define EXTRUDER_1_ENABLE_PIN 51
#define EXTRUDER_1_HEATER_PIN 3
#define EXTRUDER_1_TEMPERATURE_PIN 10
#define HEATER_2_PIN 51
#define TEMP_2_PIN 3



#define E_STEP_PIN EXTRUDER_0_STEP_PIN
#define E_DIR_PIN EXTRUDER_0_DIR_PIN
#define E_ENABLE_PIN EXTRUDER_0_ENABLE_PIN

#define SDPOWER -1
#define SDSS 53
#define LED_PIN 13
#define FAN_PIN 7
#define PS_ON_PIN 12
#define KILL_PIN -1







