/*
  temperature.c - temperature control
  Part of Marlin
  
 Copyright (C) 2011 Camiel Gubbels / Erik van der Zalm
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 This firmware is a mashup between Sprinter and grbl.
  (https://github.com/kliment/Sprinter)
  (https://github.com/simen/grbl/tree)
 
 It has preliminary support for Matthew Roberts advance algorithm 
    http://reprap.org/pipermail/reprap-dev/2011-May/003323.html

 This firmware is optimized for gen6 electronics.
 */

#include "fastio.h"
#include "Configuration.h"
#include "pins.h"
#include "Marlin.h"
#include "ultralcd.h"
#include "streaming.h"
#include "temperature.h"
#include "watchdog.h"


int target_raw[3] = {0, 0, 0};
int current_raw[3] = {0, 0, 0};

static bool temp_meas_ready = false;

static unsigned long previous_millis_heater, previous_millis_bed_heater;


  //static cannot be external:
  static float temp_iState = 0;
  static float temp_dState = 0;
  static float pTerm;
  static float iTerm;
  static float dTerm;
      //int output;
  static float pid_error;
  static float temp_iState_min;
  static float temp_iState_max;
  static float pid_input;
  static float pid_output;
  static bool pid_reset;
  
  // probably used external
  float HeaterPower;
  float pid_setpoint = 0.0;

  
  float Kp=(3000/2.2);
  float Ki=(1.2*Kp/45*0.1);
  float Kd=(0);
  float Kc=(5);

  

  static int watch_raw[3] = {-1000,-1000,-1000};
  static unsigned long watchmillis = 0;



  static int minttemp_0 = temp2analog(5);


  static int maxttemp_0 = temp2analog(275);



  


  



  


  


void manage_heater()
{
  
    wd_reset();
  
  
  float pid_input;
  float pid_output;
  if(temp_meas_ready != true)   //better readability
    return; 

  unsigned char _sreg = SREG; cli();;
    temp_meas_ready = false;
  SREG = _sreg;;

  
    pid_input = analog2temp(current_raw[TEMPSENSOR_HOTEND_0]);

    
        pid_error = pid_setpoint - pid_input;
        if(pid_error > 10){
          pid_output = 255;
          pid_reset = true;
        }
        else if(pid_error < -10) {
          pid_output = 0;
          pid_reset = true;
        }
        else {
          if(pid_reset == true) {
            temp_iState = 0.0;
            pid_reset = false;
          }
          pTerm = Kp * pid_error;
          temp_iState += pid_error;
          temp_iState = constrain(temp_iState, temp_iState_min, temp_iState_max);
          iTerm = Ki * temp_iState;
          //K1 defined in Configuration.h in the PID settings
          #define K2 (1.0-K1)
          dTerm = (Kd * (pid_input - temp_dState))*(1.0-0.95) + (0.95 * dTerm);
          temp_dState = pid_input;
          
            pTerm+=Kc*current_block->speed_e; //additional heating if extrusion speed is high
          
          pid_output = constrain(pTerm + iTerm - dTerm, 0, 255);
        }
    
    
     
    
    analogWrite(2, pid_output);
  

  
    
    
      
    
    
    
      
    
  
    
  if(millis() - previous_millis_bed_heater < 5000)
    return;
  previous_millis_bed_heater = millis();
  
  
    if(current_raw[TEMPSENSOR_BED] >= target_raw[TEMPSENSOR_BED])
    {
      do { if (LOW) {DIO4_WPORT |= (1 << DIO4_PIN); } else {DIO4_WPORT &= ~(1 << DIO4_PIN); }; } while (0);
    }
    else 
    {
      do { if (HIGH) {DIO4_WPORT |= (1 << DIO4_PIN); } else {DIO4_WPORT &= ~(1 << DIO4_PIN); }; } while (0);
    }
  
}

// Takes hot end temperature value as input and returns corresponding raw value. 
// For a thermistor, it uses the RepRap thermistor temp table.
// This is needed because PID in hydra firmware hovers around a given analog value, not a temp value.
// This function is derived from inversing the logic from a portion of getTemperature() in FiveD RepRap firmware.
int temp2analog(int celsius) {
  
    
    
    
    
    
      
      
        
          
          
          
        
      
    

    
    

    
  
  
}

// Takes bed temperature value as input and returns corresponding raw value. 
// For a thermistor, it uses the RepRap thermistor temp table.
// This is needed because PID in hydra firmware hovers around a given analog value, not a temp value.
// This function is derived from inversing the logic from a portion of getTemperature() in FiveD RepRap firmware.
int temp2analogBed(int celsius) {
  

    
    
    
    
    
      
      
        
          
          
          
      
        
      
    

    
    

    
  
  
}

// Derived from RepRap FiveD extruder::getTemperature()
// For hot end temperature measurement.
float analog2temp(int raw) {
  
    
    
    
    
    
      
      
        
          
          
          

        
      
    

    
    

    
  
  
}

// Derived from RepRap FiveD extruder::getTemperature()
// For bed temperature measurement.
float analog2tempBed(int raw) {
  
    
    

    

    
    
      
      
        
          
          
          

        
      
    

    
    

    
    
  
  
}

void tp_init()
{
  
    do {DIO2_DDR |= (1 << DIO2_PIN); } while (0);
  
  
    do {DIO4_DDR |= (1 << DIO4_PIN); } while (0);
  
  
    do {DIO51_DDR |= (1 << DIO51_PIN); } while (0);
  

  
    temp_iState_min = 0.0;
    temp_iState_max = 255 / Ki;
  

  // Set analog inputs
  ADCSRA = 1<<ADEN | 1<<ADSC | 1<<ADIF | 0x07;
  
  // Use timer0 for temperature measurement
  // Interleave temperature interrupt with millies interrupt
  OCR0B = 128;
  TIMSK0 |= (1<<OCIE0B);  
}



void setWatch() 
{  

  if(isHeatingHotend0())
  {
    watchmillis = max(1,millis());
    watch_raw[TEMPSENSOR_HOTEND_0] = current_raw[TEMPSENSOR_HOTEND_0];
  }
  else
  {
    watchmillis = 0;
  } 

}


void disable_heater()
{
  
  target_raw[0]=0;
   
     do { if (LOW) {DIO2_WPORT |= (1 << DIO2_PIN); } else {DIO2_WPORT &= ~(1 << DIO2_PIN); }; } while (0);
   
  
     
  
    target_raw[1]=0;
    
      do { if (LOW) {DIO4_WPORT |= (1 << DIO4_PIN); } else {DIO4_WPORT &= ~(1 << DIO4_PIN); }; } while (0);
    
  
      
  
    target_raw[2]=0;
    
      do { if (LOW) {DIO51_WPORT |= (1 << DIO51_PIN); } else {DIO51_WPORT &= ~(1 << DIO51_PIN); }; } while (0);
    
  
}

// Timer 0 is shared with millies
ISR(TIMER0_COMPB_vect)
{
  //these variables are only accesible from the ISR, but static, so they don't loose their value
  static unsigned char temp_count = 0;
  static unsigned long raw_temp_0_value = 0;
  static unsigned long raw_temp_1_value = 0;
  static unsigned long raw_temp_2_value = 0;
  static unsigned char temp_state = 0;
  
  switch(temp_state) {
    case 0: // Prepare TEMP_0
      
        
          
        
          DIDR2 = 1<<(8 - 8); 
          ADCSRB = 1<<MUX5;
        
        ADMUX = ((1 << REFS0) | (8 & 0x07));
        ADCSRA |= 1<<ADSC; // Start conversion
      
      
        buttons_check();
      
      temp_state = 1;
      break;
    case 1: // Measure TEMP_0
      
        raw_temp_0_value += ADC;
      
      temp_state = 2;
      break;
    case 2: // Prepare TEMP_1
      
        
          
        
          DIDR2 = 1<<(11 - 8); 
          ADCSRB = 1<<MUX5;
        
        ADMUX = ((1 << REFS0) | (11 & 0x07));
        ADCSRA |= 1<<ADSC; // Start conversion
      
      
        buttons_check();
      
      temp_state = 3;
      break;
    case 3: // Measure TEMP_1
      
        raw_temp_1_value += ADC;
      
      temp_state = 4;
      break;
    case 4: // Prepare TEMP_2
      
        
          DIDR0 = 1 << 3; 
        
          
          
        
        ADMUX = ((1 << REFS0) | (3 & 0x07));
        ADCSRA |= 1<<ADSC; // Start conversion
      
      
        buttons_check();
      
      temp_state = 5;
      break;
    case 5: // Measure TEMP_2
      
        raw_temp_2_value += ADC;
      
      temp_state = 0;
      temp_count++;
      break;
    default:
      Serial << "echo: ERROR: " << "Temp measurement error!"<<endl;;
      break;
  }
    
  if(temp_count >= 16) // 6 ms * 16 = 96ms.
  {
    
      current_raw[0] = raw_temp_0_value;
    
      
    
    
    
      
    
      current_raw[2] = 16383 - raw_temp_2_value;
    
    
    
      
    
      current_raw[1] = 16383 - raw_temp_1_value;
    
    
    temp_meas_ready = true;
    temp_count = 0;
    raw_temp_0_value = 0;
    raw_temp_1_value = 0;
    raw_temp_2_value = 0;
    
      
        if(current_raw[TEMPSENSOR_HOTEND_0] >= maxttemp_0) {
          target_raw[TEMPSENSOR_HOTEND_0] = 0;
          analogWrite(2, 0);
          Serial << "echo: ERROR: " << "Temperature extruder 0 switched off. MAXTEMP triggered !!"<<endl;;
          kill();
        }
      
    
  
    
      
        
      
        
        
        
      
    
  
  
  
    
      if(current_raw[TEMPSENSOR_HOTEND_0] <= minttemp_0) {
        target_raw[TEMPSENSOR_HOTEND_0] = 0;
        analogWrite(2, 0);
        Serial << "echo: ERROR: " << "Temperature extruder 0 switched off. MINTEMP triggered !!"<<endl;;
        kill();
      }
    
  
  
  
    
      
        
        
        
        
      
    
  
  
  
    
      
        
        
        
        
      
    
  
  
  
    
      
        
        
        
        
      
    
  
  }
}

