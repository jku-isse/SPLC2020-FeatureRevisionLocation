
#define __EEPROMH 

#include "Marlin.h"
#include "planner.h"
#include "temperature.h"
#include <EEPROM.h>

template <class T> int EEPROM_writeAnything(int &ee, const T& value)
{
  const byte* p = (const byte*)(const void*)&value;
  int i;
  for (i = 0; i < (int)sizeof(value); i++)
    EEPROM.write(ee++, *p++);
  return i;
}

template <class T> int EEPROM_readAnything(int &ee, T& value)
{
  byte* p = (byte*)(void*)&value;
  int i;
  for (i = 0; i < (int)sizeof(value); i++)
    *p++ = EEPROM.read(ee++);
  return i;
}
//======================================================================================

#define SERIAL_ECHOPAIR(name, value) {SERIAL_ECHOPGM(name);SERIAL_ECHO(value);}



#define EEPROM_OFFSET 100


// IMPORTANT:  Whenever there are changes made to the variables stored in EEPROM
// in the functions below, also increment the version number. This makes sure that
// the default values are used whenever there is a change to the data, to prevent
// wrong data being written to the variables.
// ALSO:  always make sure the variables in the Store and retrieve sections are in the same order.
#define EEPROM_VERSION "V04"

inline void StoreSettings() 
{

  
  
  
  
  
  
  
  
  
  
  
  
  
  
    
    
    
  
    
    
    
  
  
  
  
  
  

}

inline void RetrieveSettings(bool def=false)
{  // if def=true, the default values will be used
  
    
    
    
    
    
    
    
      
      
      
      
      
      
      
      
      
      
      
        
      
      
      
      

      
      
    
    
    
      
      
      
      
      
        
        
        
      
      
      
      
      
      
      
      
      
      
    
  
    
      
      
      
      
      
      
      
      
    
      
      
      
      
      
      
      
    
      
      
      
      
      
      
      
    
      
      
      
      
      
    
      
      
      
      
      
      
      
      
    
      
      
      
      
      
      
      
    
  
    
  
}  




