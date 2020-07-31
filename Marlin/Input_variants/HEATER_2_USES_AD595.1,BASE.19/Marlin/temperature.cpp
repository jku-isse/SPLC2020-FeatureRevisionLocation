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





void static Heater::manage_heater()
{

  wd_reset();

  
  float pid_input;
  float pid_output;
  if(htr.temp_meas_ready != true)   //better readability
    return; 

unsigned char _sreg = SREG; cli();;
    htr.temp_meas_ready = false;
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
float const static temp2analog(const int celsius)
{
  
    
    
    
    
    
      
      
        
          
          
          
      
        
      
    

    
    

    
  
  
}

// Takes bed temperature value as input and returns corresponding raw value. 
// For a thermistor, it uses the RepRap thermistor temp table.
// This is needed because PID in hydra firmware hovers around a given analog value, not a temp value.
// This function is derived from inversing the logic from a portion of getTemperature() in FiveD RepRap firmware.
float const static temp2analogBed(const int celsius)
{
  

    
    
    
    
    
      
      
        
          
          
          
      
        
      
    

    
    

    
  
  
}

// Derived from RepRap FiveD extruder::getTemperature()
// For hot end temperature measurement.
float const static Heater::analog2temp(const int raw) {
  
    
    
    
    
    
      
      
        
          
          
          

        
      
    

    
    

    
  
  
}

// Derived from RepRap FiveD extruder::getTemperature()
// For bed temperature measurement.
float const static Heater::analog2tempBed(const int raw) {
  
    
    

    

    
    
      
      
        
          
          
          

        
      
    

    
    

    
    
  
  
}

Heater::Heater()
{
  for(short i=0;i<3;i++)
  {
    target_raw[i]=0;
    current_raw[i] =0;
  }
  htr.temp_meas_ready = false;
  
  minttemp = temp2analog(5);
  
  
  maxttemp = temp2analog(275);
  

  
  bed_minttemp = temp2analog(5);
  
  
  bed_maxttemp = temp2analog(150);
  
  

  do {DIO2_DDR |= (1 << DIO2_PIN); } while (0);


  do {DIO4_DDR |= (1 << DIO4_PIN); } while (0);


  do {DIO51_DDR |= (1 << DIO51_PIN); } while (0);



  temp_iState_min = 0.0;
  temp_iState_max = 255 / Ki;
  temp_iState = 0;
  temp_dState = 0;
  Kp=(3000/2.2);
  Ki=(1.2*Kp/45*0.1);
  Kd=(0);
  Kc=(5);
  pid_setpoint = 0.0;



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
    
      htr.current_raw[0] = raw_temp_0_value;
    
      
    
    
    
      htr.current_raw[2] = raw_temp_2_value;
    
      
    
    
    
      
    
      htr.current_raw[1] = 16383 - raw_temp_1_value;
    
    
   htr.temp_meas_ready = true;
    temp_count = 0;
    raw_temp_0_value = 0;
    raw_temp_1_value = 0;
    raw_temp_2_value = 0;

  
    if(htr.current_raw[TEMPSENSOR_HOTEND] >= htr.maxttemp) {
      htr.target_raw[TEMPSENSOR_HOTEND] = 0;
      analogWrite(2, 0);
      Serial.println("!! Temperature extruder 0 switched off. MAXTEMP triggered !!");
    }
  
  
    if(htr.current_raw[TEMPSENSOR_AUX] >= htr.maxttemp) {
      htr.target_raw[TEMPSENSOR_AUX] = 0;
      analogWrite(51, 0);
      Serial.println("!! Temperature extruder 1 switched off. MAXTEMP triggered !!");
    }
  


  
    if(htr.current_raw[TEMPSENSOR_HOTEND] <= htr.minttemp) {
      htr.target_raw[TEMPSENSOR_HOTEND] = 0;
      analogWrite(2, 0);
      Serial.println("!! Temperature extruder 0 switched off. MINTEMP triggered !!");
    }
  
  
    if(htr.current_raw[TEMPSENSOR_AUX] <= htr.minttemp) {
      htr.target_raw[TEMPSENSOR_AUX] = 0;
      analogWrite(51, 0);
      Serial.println("!! Temperature extruder 1 switched off. MINTEMP triggered !!");
    }
  


  
    if(htr.current_raw[1] <= htr.bed_minttemp) {
      htr.target_raw[1] = 0;
      do { if (0) {DIO4_WPORT |= (1 << DIO4_PIN); } else {DIO4_WPORT &= ~(1 << DIO4_PIN); }; } while (0);
      Serial.println("!! Temperatur heated bed switched off. MINTEMP triggered !!");
    }
  


  
    if(htr.current_raw[1] >= htr.bed_maxttemp) {
      htr.target_raw[1] = 0;
      do { if (0) {DIO4_WPORT |= (1 << DIO4_PIN); } else {DIO4_WPORT &= ~(1 << DIO4_PIN); }; } while (0);
      Serial.println("!! Temperature heated bed switched off. MAXTEMP triggered !!");
    }
  

  }
}

//Heater htr;

