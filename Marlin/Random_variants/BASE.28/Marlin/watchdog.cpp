

#include <avr/wdt.h>
#include <avr/interrupt.h>

volatile uint8_t timeout_seconds=0;

void(* ctrlaltdelete) (void) = 0; //does not work on my atmega2560

//Watchdog timer interrupt, called if main program blocks >1sec
ISR(WDT_vect) 
{ 
  if(timeout_seconds++ >= 4)
  {
 
    
      lcd_status("Please Reset!");;
      Serial << "echo: "<<"echo_: Something is wrong, please turn off the printer."<<endl;;
    
      
    
    //disable watchdog, it will survife reboot.
    WDTCSR |= (1<<WDCE) | (1<<WDE);
    WDTCSR = 0;
    
      kill(); //kill blocks
      while(1); //wait for user or serial reset
    
      
    
  }
}

/// intialise watch dog with a 1 sec interrupt time
void wd_init() 
{
  WDTCSR = (1<<WDCE )|(1<<WDE ); //allow changes
  WDTCSR = (1<<WDIF)|(1<<WDIE)| (1<<WDCE )|(1<<WDE )|  (1<<WDP2 )|(1<<WDP1)|(0<<WDP0);
}

/// reset watchdog. MUST be called every 1s after init or avr will reset.
void wd_reset() 
{
  wdt_reset();
  timeout_seconds=0; //reset counter for resets
}


