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
  
  float Kp=20.0;
  float Ki=1.5*0.10;
  float Kd=80/0.10;
  float Kc=0;



int minttemp = temp2analog(5);


int maxttemp = temp2analog(275);



int bed_minttemp = temp2analog(5);


int bed_maxttemp = temp2analog(150);


void manage_heater()
{

  wd_reset();

  
  float pid_input;
  float pid_output;
  if(temp_meas_ready == true) {

unsigned char _sreg = SREG; cli();;
    temp_meas_ready = false;
SREG = _sreg;;


    pid_input = analog2temp(current_raw[0]);


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
      #define K1 0.95
      #define K2 (1.0-K1)
      dTerm = (Kd * (pid_input - temp_dState))*(1.0-0.95) + (0.95 * dTerm);
      temp_dState = pid_input;
      pid_output = constrain(pTerm + iTerm - dTerm, 0, 255);
    }


     
     
     
     
     
     
     
     
     
     
     

    analogWrite(2, pid_output);



    
    
      
    
    
    
      
    

    
  if(millis() - previous_millis_bed_heater < 5000)
    return;
  previous_millis_bed_heater = millis();
  
  
    if(current_raw[1] >= target_raw[1])
    {
      do { if (LOW) {DIO4_WPORT |= (1 << DIO4_PIN); } else {DIO4_WPORT &= ~(1 << DIO4_PIN); }; } while (0);
    }
    else 
    {
      do { if (HIGH) {DIO4_WPORT |= (1 << DIO4_PIN); } else {DIO4_WPORT &= ~(1 << DIO4_PIN); }; } while (0);
    }
  
  }
}

// Takes hot end temperature value as input and returns corresponding raw value. 
// For a thermistor, it uses the RepRap thermistor temp table.
// This is needed because PID in hydra firmware hovers around a given analog value, not a temp value.
// This function is derived from inversing the logic from a portion of getTemperature() in FiveD RepRap firmware.
float temp2analog(int celsius) {
  
    
    
    
    
    
      
      
        
          
          
          
      
        
      
    

    
    

    
  
  
}

// Takes bed temperature value as input and returns corresponding raw value. 
// For a thermistor, it uses the RepRap thermistor temp table.
// This is needed because PID in hydra firmware hovers around a given analog value, not a temp value.
// This function is derived from inversing the logic from a portion of getTemperature() in FiveD RepRap firmware.
float temp2analogBed(int celsius) {
  

    int raw = 0;
    byte i;
    
    for (i=1; i<BNUMTEMPS; i++)
    {
      if (bedtemptable[i][1] < celsius)
      {
        raw = bedtemptable[i-1][0] + 
          (celsius - bedtemptable[i-1][1]) * 
          (bedtemptable[i][0] - bedtemptable[i-1][0]) /
          (bedtemptable[i][1] - bedtemptable[i-1][1]);
      
        break;
      }
    }

    // Overflow: Set to last value in the table
    if (i == BNUMTEMPS) raw = bedtemptable[i-1][0];

    return (1023 * 16) - raw;
  
  
}

// Derived from RepRap FiveD extruder::getTemperature()
// For hot end temperature measurement.
float analog2temp(int raw) {
  
    
    
    
    
    
      
      
        
          
          
          

        
      
    

    
    

    
  
  
}

// Derived from RepRap FiveD extruder::getTemperature()
// For bed temperature measurement.
float analog2tempBed(int raw) {
  
    int celsius = 0;
    byte i;

    raw = (1023 * 16) - raw;

    for (i=1; i<BNUMTEMPS; i++)
    {
      if (bedtemptable[i][0] > raw)
      {
        celsius  = bedtemptable[i-1][1] + 
          (raw - bedtemptable[i-1][0]) * 
          (bedtemptable[i][1] - bedtemptable[i-1][1]) /
          (bedtemptable[i][0] - bedtemptable[i-1][0]);

        break;
      }
    }

    // Overflow: Set to last value in the table
    if (i == BNUMTEMPS) celsius = bedtemptable[i-1][1];

    return celsius;
    
  
  
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

static unsigned char temp_count = 0;
static unsigned long raw_temp_0_value = 0;
static unsigned long raw_temp_1_value = 0;
static unsigned long raw_temp_2_value = 0;
static unsigned char temp_state = 0;

// Timer 0 is shared with millies
ISR(TIMER0_COMPB_vect)
{
  switch(temp_state) {
    case 0: // Prepare TEMP_0
            
              
                
              
                DIDR2 = 1<<(8 - 8); 
                ADCSRB = 1<<MUX5;
              
              ADMUX = ((1 << REFS0) | (8 & 0x07));
              ADCSRA |= 1<<ADSC; // Start conversion
            
            
              
            
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

  
    if(current_raw[0] >= maxttemp) {
      target_raw[0] = 0;
      analogWrite(2, 0);
      Serial.println("!! Temperature extruder 0 switched off. MAXTEMP triggered !!");
    }
  
  
    if(current_raw[2] >= maxttemp) {
      target_raw[2] = 0;
      analogWrite(51, 0);
      Serial.println("!! Temperature extruder 1 switched off. MAXTEMP triggered !!");
    }
  


  
    if(current_raw[0] <= minttemp) {
      target_raw[0] = 0;
      analogWrite(2, 0);
      Serial.println("!! Temperature extruder 0 switched off. MINTEMP triggered !!");
    }
  
  
    if(current_raw[2] <= minttemp) {
      target_raw[2] = 0;
      analogWrite(51, 0);
      Serial.println("!! Temperature extruder 1 switched off. MINTEMP triggered !!");
    }
  


  
    if(current_raw[1] <= bed_minttemp) {
      target_raw[1] = 0;
      do { if (0) {DIO4_WPORT |= (1 << DIO4_PIN); } else {DIO4_WPORT &= ~(1 << DIO4_PIN); }; } while (0);
      Serial.println("!! Temperatur heated bed switched off. MINTEMP triggered !!");
    }
  


  
    if(current_raw[1] >= bed_maxttemp) {
      target_raw[1] = 0;
      do { if (0) {DIO4_WPORT |= (1 << DIO4_PIN); } else {DIO4_WPORT &= ~(1 << DIO4_PIN); }; } while (0);
      Serial.println("!! Temperature heated bed switched off. MAXTEMP triggered !!");
    }
  

  }
}
