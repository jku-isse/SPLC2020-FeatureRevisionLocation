
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

  char ver[4]= "000";
  int i=100;
  EEPROM_writeAnything(i,ver); // invalidate data first 
  EEPROM_writeAnything(i,axis_steps_per_unit);  
  EEPROM_writeAnything(i,max_feedrate);  
  EEPROM_writeAnything(i,max_acceleration_units_per_sq_second);
  EEPROM_writeAnything(i,acceleration);
  EEPROM_writeAnything(i,retract_acceleration);
  EEPROM_writeAnything(i,minimumfeedrate);
  EEPROM_writeAnything(i,mintravelfeedrate);
  EEPROM_writeAnything(i,minsegmenttime);
  EEPROM_writeAnything(i,max_xy_jerk);
  EEPROM_writeAnything(i,max_z_jerk);
  
    EEPROM_writeAnything(i,Kp);
    EEPROM_writeAnything(i,Ki);
    EEPROM_writeAnything(i,Kd);
  
    
    
    
  
  char ver2[4]="V04";
  i=100;
  EEPROM_writeAnything(i,ver2); // validate data
  serialprintPGM(echomagic);;
  {serialprintPGM(PSTR("Settings Stored"));Serial.write('\n');};

}

inline void RetrieveSettings(bool def=false)
{  // if def=true, the default values will be used
  
    int i=100;
    char stored_ver[4];
    char ver[4]="V04";
    EEPROM_readAnything(i,stored_ver); //read stored version
    //  SERIAL_ECHOLN("Version: [" << ver << "] Stored version: [" << stored_ver << "]");
    if ((!def)&&(strncmp(ver,stored_ver,3)==0)) 
    {   // version number match
      EEPROM_readAnything(i,axis_steps_per_unit);  
      EEPROM_readAnything(i,max_feedrate);  
      EEPROM_readAnything(i,max_acceleration_units_per_sq_second);
      EEPROM_readAnything(i,acceleration);
      EEPROM_readAnything(i,retract_acceleration);
      EEPROM_readAnything(i,minimumfeedrate);
      EEPROM_readAnything(i,mintravelfeedrate);
      EEPROM_readAnything(i,minsegmenttime);
      EEPROM_readAnything(i,max_xy_jerk);
      EEPROM_readAnything(i,max_z_jerk);
      
        
      
      EEPROM_readAnything(i,Kp);
      EEPROM_readAnything(i,Ki);
      EEPROM_readAnything(i,Kd);

      serialprintPGM(echomagic);;
      {serialprintPGM(PSTR("Stored settings retreived:"));Serial.write('\n');};
    }
    else 
    {
      float tmp1[]={79.87220447,79.87220447,200*8/3,14};
      float tmp2[]={160*60, 160*60, 10*60, 500000};
      long tmp3[]={9000,9000,150,10000};
      for (short i=0;i<4;i++) 
      {
        axis_steps_per_unit[i]=tmp1[i];  
        max_feedrate[i]=tmp2[i];  
        max_acceleration_units_per_sq_second[i]=tmp3[i];
      }
      acceleration=3000;
      retract_acceleration=7000;
      minimumfeedrate=10;
      minsegmenttime=20000;       
      mintravelfeedrate=10;
      max_xy_jerk=30.0*60;
      max_z_jerk=10.0*60;
      serialprintPGM(echomagic);;
      {Serial.print("Using Default settings:");Serial.write('\n');};
    }
  
    serialprintPGM(echomagic);;
      {serialprintPGM(PSTR("Steps per unit:"));Serial.write('\n');};
      serialprintPGM(echomagic);;
      {serialprintPGM(PSTR("  M92 X"));;Serial.print(axis_steps_per_unit[0]);;};
      {serialprintPGM(PSTR(" Y"));;Serial.print(axis_steps_per_unit[1]);;};
      {serialprintPGM(PSTR(" Z"));;Serial.print(axis_steps_per_unit[2]);;};
      {serialprintPGM(PSTR(" E"));;Serial.print(axis_steps_per_unit[3]);;};
      {Serial.print("");Serial.write('\n');};
      
    serialprintPGM(echomagic);;
      {serialprintPGM(PSTR("Maximum feedrates (mm/s):"));Serial.write('\n');};
      serialprintPGM(echomagic);;
      {serialprintPGM(PSTR("  M203 X"));;Serial.print(max_feedrate[0]/60);;};
      {serialprintPGM(PSTR(" Y"));;Serial.print(max_feedrate[1]/60);;}; 
      {serialprintPGM(PSTR(" Z"));;Serial.print(max_feedrate[2]/60);;}; 
      {serialprintPGM(PSTR(" E"));;Serial.print(max_feedrate[3]/60);;};
      {Serial.print("");Serial.write('\n');};
    serialprintPGM(echomagic);;
      {serialprintPGM(PSTR("Maximum Acceleration (mm/s2):"));Serial.write('\n');};
      serialprintPGM(echomagic);;
      {serialprintPGM(PSTR("  M201 X"));;Serial.print(max_acceleration_units_per_sq_second[0]);;}; 
      {serialprintPGM(PSTR(" Y"));;Serial.print(max_acceleration_units_per_sq_second[1]);;}; 
      {serialprintPGM(PSTR(" Z"));;Serial.print(max_acceleration_units_per_sq_second[2]);;};
      {serialprintPGM(PSTR(" E"));;Serial.print(max_acceleration_units_per_sq_second[3]);;};
      {Serial.print("");Serial.write('\n');};
    serialprintPGM(echomagic);;
      {serialprintPGM(PSTR("Acceleration: S=acceleration, T=retract acceleration"));Serial.write('\n');};
      serialprintPGM(echomagic);;
      {serialprintPGM(PSTR("  M204 S"));;Serial.print(acceleration);;}; 
      {serialprintPGM(PSTR(" T"));;Serial.print(retract_acceleration);;};
      {Serial.print("");Serial.write('\n');};
    serialprintPGM(echomagic);;
      {serialprintPGM(PSTR("Advanced variables: S=Min feedrate (mm/s), T=Min travel feedrate (mm/s), B=minimum segment time (ms), X=maximum xY jerk (mm/s),  Z=maximum Z jerk (mm/s)"));Serial.write('\n');};
      serialprintPGM(echomagic);;
      {serialprintPGM(PSTR("  M205 S"));;Serial.print(minimumfeedrate/60);;}; 
      {serialprintPGM(PSTR(" T"));;Serial.print(mintravelfeedrate/60);;}; 
      {serialprintPGM(PSTR(" B"));;Serial.print(minsegmenttime);;}; 
      {serialprintPGM(PSTR(" X"));;Serial.print(max_xy_jerk/60);;}; 
      {serialprintPGM(PSTR(" Z"));;Serial.print(max_z_jerk/60);;};
      {Serial.print("");Serial.write('\n');}; 
    
      serialprintPGM(echomagic);;
      {serialprintPGM(PSTR("PID settings:"));Serial.write('\n');};
      serialprintPGM(echomagic);;
      {serialprintPGM(PSTR("   M301 P"));;Serial.print(Kp);;}; 
      {serialprintPGM(PSTR(" I"));;Serial.print(Ki);;}; 
      {serialprintPGM(PSTR(" D"));;Serial.print(Kd);;};
      {Serial.print("");Serial.write('\n');}; 
    
  
    
  
}  




