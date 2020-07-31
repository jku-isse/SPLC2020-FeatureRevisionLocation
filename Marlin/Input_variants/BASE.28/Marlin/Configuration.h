
#define CONFIGURATION_H 

//#define DEBUG_STEPS

#define MM_PER_ARC_SEGMENT 1
#define N_ARC_CORRECTION 25

// BASIC SETTINGS: select your board type, thermistor type, axis scaling, and endstop configuration

//// The following define selects which electronics board you have. Please choose the one that matches your setup
// MEGA/RAMPS up to 1.2 = 3,
// RAMPS 1.3 = 33
// Gen6 = 5,
// Sanguinololu 1.2 and above = 62
// Ultimaker = 7,
#define MOTHERBOARD 7
//#define MOTHERBOARD 5


//// Thermistor settings:
// 1 is 100k thermistor
// 2 is 200k thermistor
// 3 is mendel-parts thermistor
// 4 is 10k thermistor
// 5 is ParCan supplied 104GT-2 100K
// 6 is EPCOS 100k
// 7 is 100k Honeywell thermistor 135-104LAG-J01
#define THERMISTORHEATER_1 3
#define THERMISTORHEATER_2 3
#define THERMISTORBED 3

//#define HEATER_0_USES_THERMISTOR
//#define HEATER_1_USES_THERMISTOR
#define HEATER_0_USES_AD595 
//#define HEATER_1_USES_AD595

// Select one of these only to define how the bed temp is read.
//#define BED_USES_THERMISTOR
//#define BED_USES_AD595

#define HEATER_CHECK_INTERVAL 50
#define BED_CHECK_INTERVAL 5000


//// Endstop Settings
#define ENDSTOPPULLUPS 
// The pullups are needed if you directly connect a mechanical endswitch between the signal and ground pins.
const bool ENDSTOPS_INVERTING = true; // set to true to invert the logic of the endstops. 
// For optos H21LOB set to true, for Mendel-Parts newer optos TCST2103 set to false

// This determines the communication speed of the printer
#define BAUDRATE 250000
//#define BAUDRATE 115200
//#define BAUDRATE 230400

// Comment out (using // at the start of the line) to disable SD support:

// #define ULTRA_LCD  //any lcd 

#define ULTIPANEL 
#define ULTIPANEL 

 //#define NEWPANEL  //enable this if you have a click-encoder panel
 #define SDSUPPORT 
 #define ULTRA_LCD 
 #define LCD_WIDTH 20
#define LCD_HEIGHT 4

  
    
    
  



//#define SDSUPPORT // Enable SD Card Support in Hardware Console



const int dropsegments=5; //everything with this number of steps  will be ignored as move

//// ADVANCED SETTINGS - to tweak parameters

#include "thermistortables.h"

// For Inverting Stepper Enable Pins (Active Low) use 0, Non Inverting (Active High) use 1
#define X_ENABLE_ON 0
#define Y_ENABLE_ON 0
#define Z_ENABLE_ON 0
#define E_ENABLE_ON 0

// Disables axis when it's not being used.
#define DISABLE_X false
#define DISABLE_Y false
#define DISABLE_Z false
#define DISABLE_E false

// Inverting axis direction
#define INVERT_X_DIR true
#define INVERT_Y_DIR false
#define INVERT_Z_DIR true
#define INVERT_E_DIR false

//// ENDSTOP SETTINGS:
// Sets direction of endstops when homing; 1=MAX, -1=MIN
#define X_HOME_DIR -1
#define Y_HOME_DIR -1
#define Z_HOME_DIR -1

#define min_software_endstops false
#define max_software_endstops false
#define X_MAX_LENGTH 210
#define Y_MAX_LENGTH 210
#define Z_MAX_LENGTH 210

//// MOVEMENT SETTINGS
#define NUM_AXIS 4
//note: on bernhards ultimaker 200 200 12 are working well.
#define HOMING_FEEDRATE {50*60, 50*60, 12*60, 0}
//the followint checks if an extrusion is existent in the move. if _not_, the speed of the move is set to the maximum speed. 
//!!!!!!Use only if you know that your printer works at the maximum declared speeds.
// works around the skeinforge cool-bug. There all moves are slowed to have a minimum layer time. However slow travel moves= ooze
#define TRAVELING_AT_MAXSPEED 
#define AXIS_RELATIVE_MODES {false, false, false, false}

#define MAX_STEP_FREQUENCY 40000

// default settings 

#define DEFAULT_AXIS_STEPS_PER_UNIT {79.87220447,79.87220447,200*8/3,14}
#define DEFAULT_MAX_FEEDRATE {160*60, 160*60, 10*60, 500000}
#define DEFAULT_MAX_ACCELERATION {9000,9000,150,10000}

#define DEFAULT_ACCELERATION 3000
#define DEFAULT_RETRACT_ACCELERATION 7000

#define DEFAULT_MINIMUMFEEDRATE 10
#define DEFAULT_MINTRAVELFEEDRATE 10

// minimum time in microseconds that a movement needs to take if the buffer is emptied.   Increase this number if you see blobs while printing high speed & high detail.  It will slowdown on the detailed stuff.
#define DEFAULT_MINSEGMENTTIME 20000
#define DEFAULT_XYJERK 30.0*60
#define DEFAULT_ZJERK 10.0*60


// The watchdog waits for the watchperiod in milliseconds whenever an M104 or M109 increases the target temperature
//this enables the watchdog interrupt.
#define USE_WATCHDOG 
//you cannot reboot on a mega2560 due to a bug in he bootloader. Hence, you have to reset manually, and this is done hereby:
#define RESET_MANUAL 

#define WATCHDOG_TIMEOUT 4



//// Experimental watchdog and minimal temp
// The watchdog waits for the watchperiod in milliseconds whenever an M104 or M109 increases the target temperature
// If the temperature has not increased at the end of that period, the target temperature is set to zero. It can be reset with another M104/M109
/// CURRENTLY NOT IMPLEMENTED AND UNUSEABLE
//#define WATCHPERIOD 5000 //5 seconds

// Actual temperature must be close to target for this long before M109 returns success
//#define TEMP_RESIDENCY_TIME 20  // (seconds)
//#define TEMP_HYSTERESIS 5       // (C°) range of +/- temperatures considered "close" to the target one

//// The minimal temperature defines the temperature below which the heater will not be enabled
#define HEATER_0_MINTEMP 5
//#define HEATER_1_MINTEMP 5
//#define BED_MINTEMP 5


// When temperature exceeds max temp, your heater will be switched off.
// This feature exists to protect your hotend from overheating accidentally, but *NOT* from thermistor short/failure!
// You should use MINTEMP for thermistor short/failure protection.
#define HEATER_0_MAXTEMP 275
//#define_HEATER_1_MAXTEMP 275
//#define BED_MAXTEMP 150







#define PIDTEMP 

  /// PID settings:
  // Uncomment the following line to enable PID support.
  //#define SMOOTHING
  //#define SMOOTHFACTOR 5.0
  //float current_raw_average=0;
    #define K1 0.95
  //#define PID_DEBUG // Sends debug data to the serial port. 
  //#define PID_OPENLOOP 1 // Puts PID in open loop. M104 sets the output power in %
  #define PID_MAX 255
  #define PID_INTEGRAL_DRIVE_MAX 255
  #define PID_dT 0.1
 //machine with red silicon: 1950:45 second ; with fan fully blowin 3000:47

  #define PID_CRITIAL_GAIN 3000
  #define PID_SWING_AT_CRITIAL 45
  #define PIDIADD 5
  /*
  //PID according to Ziegler-Nichols method
  float Kp = 0.6*PID_CRITIAL_GAIN; 
  float Ki =PIDIADD+2*Kp/PID_SWING_AT_CRITIAL*PID_dT;  
  float Kd = Kp*PID_SWING_AT_CRITIAL/8./PID_dT;  
  */
  //PI according to Ziegler-Nichols method
  #define DEFAULT_Kp (PID_CRITIAL_GAIN/2.2)
  #define DEFAULT_Ki (1.2*Kp/PID_SWING_AT_CRITIAL*PID_dT)
  #define DEFAULT_Kd (0)
  
  #define PID_ADD_EXTRUSION_RATE 
  
    #define DEFAULT_Kc (5)
  


// extruder advance constant (s2/mm3)
//
// advance (steps) = STEPS_PER_CUBIC_MM_E * EXTUDER_ADVANCE_K * cubic mm per second ^ 2
//
// hooke's law says:		force = k * distance
// bernoulli's priniciple says:	v ^ 2 / 2 + g . h + pressure / density = constant
// so: v ^ 2 is proportional to number of steps we advance the extruder
//#define ADVANCE











// THE BLOCK_BUFFER_SIZE NEEDS TO BE A POWER OF 2, e.g. 8,16,32 

// The number of linear motions that can be in the plan at any give time.  
  #define BLOCK_BUFFER_SIZE 16

  




