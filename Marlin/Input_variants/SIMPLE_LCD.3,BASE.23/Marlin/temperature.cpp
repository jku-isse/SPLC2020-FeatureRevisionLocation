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

int target_bed_raw = 0;
int current_bed_raw = 0;

int target_raw[3] = {0, 0, 0};
int current_raw[3] = {0, 0, 0};
unsigned char temp_meas_ready = false;

unsigned long previous_millis_heater, previous_millis_bed_heater;


  double temp_iState = 0;
  double temp_dState = 0;
  double pTerm;
  double iTerm;
  double dTerm;
      //int output;
  double pid_error;
  double temp_iState_min;
  double temp_iState_max;
  double pid_setpoint = 0.0;
  double pid_input;
  double pid_output;
  bool pid_reset;
  float HeaterPower;
  
  float Kp=(3000/2.2);
  float Ki=(1.2*Kp/45*0.1);
  float Kd=(0);
  float Kc=(5);



int minttemp_0 = temp2analog(5);


int maxttemp_0 = temp2analog(275);
















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


    pid_input = analog2temp(current_raw[TEMPSENSOR_HOTEND]);


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
            Serial.println("!! Temp measurement error !!");
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

  
    if(current_raw[TEMPSENSOR_HOTEND] >= maxttemp) {
      target_raw[TEMPSENSOR_HOTEND] = 0;
      analogWrite(2, 0);
      Serial.println("!! Temperature extruder 0 switched off. MAXTEMP triggered !!");
      kill();
    }
  

    if(current_raw[TEMPSENSOR_AUX] >= maxttemp) {
      target_raw[TEMPSENSOR_AUX] = 0;
    if(current_raw[2] >= maxttemp_1) {
      analogWrite(51, 0);
      Serial.println("!! Temperature extruder 1 switched off. MAXTEMP triggered !!");
      kill()
    }
  


  
    if(current_raw[TEMPSENSOR_HOTEND] <= minttemp) {
      target_raw[TEMPSENSOR_HOTEND] = 0;
      analogWrite(2, 0);
      Serial.println("!! Temperature extruder 0 switched off. MINTEMP triggered !!");
      kill();
    }
  


  
    
      
      
      
      
    
  


  
    
      
      
      
      
    
  


  
    
      
      
      
      
    
  

  }
}
