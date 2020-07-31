
#define __MARLINH 

// Tonokip RepRap firmware rewrite based off of Hydra-mmm firmware.
// Licence: GPL
#include <WProgram.h>
#include "fastio.h"


#define ECHO(x) Serial << "echo: " << x;
#define ECHOLN(x) Serial << "echo: "<<x<<endl;

void get_command();
void process_commands();

void manage_inactivity(byte debug);


#define enable_x() WRITE(X_ENABLE_PIN, X_ENABLE_ON)
#define disable_x() WRITE(X_ENABLE_PIN,!X_ENABLE_ON)





#define enable_y() WRITE(Y_ENABLE_PIN, Y_ENABLE_ON)
#define disable_y() WRITE(Y_ENABLE_PIN,!Y_ENABLE_ON)





#define enable_z() WRITE(Z_ENABLE_PIN, Z_ENABLE_ON)
#define disable_z() WRITE(Z_ENABLE_PIN,!Z_ENABLE_ON)







	#define enable_e() WRITE(E_ENABLE_PIN, E_ENABLE_ON)
	#define disable_e() WRITE(E_ENABLE_PIN,!E_ENABLE_ON)






#define X_AXIS 0
#define Y_AXIS 1
#define Z_AXIS 2
#define E_AXIS 3

void FlushSerialRequestResend();
void ClearToSend();

void get_coordinates();
void prepare_move();
void kill(byte debug);

//void check_axes_activity();
//void plan_init();
//void st_init();
//void tp_init();
//void plan_buffer_line(float x, float y, float z, float e, float feed_rate);
//void plan_set_position(float x, float y, float z, float e);
//void st_wake_up();
//void st_synchronize();
void enquecommand(const char *cmd);
void wd_reset();


#define CRITICAL_SECTION_START unsigned char _sreg = SREG; cli();
#define CRITICAL_SECTION_END SREG = _sreg;


extern float homing_feedrate[];
extern bool axis_relative_modes[];

void manage_inactivity(byte debug);


