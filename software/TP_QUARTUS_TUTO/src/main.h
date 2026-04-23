/**
 * @file main.h
 * @brief This file contains the main function and the prototypes of the functions used in the main.c file
 * @author A. Frayard
 */

/**
 * @brief NIOS II header files
 */
#include "system.h"
#include "alt_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <altera_avalon_pio_regs.h>
#include <altera_avalon_timer.h>
#include "sys/alt_alarm.h"

/**
 * @brief user defined header files
 */



/**
 * @brief Key definitions
 */
#define NO_KEY 	0
#define KEY_0 	1
#define KEY_1 	2
#define KEY_0_1	3

/**
 * @brief time format definitions
 */
#define FORMAT_24H 0
#define FORMAT_12H 1

/**
 * @brief 7 segment display definitions
 */
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
#define SEG_A		0b1000000
#define SEG_P		0b0001000
#define SEG_M		0b1000000
alt_u8 convertion_array[10] = {HEX_0, HEX_1, HEX_2, HEX_3, HEX_4, HEX_5, HEX_6, HEX_7, HEX_8, HEX_9};



/**
 * @brief NIOS II PIO addresses
 */
volatile int * LED_ptr = (int *)LED_R_BASE; // LED address
volatile int * SW_switch_ptr = (int *)SWITCHES_2POS_BASE; // SW slider address
volatile int * KEY_ptr = (int *)PUSH_BUTTONS_BASE; // pushbutton KEY address
volatile int * HEX3_HEX0_ptr = (int *)BCD3_BCD0_BASE; // HEX3_HEX0 address
volatile int * HEX5_HEX4_ptr = (int *)BCD5_BCD4_BASE; // HEX3_HEX0 address
volatile int * HP_ptr = (int *)HP_OUT_BASE; // HEX3_HEX0 address



/**
 * @brief structure to store the time in a formated way to be displayed on the 6 7seg displays
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


alt_u32 internal_time = 0;      // time register incremented every second
alt_u32 alarm_time = 0;         // time register to set the alarm
alt_u8 alarm_state = 0;         // flag to indicate if the alarm is activated
alt_u8 alarm_set = 0;           // flag to display the alarm time on the 6 7seg displays
alt_u8 internal_time_set = 0;   // flag to display the modified time on the 6 7seg displays
alt_u32 HEX_bits = 0x0;         // pattern for HEX displays
alt_u32 LED_bits = 0x0;         // pattern for LED lights
alt_u16 SW_value = 0;           // variable to store the value of the SW slider switches
alt_u8 KEY_value = 0;           // variable to store the value of the push button keys


/*** Function Prototypes ***/

alt_u8 get_key(void);
alt_u8 get_switch(void);
alt_u8 hp_out(void);
alt_u32 internal_alarm_callback (void* context);
alt_u32 user_alarm_callback (void* context);
alt_u8 display_current_time(void);
alt_u8 time_2_hhmmss(alt_u32 time, alt_u8 *hour, alt_u8 *min, alt_u8 *sec);
alt_u8 time_2_bcd(alt_u8 time, alt_u8 *decimal, alt_u8 *unit);
alt_u8 update_display(alt_u32 time, alt_u8 format);
alt_u8 activate_alarm(void);
alt_u8 deactivate_alarm(void);
alt_u8 set_alarm_time(void);
alt_u8 set_internal_time(void);
alt_u8 delay(alt_u16 delay_ms);