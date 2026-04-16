/*
 * Reveil.c
 *
 *  Created on: 12 mars 2026
 *      Author: afrayard
 */



#include "system.h"
#include "alt_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <altera_avalon_pio_regs.h>
#include <altera_avalon_timer.h>
#include "sys/alt_alarm.h"


alt_u32 time1;
alt_u32 time2;
alt_u32 time3;


#define NO_KEY 	0
#define KEY_0 	1
#define KEY_1 	2
#define KEY_0_1	3

#define FORMAT_24H 0
#define FORMAT_12H 1


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

/**

DEBUG macros

*/
// #define DEBUG
#define INFO
#define FASTCLOCK
#ifdef FASTCLOCK
#define FASTCLOCK_FREQ 20
#endif


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

alt_u32 internal_time = 0;
alt_u32 alarm_time = 0;
display_img display;
alt_u8 alarm_state = 0;
alt_u8 alarm_set = 0;
alt_u8 internal_time_set = 0;

//
// fonction prototypes
//

alt_u8 get_key(void);
alt_u8 get_switch(void);
int print_result(void);
int hp_out(void);
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


static alt_alarm internal_alarm;
static alt_alarm user_alarm;



volatile int * LED_ptr = (int *)LED_R_BASE; // LED address
volatile int * SW_switch_ptr = (int *)SWITCHES_2POS_BASE; // SW slider address
volatile int * KEY_ptr = (int *)PUSH_BUTTONS_BASE; // pushbutton KEY address
volatile int * HEX3_HEX0_ptr = (int *)BCD3_BCD0_BASE; // HEX3_HEX0 address
volatile int * HEX5_HEX4_ptr = (int *)BCD5_BCD4_BASE; // HEX3_HEX0 address
volatile int * HP_ptr = (int *)HP_OUT_BASE; // HEX3_HEX0 address
int HEX_bits = 0x0; // initial pattern for HEX displays
int LED_bits = 0x0; // initial pattern for LED lights
alt_u16 SW_value;
alt_u8 KEY_value;
int press, delay_count = 0;
int pressed_key = NO_KEY;
int op_LSB = 0;
int op_MSB = 0;
int result = 0;
int cen = 0;
int dec = 0;
int unit = 0;

int convertion_array[10] = {HEX_0, HEX_1, HEX_2, HEX_3, HEX_4, HEX_5, HEX_6, HEX_7, HEX_8, HEX_9};

alt_u8 test_var = 8;


/* MAIN FUNCTION */
int main(void) {
#ifdef FASTCLOCK
	printf("WARINING FASTCLOCK IS ENABLED\n");
	if (alt_alarm_start (&internal_alarm, alt_ticks_per_second()/FASTCLOCK_FREQ, internal_alarm_callback, NULL) < 0)
#else
	if (alt_alarm_start (&internal_alarm, alt_ticks_per_second(), internal_alarm_callback, NULL) < 0)
#endif
	{
		printf ("No system clock available\n");
	}

	/**
	 * user alarm
	 */
	if (alt_alarm_start (&user_alarm, 10000, user_alarm_callback, NULL) < 0)
	{
		printf ("No system clock available\n");
	}
	printf("NIOSII started !!!");
	for(;;)
	{
		get_key();
		get_switch();
		

		if(SW_value & 0b0000000001)
		{
			alarm_state = 1;
			activate_alarm();
		}
		else
		{
			alarm_state = 0;
			deactivate_alarm();
		}
		
		if (SW_value & 0b0000000010)
		{
			alarm_set = 1;
			set_alarm_time();
		}
		else
		{
			alarm_set = 0;
		}

		if (SW_value & 0b0000000100)
		{
			internal_time_set = 1;
			set_internal_time();
		}
		else
		{
			internal_time_set = 0;
		}

		printf("\nalarm state = %d", alarm_state);
		printf("\nset alarm = %d", alarm_set);
		

		for (delay_count = 20000; delay_count != 0; --delay_count); // delay loop
	}
    return 0;
}


/* The alt_alarm must persist for the duration of the alarm. */


/*
* The callback function.
*/
alt_u32 internal_alarm_callback (void* context)
{
	if(!internal_time_set)
	{
		internal_time++;
	}
	if (alarm_set)
	{
		update_display(alarm_time, FORMAT_12H);
	}
	else
	{
		display_current_time();
	}
	if(alarm_state)
	{
		if(internal_time == alarm_time)
		{
			hp_out();
		}
	}
	if(internal_time >= 86400)
	{
		internal_time = 0;
	}
#ifdef FASTCLOCK
	return alt_ticks_per_second()/FASTCLOCK_FREQ;
#else
	return alt_ticks_per_second();
#endif
}

alt_u32 user_alarm_callback (void* context)
{
	//hp_out();
	printf("\nuser alarm");
	return 10000;
}

/**
 * store the switch register value in "SW_value"
 */
alt_u8 get_switch(void)
{
	SW_value = IORD_ALTERA_AVALON_PIO_DATA(SW_switch_ptr); //*(SW_switch_ptr); // read the SW slider switch values
#ifdef DEBUG
	printf("\n switches -> %x", SW_value);
#endif
    return 0;
}

/**
 * store the push button (key) register value in "KEY_value"
 */
alt_u8 get_key(void)
{
	KEY_value = IORD_ALTERA_AVALON_PIO_DATA(KEY_ptr);
#ifdef DEBUG
	printf("\n switches -> %d", KEY_value);
#endif
    return 0;
}

int print_result(void)
{
	if (result < 0)
	{
		HEX_bits = NEGATIVE;
		HEX_bits = HEX_bits << 8;
	}
	else
	{
		HEX_bits = NO_NUM;
		HEX_bits = HEX_bits << 8;
	}
	cen = result / 100;
	dec = (result- cen*100) / 10;
	unit = result - (dec*10) - (cen*100);

	printf("MSB = ");
	printf("%d + %d + %d \n", cen*100, dec*10, unit);
	HEX_bits = HEX_bits + convertion_array[abs(cen)];
	HEX_bits = HEX_bits << 8;
	HEX_bits = HEX_bits + convertion_array[abs(dec)];
	HEX_bits = HEX_bits << 8;
	HEX_bits = HEX_bits + convertion_array[abs(unit)];

    IOWR_ALTERA_AVALON_PIO_DATA(HEX5_HEX4_ptr, HEX_4);
	IOWR_ALTERA_AVALON_PIO_DATA(HEX3_HEX0_ptr, HEX_bits); //*(HEX3_HEX0_ptr) = HEX_bits; // display pattern on HEX3 ... HEX0
	//IOWR_ALTERA_AVALON_PIO_DATA(LED_ptr, LED_bits); //*(LED_ptr)= LED_bits;
    return 0;
}

/**
 * allow the user to set the alarm time using the 2 push button the the board
 */
alt_u8 set_alarm_time(void)
{
#ifdef INFO
	printf("\n alarm_time = %d", alarm_time);
#endif
	while(SW_value & 0b0000000010)
	{
		get_switch();
		get_key();
		switch (KEY_value)
		{
		case NO_KEY:
			break;

		case KEY_0:
			alarm_time = alarm_time - 60; // add 1 hour

		case KEY_0_1:
			alarm_time = alarm_time + 3600; // add 1 minute

		case KEY_1:
			alarm_time = alarm_time + 60; // remove 1 minute

		default:
			break;
		}
		printf("\nalarm_time = %d", alarm_time);
		for (delay_count = 200000; delay_count != 0; --delay_count); // delay loop
		if(alarm_time >= 86400)
		{
			alarm_time = 0;
		}
	}
}

alt_u8 set_internal_time(void)
{
#ifdef INFO
	printf("\n internal_time = %d", internal_time);
#endif
	while(SW_value & 0b0000000100)
	{
		get_switch();
		get_key();
		switch (KEY_value)
		{
		case NO_KEY:
			break;

		case KEY_0:
			internal_time = internal_time - 60; // add 1 hour

		case KEY_0_1:
			internal_time = internal_time + 3600; // add 1 minute

		case KEY_1:
			internal_time = internal_time + 60; // remove 1 minute

		default:
			break;
		}
		printf("\n internal_time = %d", internal_time);
		for (delay_count = 200000; delay_count != 0; --delay_count); // delay loop
		if(internal_time >= 86400)
		{
			internal_time = 0;
		}
	}
}

alt_u8 activate_alarm(void)
{
	LED_bits = 0b000000001;
	IOWR_ALTERA_AVALON_PIO_DATA(LED_ptr, LED_bits);
	return 0;
}

alt_u8 deactivate_alarm(void)
{
	LED_bits = 0b000000000;
	IOWR_ALTERA_AVALON_PIO_DATA(LED_ptr, LED_bits);
	return 0;
}

/**
 * display the internal time on the 6 7seg displays
 */
alt_u8 display_current_time(void)
{
	update_display(internal_time, FORMAT_12H);
#ifdef INFO
	printf("\n interal time = %d", (int)internal_time);
	printf("\n%d - %d - %d", (int)display.hours, (int)display.minutes, (int)display.seconds);
#endif


#ifdef DEBUG
	printf("\n%d%d", display.bcd_hou_1, display.bcd_hou_0);
#endif
	return 0;
}

/**
 * convert 32bits int in formated time structure
 */
alt_u8 time_2_hhmmss(alt_u32 time, alt_u8 *hour, alt_u8 *min, alt_u8 *sec)
{
	*hour = time / 3600;
	*min = (time % 3600)/60;
	*sec = ((time % 3600) % 60);
#ifdef DEBUG
	printf("\ntime_2_hhmmss");
	printf("\ntime = %d", time);
#endif
	return 0;
}


/**
 * convert 8bits hex in 2 8bits BCD
 */
alt_u8 time_2_bcd(alt_u8 time, alt_u8 *decimal, alt_u8 *unit)
{

	*decimal = time / 10;
	*unit = time % 10;
#ifdef DEBUG
	printf("\ntime 4 bcd = %d", time);
	printf("\ndec 4 bcd = %d", *decimal);
	printf("\nunit 4 bcd = %d", *unit);
#endif
	return 0;
}

/**
 * transforme the 32bits value to time format and diplay it on the 6 7seg displays
 */
alt_u8 update_display(alt_u32 time, alt_u8 format)
{
	time_2_hhmmss(time, &display.hours, &display.minutes, &display.seconds);
	time_2_bcd(display.seconds, &display.bcd_sec_1, &display.bcd_sec_0);
	time_2_bcd(display.minutes, &display.bcd_min_1, &display.bcd_min_0);
	if (format)
	{
		if(display.hours > 11)
		{
			display.hours = display.hours - 12;
		}
	}
	
	time_2_bcd(display.hours, &display.bcd_hou_1, &display.bcd_hou_0);
#ifdef DEBUG
	printf("\nupdate display ");
	printf("%d", display.bcd_sec_0);
#endif
	if(format)
	{
		/* HOURS */
		HEX_bits = convertion_array[abs(display.bcd_hou_0)];
		HEX_bits = HEX_bits << 8;
		HEX_bits = HEX_bits + convertion_array[abs(display.bcd_hou_1)];
		IOWR_ALTERA_AVALON_PIO_DATA(HEX5_HEX4_ptr, HEX_bits);

		/* MINUTES */
		HEX_bits = convertion_array[abs(display.bcd_min_1)];
		HEX_bits = HEX_bits << 8;
		HEX_bits = HEX_bits + convertion_array[abs(display.bcd_min_0)];
		HEX_bits = HEX_bits << 8;

		/* SECONDS */
		HEX_bits = HEX_bits + convertion_array[abs(display.bcd_sec_1)];
		HEX_bits = HEX_bits << 8;
		HEX_bits = HEX_bits + convertion_array[abs(display.bcd_sec_0)];
		IOWR_ALTERA_AVALON_PIO_DATA(HEX3_HEX0_ptr, HEX_bits);
	}
	else
	{
		/* MERRIDIAN */
		HEX_bits = SEG_A;
		HEX_bits = HEX_bits << 8;
		HEX_bits = HEX_bits + SEG_M;
		IOWR_ALTERA_AVALON_PIO_DATA(HEX5_HEX4_ptr, HEX_bits);

		/* HOUR */
		HEX_bits = convertion_array[abs(display.bcd_hou_0)];
		HEX_bits = HEX_bits << 8;
		HEX_bits = HEX_bits + convertion_array[abs(display.bcd_hou_1)];
		HEX_bits = HEX_bits << 8;

		/* MINUTES */
		HEX_bits = HEX_bits + convertion_array[abs(display.bcd_min_1)];
		HEX_bits = HEX_bits << 8;
		HEX_bits = HEX_bits + convertion_array[abs(display.bcd_min_0)];
		IOWR_ALTERA_AVALON_PIO_DATA(HEX3_HEX0_ptr, HEX_bits);
	}
	
	return 0;
}

int hp_out(void)
{
    printf("\nHP_out");
    for (size_t i = 0; i < 250; i++)
    {
        
        IOWR_ALTERA_AVALON_PIO_DATA(HP_ptr, 1);
        for (size_t j = 200; j != 0; --j); // delay loop
        IOWR_ALTERA_AVALON_PIO_DATA(HP_ptr, 0);
        for (size_t j = 200; j != 0; --j); // delay loop
    }
    
    return 0;
}
