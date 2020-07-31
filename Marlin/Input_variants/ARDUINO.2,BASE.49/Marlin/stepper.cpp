/*
  stepper.c - stepper motor driver: executes motion plans using stepper motors
  Part of Grbl

  Copyright (c) 2009-2011 Simen Svale Skogsrud

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

/* The timer calculations of this module informed by the 'RepRap cartesian firmware' by Zack Smith
   and Philipp Tiefenbacher. */

#include "stepper.h"
#include "Configuration.h"
#include "Marlin.h"
#include "planner.h"
#include "pins.h"
#include "fastio.h"
#include "temperature.h"
#include "ultralcd.h"

#include "speed_lookuptable.h"


//===========================================================================
//=============================public variables  ============================
//===========================================================================
block_t *current_block;  // A pointer to the block currently being traced


//===========================================================================
//=============================private variables ============================
//===========================================================================
//static makes it inpossible to be called from outside of this file by extern.!

// Variables used by The Stepper Driver Interrupt
static unsigned char out_bits;        // The next stepping-bits to be output
static long counter_x,       // Counter variables for the bresenham line tracer
            counter_y, 
            counter_z,       
            counter_e;
static unsigned long step_events_completed; // The number of step events executed in the current block

  
  
  

static unsigned char busy = false; // TRUE when SIG_OUTPUT_COMPARE1A is being serviced. Used to avoid retriggering that handler.
static long acceleration_time, deceleration_time;
//static unsigned long accelerate_until, decelerate_after, acceleration_rate, initial_rate, final_rate, nominal_rate;
static unsigned short acc_step_rate; // needed for deccelaration start point
static char step_loops;



// if DEBUG_STEPS is enabled, M114 can be used to compare two methods of determining the X,Y,Z position of the printer.
// for debugging purposes only, should be disabled by default

  
  


//===========================================================================
//=============================functions         ============================
//===========================================================================
  

// intRes = intIn1 * intIn2 >> 16
// uses:
// r26 to store 0
// r27 to store the byte 1 of the 24 bit result
#define MultiU16X8toH16(intRes, charIn1, intIn2) asm volatile ( "clr r26 \n\t" "mul %A1, %B2 \n\t" "movw %A0, r0 \n\t" "mul %A1, %A2 \n\t" "add %A0, r1 \n\t" "adc %B0, r26 \n\t" "lsr r0 \n\t" "adc %A0, r26 \n\t" "adc %B0, r26 \n\t" "clr r1 \n\t" : "=&r" (intRes) : "d" (charIn1), "d" (intIn2) : "r26" )




















// intRes = longIn1 * longIn2 >> 24
// uses:
// r26 to store 0
// r27 to store the byte 1 of the 48bit result
#define MultiU24X24toH16(intRes, longIn1, longIn2) asm volatile ( "clr r26 \n\t" "mul %A1, %B2 \n\t" "mov r27, r1 \n\t" "mul %B1, %C2 \n\t" "movw %A0, r0 \n\t" "mul %C1, %C2 \n\t" "add %B0, r0 \n\t" "mul %C1, %B2 \n\t" "add %A0, r0 \n\t" "adc %B0, r1 \n\t" "mul %A1, %C2 \n\t" "add r27, r0 \n\t" "adc %A0, r1 \n\t" "adc %B0, r26 \n\t" "mul %B1, %B2 \n\t" "add r27, r0 \n\t" "adc %A0, r1 \n\t" "adc %B0, r26 \n\t" "mul %C1, %A2 \n\t" "add r27, r0 \n\t" "adc %A0, r1 \n\t" "adc %B0, r26 \n\t" "mul %B1, %A2 \n\t" "add r27, r1 \n\t" "adc %A0, r26 \n\t" "adc %B0, r26 \n\t" "lsr r27 \n\t" "adc %A0, r26 \n\t" "adc %B0, r26 \n\t" "clr r1 \n\t" : "=&r" (intRes) : "d" (longIn1), "d" (longIn2) : "r26" , "r27" )








































// Some useful constants

#define ENABLE_STEPPER_DRIVER_INTERRUPT() TIMSK1 |= (1<<OCIE1A)
#define DISABLE_STEPPER_DRIVER_INTERRUPT() TIMSK1 &= ~(1<<OCIE1A)






//         __________________________
//        /|                        |\_________________         ^
//       / |                        | \/|               |\|
//      /  |                        |  \/ |               | \s
//     /   |                        |   |  |               |  \p
//    /    |                        |   |  |               |   \e
//   +-----+------------------------+---+--+---------------+----+    e
//   |               BLOCK 1            |      BLOCK 2          |    d
//
//                           time ----->
// 
//  The trapezoid is the shape the speed curve over time. It starts at block->initial_rate, accelerates 
//  first block->accelerate_until step_events_completed, then keeps going at constant speed until 
//  step_events_completed reaches block->decelerate_after after which it decelerates until the trapezoid generator is reset.
//  The slope of acceleration is calculated with the leib ramp alghorithm.

void st_wake_up() {
  //  TCNT1 = 0;
  TIMSK1 |= (1<<OCIE1A);  
}

inline unsigned short calc_timer(unsigned short step_rate) {
  unsigned short timer;
  if(step_rate > 40000) step_rate = 40000;
  
  if(step_rate > 20000) { // If steprate > 20kHz >> step 4 times
    step_rate = step_rate >> 2;
    step_loops = 4;
  }
  else if(step_rate > 10000) { // If steprate > 10kHz >> step 2 times
    step_rate = step_rate >> 1;
    step_loops = 2;
  }
  else {
    step_loops = 1;
  } 
  
  if(step_rate < 32) step_rate = 32;
  step_rate -= 32; // Correct for minimal speed
  if(step_rate >= (8*256)){ // higher step rate 
    unsigned short table_address = (unsigned short)&speed_lookuptable_fast[(unsigned char)(step_rate>>8)][0];
    unsigned char tmp_step_rate = (step_rate & 0x00ff);
    unsigned short gain = (unsigned short)pgm_read_word_near(table_address+2);
    asm volatile ( "clr r26 \n\t" "mul %A1, %B2 \n\t" "movw %A0, r0 \n\t" "mul %A1, %A2 \n\t" "add %A0, r1 \n\t" "adc %B0, r26 \n\t" "lsr r0 \n\t" "adc %A0, r26 \n\t" "adc %B0, r26 \n\t" "clr r1 \n\t" : "=&r" (timer) : "d" (tmp_step_rate), "d" (gain) : "r26" );
    timer = (unsigned short)pgm_read_word_near(table_address) - timer;
  }
  else { // lower step rates
    unsigned short table_address = (unsigned short)&speed_lookuptable_slow[0][0];
    table_address += ((step_rate)>>1) & 0xfffc;
    timer = (unsigned short)pgm_read_word_near(table_address);
    timer -= (((unsigned short)pgm_read_word_near(table_address+2) * (unsigned char)(step_rate & 0x0007))>>3);
  }
  if(timer < 100) timer = 100;
  return timer;
}

// Initializes the trapezoid generator from the current block. Called whenever a new 
// block begins.
inline void trapezoid_generator_reset() {
  
    
    
  
  deceleration_time = 0;
  // advance_rate = current_block->advance_rate;
  // step_rate to timer interval
  acc_step_rate = current_block->initial_rate;
  acceleration_time = calc_timer(acc_step_rate);
  OCR1A = acceleration_time;
}

// "The Stepper Driver Interrupt" - This timer interrupt is the workhorse.  
// It pops blocks from the block_buffer and executes them by pulsing the stepper pins appropriately. 
ISR(TIMER1_COMPA_vect)
{        
  if(busy){ 
    serialprintPGM(errormagic);
    Serial.print(*(unsigned short *)OCR1A);;
    {serialprintPGM(PSTR(" ISR overtaking itself."));Serial.write('\n');};
    return; 
  } // The busy-flag is used to avoid reentering this interrupt

  busy = true;
  sei(); // Re enable interrupts (normally disabled while inside an interrupt handler)

  // If there is no current block, attempt to pop one from the buffer
  if (current_block == NULL) {
    // Anything in the buffer?
    current_block = plan_get_current_block();
    if (current_block != NULL) {
      trapezoid_generator_reset();
      counter_x = -(current_block->step_event_count >> 1);
      counter_y = counter_x;
      counter_z = counter_x;
      counter_e = counter_x;
      step_events_completed = 0;
      
      
      
    } 
    else {
//      DISABLE_STEPPER_DRIVER_INTERRUPT();
    }    
  } 

  if (current_block != NULL) {
    // Set directions TO DO This should be done once during init of trapezoid. Endstops -> interrupt
    out_bits = current_block->direction_bits;

    
        
        
        
          
          
            
            
            
          
          
            
            
            
          
        
        
        
        
        
        
    

    // Set direction en check limit switches
    if ((out_bits & (1<<X_AXIS)) != 0) {   // -direction
      do { if (true) {DIO23_WPORT |= (1 << DIO23_PIN); } else {DIO23_WPORT &= ~(1 << DIO23_PIN); }; } while (0);
      
        
      
      
            if(((bool)(DIO22_RPORT & (1 << DIO22_PIN))) != ENDSTOPS_INVERTING) {
              step_events_completed = current_block->step_event_count;
            }
      
    }
    else { // +direction 
      do { if (!true) {DIO23_WPORT |= (1 << DIO23_PIN); } else {DIO23_WPORT &= ~(1 << DIO23_PIN); }; } while (0);
      
        
      
      
        if((((bool)(DIO24_RPORT & (1 << DIO24_PIN))) != ENDSTOPS_INVERTING)  && (current_block->steps_x >0)){
          step_events_completed = current_block->step_event_count;
        }
        
    }

    if ((out_bits & (1<<Y_AXIS)) != 0) {   // -direction
      do { if (false) {DIO33_WPORT |= (1 << DIO33_PIN); } else {DIO33_WPORT &= ~(1 << DIO33_PIN); }; } while (0);
      
        
      
      
        if(((bool)(DIO26_RPORT & (1 << DIO26_PIN))) != ENDSTOPS_INVERTING) {
          step_events_completed = current_block->step_event_count;
        }
      
    }
    else { // +direction
    do { if (!false) {DIO33_WPORT |= (1 << DIO33_PIN); } else {DIO33_WPORT &= ~(1 << DIO33_PIN); }; } while (0);
      
        
      
      
      if((((bool)(DIO28_RPORT & (1 << DIO28_PIN))) != ENDSTOPS_INVERTING) && (current_block->steps_y >0)){
          step_events_completed = current_block->step_event_count;
        }
      
    }

    if ((out_bits & (1<<Z_AXIS)) != 0) {   // -direction
      do { if (true) {DIO39_WPORT |= (1 << DIO39_PIN); } else {DIO39_WPORT &= ~(1 << DIO39_PIN); }; } while (0);
      
      
      
      
        if(((bool)(DIO30_RPORT & (1 << DIO30_PIN))) != ENDSTOPS_INVERTING) {
          step_events_completed = current_block->step_event_count;
        }
      
    }
    else { // +direction
      do { if (!true) {DIO39_WPORT |= (1 << DIO39_PIN); } else {DIO39_WPORT &= ~(1 << DIO39_PIN); }; } while (0);
      
        
      
      
        if((((bool)(DIO32_RPORT & (1 << DIO32_PIN))) != ENDSTOPS_INVERTING)  && (current_block->steps_z >0)){
          step_events_completed = current_block->step_event_count;
        }
      
    }

    
      if ((out_bits & (1<<E_AXIS)) != 0)   // -direction
        do { if (false) {DIO45_WPORT |= (1 << DIO45_PIN); } else {DIO45_WPORT &= ~(1 << DIO45_PIN); }; } while (0);
      else // +direction
        do { if (!false) {DIO45_WPORT |= (1 << DIO45_PIN); } else {DIO45_WPORT &= ~(1 << DIO45_PIN); }; } while (0);
    

    for(int8_t i=0; i < step_loops; i++) { // Take multiple steps per interrupt (For high speed moves) 
      counter_x += current_block->steps_x;
      if (counter_x > 0) {
        do { if (HIGH) {DIO25_WPORT |= (1 << DIO25_PIN); } else {DIO25_WPORT &= ~(1 << DIO25_PIN); }; } while (0);
        counter_x -= current_block->step_event_count;
        do { if (LOW) {DIO25_WPORT |= (1 << DIO25_PIN); } else {DIO25_WPORT &= ~(1 << DIO25_PIN); }; } while (0);
        
          
        
      }

      counter_y += current_block->steps_y;
      if (counter_y > 0) {
        do { if (HIGH) {DIO31_WPORT |= (1 << DIO31_PIN); } else {DIO31_WPORT &= ~(1 << DIO31_PIN); }; } while (0);
        counter_y -= current_block->step_event_count;
        do { if (LOW) {DIO31_WPORT |= (1 << DIO31_PIN); } else {DIO31_WPORT &= ~(1 << DIO31_PIN); }; } while (0);
        
          
        
      }

      counter_z += current_block->steps_z;
      if (counter_z > 0) {
        do { if (HIGH) {DIO37_WPORT |= (1 << DIO37_PIN); } else {DIO37_WPORT &= ~(1 << DIO37_PIN); }; } while (0);
        counter_z -= current_block->step_event_count;
        do { if (LOW) {DIO37_WPORT |= (1 << DIO37_PIN); } else {DIO37_WPORT &= ~(1 << DIO37_PIN); }; } while (0);
        
          
        
      }

      
        counter_e += current_block->steps_e;
        if (counter_e > 0) {
          do { if (HIGH) {DIO43_WPORT |= (1 << DIO43_PIN); } else {DIO43_WPORT &= ~(1 << DIO43_PIN); }; } while (0);
          counter_e -= current_block->step_event_count;
          do { if (LOW) {DIO43_WPORT |= (1 << DIO43_PIN); } else {DIO43_WPORT &= ~(1 << DIO43_PIN); }; } while (0);
        }
      
      step_events_completed += 1;  
      if(step_events_completed >= current_block->step_event_count) break;
    }
    // Calculare new timer value
    unsigned short timer;
    unsigned short step_rate;
    if (step_events_completed <= current_block->accelerate_until) {
      asm volatile ( "clr r26 \n\t" "mul %A1, %B2 \n\t" "mov r27, r1 \n\t" "mul %B1, %C2 \n\t" "movw %A0, r0 \n\t" "mul %C1, %C2 \n\t" "add %B0, r0 \n\t" "mul %C1, %B2 \n\t" "add %A0, r0 \n\t" "adc %B0, r1 \n\t" "mul %A1, %C2 \n\t" "add r27, r0 \n\t" "adc %A0, r1 \n\t" "adc %B0, r26 \n\t" "mul %B1, %B2 \n\t" "add r27, r0 \n\t" "adc %A0, r1 \n\t" "adc %B0, r26 \n\t" "mul %C1, %A2 \n\t" "add r27, r0 \n\t" "adc %A0, r1 \n\t" "adc %B0, r26 \n\t" "mul %B1, %A2 \n\t" "add r27, r1 \n\t" "adc %A0, r26 \n\t" "adc %B0, r26 \n\t" "lsr r27 \n\t" "adc %A0, r26 \n\t" "adc %B0, r26 \n\t" "clr r1 \n\t" : "=&r" (acc_step_rate) : "d" (acceleration_time), "d" (current_block->acceleration_rate) : "r26" , "r27" );
      acc_step_rate += current_block->initial_rate;
      
      // upper limit
      if(acc_step_rate > current_block->nominal_rate)
        acc_step_rate = current_block->nominal_rate;

      // step_rate to timer interval
      timer = calc_timer(acc_step_rate);
      
        
      
      acceleration_time += timer;
      OCR1A = timer;
    } 
    else if (step_events_completed > current_block->decelerate_after) {   
      asm volatile ( "clr r26 \n\t" "mul %A1, %B2 \n\t" "mov r27, r1 \n\t" "mul %B1, %C2 \n\t" "movw %A0, r0 \n\t" "mul %C1, %C2 \n\t" "add %B0, r0 \n\t" "mul %C1, %B2 \n\t" "add %A0, r0 \n\t" "adc %B0, r1 \n\t" "mul %A1, %C2 \n\t" "add r27, r0 \n\t" "adc %A0, r1 \n\t" "adc %B0, r26 \n\t" "mul %B1, %B2 \n\t" "add r27, r0 \n\t" "adc %A0, r1 \n\t" "adc %B0, r26 \n\t" "mul %C1, %A2 \n\t" "add r27, r0 \n\t" "adc %A0, r1 \n\t" "adc %B0, r26 \n\t" "mul %B1, %A2 \n\t" "add r27, r1 \n\t" "adc %A0, r26 \n\t" "adc %B0, r26 \n\t" "lsr r27 \n\t" "adc %A0, r26 \n\t" "adc %B0, r26 \n\t" "clr r1 \n\t" : "=&r" (step_rate) : "d" (deceleration_time), "d" (current_block->acceleration_rate) : "r26" , "r27" );
      
      if(step_rate > acc_step_rate) { // Check step_rate stays positive
        step_rate = current_block->final_rate;
      }
      else {
        step_rate = acc_step_rate - step_rate; // Decelerate from aceleration end point.
      }

      // lower limit
      if(step_rate < current_block->final_rate)
        step_rate = current_block->final_rate;

      // step_rate to timer interval
      timer = calc_timer(step_rate);
      
        
        
          
      
      deceleration_time += timer;
      OCR1A = timer;
    }       
    // If current block is finished, reset pointer 
    if (step_events_completed >= current_block->step_event_count) {
      current_block = NULL;
      plan_discard_current_block();
    }   
  } 
  cli(); // disable interrupts
  busy=false;
}


  
  
  
  
  
    
    
    
    
    
      
      
      
    
    
      
      
      
    
    
    
  


void st_init()
{
    //Initialize Dir Pins
  
    do {DIO23_DDR |= (1 << DIO23_PIN); } while (0);
  
  
    do {DIO33_DDR |= (1 << DIO33_PIN); } while (0);
  
  
    do {DIO39_DDR |= (1 << DIO39_PIN); } while (0);
  
  
    do {DIO45_DDR |= (1 << DIO45_PIN); } while (0);
  

  //Initialize Enable Pins - steppers default to disabled.

  
    do {DIO27_DDR |= (1 << DIO27_PIN); } while (0);
    if(!0) do { if (HIGH) {DIO27_WPORT |= (1 << DIO27_PIN); } else {DIO27_WPORT &= ~(1 << DIO27_PIN); }; } while (0);
  
  
    do {DIO29_DDR |= (1 << DIO29_PIN); } while (0);
    if(!0) do { if (HIGH) {DIO29_WPORT |= (1 << DIO29_PIN); } else {DIO29_WPORT &= ~(1 << DIO29_PIN); }; } while (0);
  
  
    do {DIO35_DDR |= (1 << DIO35_PIN); } while (0);
    if(!0) do { if (HIGH) {DIO35_WPORT |= (1 << DIO35_PIN); } else {DIO35_WPORT &= ~(1 << DIO35_PIN); }; } while (0);
  
  
    do {DIO41_DDR |= (1 << DIO41_PIN); } while (0);
    if(!0) do { if (HIGH) {DIO41_WPORT |= (1 << DIO41_PIN); } else {DIO41_WPORT &= ~(1 << DIO41_PIN); }; } while (0);
  

  //endstops and pullups
  
    
      do {DIO22_DDR &= ~(1 << DIO22_PIN); } while (0); 
      do { if (HIGH) {DIO22_WPORT |= (1 << DIO22_PIN); } else {DIO22_WPORT &= ~(1 << DIO22_PIN); }; } while (0);
    
    
      do {DIO24_DDR &= ~(1 << DIO24_PIN); } while (0); 
      do { if (HIGH) {DIO24_WPORT |= (1 << DIO24_PIN); } else {DIO24_WPORT &= ~(1 << DIO24_PIN); }; } while (0);
    
    
      do {DIO26_DDR &= ~(1 << DIO26_PIN); } while (0); 
      do { if (HIGH) {DIO26_WPORT |= (1 << DIO26_PIN); } else {DIO26_WPORT &= ~(1 << DIO26_PIN); }; } while (0);
    
    
      do {DIO28_DDR &= ~(1 << DIO28_PIN); } while (0); 
      do { if (HIGH) {DIO28_WPORT |= (1 << DIO28_PIN); } else {DIO28_WPORT &= ~(1 << DIO28_PIN); }; } while (0);
    
    
      do {DIO30_DDR &= ~(1 << DIO30_PIN); } while (0); 
      do { if (HIGH) {DIO30_WPORT |= (1 << DIO30_PIN); } else {DIO30_WPORT &= ~(1 << DIO30_PIN); }; } while (0);
    
    
      do {DIO32_DDR &= ~(1 << DIO32_PIN); } while (0); 
      do { if (HIGH) {DIO32_WPORT |= (1 << DIO32_PIN); } else {DIO32_WPORT &= ~(1 << DIO32_PIN); }; } while (0);
    
  
    
      
    
    
      
    
    
      
    
    
      
    
    
      
    
    
      
    
  
 

  //Initialize Step Pins
  
    do {DIO25_DDR |= (1 << DIO25_PIN); } while (0);
  
  
    do {DIO31_DDR |= (1 << DIO31_PIN); } while (0);
  
  
    do {DIO37_DDR |= (1 << DIO37_PIN); } while (0);
  
  
    do {DIO43_DDR |= (1 << DIO43_PIN); } while (0);
  

  // waveform generation = 0100 = CTC
  TCCR1B &= ~(1<<WGM13);
  TCCR1B |=  (1<<WGM12);
  TCCR1A &= ~(1<<WGM11); 
  TCCR1A &= ~(1<<WGM10);

  // output mode = 00 (disconnected)
  TCCR1A &= ~(3<<COM1A0); 
  TCCR1A &= ~(3<<COM1B0); 
  TCCR1B = (TCCR1B & ~(0x07<<CS10)) | (2<<CS10); // 2MHz timer

  OCR1A = 0x4000;
  TIMSK1 &= ~(1<<OCIE1A);  

  
    
    
  
  sei();
}

// Block until all buffered steps are executed
void st_synchronize()
{
  while(plan_get_current_block()) {
    manage_heater();
    manage_inactivity(1);
    lcd_status();
  }   
}
