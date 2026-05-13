/**
 * 
 * @file main.h
 * @author Antoine.F
 * @brief this file contains function prototypes, macros and global variables definitions for the main.c file
 * @version 0.1
 * @date 07/05/2026
 * @copyright Copyright (c) 2026
 * 
 * 
 */
 

// NIOSII header files

#include "system.h"
#include "alt_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <altera_avalon_pio_regs.h>
#include <altera_avalon_timer.h>
#include "sys/alt_alarm.h"
#include "sys/alt_irq.h"
#include <altera_avalon_timer_regs.h>




// Key definitions

#define NO_KEY 	0
#define KEY_0 	1
#define KEY_1 	2
#define KEY_0_1	3

// time format definitions

#define FORMAT_24H 0
#define FORMAT_12H 1

// 7 segment display definitions

#define HEX_0		0b1000000
#define HEX_1		0b1111001
#define HEX_2		0b0100100
#define HEX_3		0b0110000
#define HEX_4		0b0011001
#define HEX_5		0b0010010
#define HEX_6		0b0000010
#define HEX_7		0b1111000
#define HEX_8		0b0000000
#define HEX_9		0b0010000
#define NEGATIVE	0b0111111
#define NO_NUM		0b1111111
#define SEG_A		0b0001000
#define SEG_P		0b0001100
#define SEG_M		0b1001000
alt_u8 conversion_array[10] = {HEX_0, HEX_1, HEX_2, HEX_3, HEX_4, HEX_5, HEX_6, HEX_7, HEX_8, HEX_9};

/**
 * @brief Error codes
 */
enum error_codes
{
	ERR_OK = 0,
	ERR_INVALID_KEY,
	ERR_INVALID_SW,
	ERR_INVALID_TIME,
	ERR_ALARM_TIME_NOT_SET,
	ERR_ALARM_NOT_SET,
	ERR_ALARM_ALREADY_SET,
	ERR_OVERFLOW,
	ERR_LAUNCH_ALARM,
	ERR_DELAY_WATCHDOG,
	ERR_WRONG_MELODY,
};


// NIOSII PIO addresses

volatile int * LED_ptr = (int *)LED_R_BASE; // LED address
volatile int * SW_switch_ptr = (int *)SWITCHES_2POS_BASE; // SW slider address
volatile int * KEY_ptr = (int *)PUSH_BUTTONS_BASE; // pushbutton KEY address
volatile int * HEX3_HEX0_ptr = (int *)BCD3_BCD0_BASE; // HEX3_HEX0 address
volatile int * HEX5_HEX4_ptr = (int *)BCD5_BCD4_BASE; // HEX3_HEX0 address
volatile int * HP_ptr = (int *)HP_OUT_BASE; // HEX3_HEX0 address



// NIOSII Alarms

static alt_alarm internal_alarm; 	// internal alarm to update the internal time every second and check if the alarm time is reached
static alt_alarm hp_alarm;			// alarm for the sound generation
static alt_alarm delay_alarm;		// alarm for accurate blocking delay


/**
 * @brief structure to store the time in a formatted way to be displayed on the 6 7seg displays
 */
typedef struct display_img_s
{
	alt_u8 seconds;
	alt_u8 minutes;
	alt_u8 hours;

	alt_u8 bcd_sec_0;
	alt_u8 bcd_sec_1;

	alt_u8 bcd_min_0;
	alt_u8 bcd_min_1;

	alt_u8 bcd_hou_0;
	alt_u8 bcd_hou_1;
}display_img;
display_img display;


// variables definitions

alt_u32 internal_time = 0;      // time register incremented every second
alt_u32 alarm_time = 0;         // time register to set the alarm
alt_u32 HEX_bits = 0;         	// pattern for HEX displays
alt_u32 LED_bits = 0;          	// pattern for LED lights
alt_u16 SW_value = 0;           // variable to store the value of the SW slider switches
alt_u8 KEY_value = 0;           // variable to store the value of the push button keys
alt_u16 melody_freq = 0;		// variable to store the frequency of the note to be played
alt_u8 hp_output_state = 0;		// variable to store the state of the HP output (0 or 1) to generate a square wave
alt_u8 select_melody = 0;		// variable to store the selected melody
alt_u8 time_format = FORMAT_24H;	// flag to store the time format


// flag definitions 

alt_u8 delay_alarm_flag = 0;		// flag to indicate that the blocking delay is over
alt_u8 launch_alarm_flag = 0;		// flag to indicate that the alarm time is reached and the alarm should be launched
alt_u8 hp_alarm_en = 0;				// flag to indicate that the hp_alarm is enabled to generate the sound (used to stop the hp_alarm when the melody is over or when a key is pressed to stop the alarm)
alt_u8 hp_alarm_flag = 0;			// flag to indicate that the hp_alarm callback function is called and the sound should be generated
alt_u8 alarm_state = 0;         	// flag to indicate if the alarm is activated
alt_u8 alarm_set = 0;           	// flag to display the alarm time on the 6 7seg displays
alt_u8 internal_time_set = 0;   	// flag to display the modified time on the 6 7seg displays


// function prototypes

alt_u32 internal_alarm_callback (void* context);
alt_u32 hp_alarm_callback (void* context);
alt_u32 delay_alarm_callback (void* context);

alt_u8 get_key(void);
alt_u8 get_switch(void);
alt_u8 hp_out(alt_u16 *melody, alt_u16 melody_count);
alt_u8 time_2_hhmmss(alt_u32 time, alt_u8 *hour, alt_u8 *min, alt_u8 *sec);
alt_u8 bin_2_bcd(alt_u8 bin, alt_u8 *decimal, alt_u8 *unit);
alt_u8 update_display(alt_u32 time, alt_u8 format);
alt_u8 activate_alarm(void);
alt_u8 deactivate_alarm(void);
alt_u8 set_time(alt_u32 *time);
alt_u8 delay(alt_u16 delay_ms);
alt_u8 launch_alarm(void);
alt_u8 user_timer_setup(void);
alt_u8 set_user_timer(alt_16 frequency);



