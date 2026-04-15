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

/**

DEBUG macros

*/
#define DEBUG
#define FASTCLOCK
#ifdef FASTCLOCK
#define FASTCLOCK_FREQ 10
#endif


typedef struct internal_time_s 
{
	alt_32 time;

	alt_8 seconds;
	alt_8 minutes;
	alt_8 hours;

	alt_8 bcd_sec_0;
	alt_8 bcd_sec_1;

	alt_8 bcd_min_0;
	alt_8 bcd_min_1;

	alt_8 bcd_hou_0;
	alt_8 bcd_hou_1;
}internal_time;

internal_time internal;
internal_time alarm_time;


//
// fonction prototypes
//

int get_key(void);
alt_u16 get_switch(void);
int print_result(void);
int hp_out(void);
alt_u32 internal_alarm_callback (void* context);
alt_u32 user_alarm_callback (void* context);
alt_u8 time_display(void);
alt_u8 time_2_hhmmss(internal_time *time);
alt_u8 time_2_bcd(alt_8 time, alt_8 *decimal, alt_8 *unit);
alt_u8 update_display(internal_time *time);
alt_u8 activate_alarm(void);
alt_u8 foo(alt_u8 *fee);
alt_u8 sub_foo(alt_u8 *fee);

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
alt_u16 SW_value; //, KEY_value;
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
	internal.bcd_sec_1 = 5;
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
		//get_key();
		//get_switch();
		//print_result();
        //hp_out();
		if(SW_value && 0b0000000001);
		{
			activate_alarm();
		}

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
	internal.time++;
	time_display();
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

int get_key(void)
{
	press = IORD_ALTERA_AVALON_PIO_DATA(KEY_ptr); //*(KEY_ptr + 3); // read the pushbutton edge capture register
	//IOWR_ALTERA_AVALON_PIO_EDGE_CAP(KEY_ptr, press); //*(KEY_ptr + 3) = press; // Clear the edge capture register
	switch (press)
	{
	case NO_KEY:
		result = op_LSB + op_MSB;
		break;
	case KEY_0:
		result = op_LSB - op_MSB;
		break;
	case KEY_1:
		result = op_LSB * op_MSB;
		break;
	case KEY_0_1:
		result = op_LSB / op_MSB;
		break;
	
	default:
		break;
	}
	printf("result = ");
	printf("%d\n", result);
    return 0;
}

alt_u16 get_switch(void)
{
	SW_value = IORD_ALTERA_AVALON_PIO_DATA(SW_switch_ptr); //*(SW_switch_ptr); // read the SW slider switch values
	op_LSB = SW_value & 0b0000011111;
	op_MSB = SW_value & 0b1111100000;
	op_MSB = op_MSB >> 5;

#ifdef DEBUG_SW
	printf("LSB = ");
	printf("%d\n", op_LSB);

	printf("MSB = ");
	printf("%d\n", op_MSB);
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

alt_u8 set_alarm_time(void)
{
	return 0;
}

alt_u8 activate_alarm(void)
{
	LED_bits = 0b000000001;
	IOWR_ALTERA_AVALON_PIO_DATA(LED_ptr, LED_bits);
	return 0;
}

alt_u8 time_display(void)
{
	update_display(&internal);
	printf("\n interal time = %d", (int)internal.time);
	printf("\n%d - %d - %d", (int)internal.hours, (int)internal.minutes, (int)internal.seconds);


#ifdef DEBUG
	printf("\n%d%d", internal.bcd_hou_1, internal.bcd_hou_0);
#endif

	/* HOURS */
	HEX_bits = convertion_array[abs(internal.bcd_hou_1)];
	HEX_bits = HEX_bits << 8;
	HEX_bits = HEX_bits + convertion_array[abs(internal.bcd_hou_0)];
	IOWR_ALTERA_AVALON_PIO_DATA(HEX5_HEX4_ptr, HEX_bits);

	/* MINUTES */
	HEX_bits = convertion_array[abs(internal.bcd_min_1)];
	HEX_bits = HEX_bits << 8;
	HEX_bits = HEX_bits + convertion_array[abs(internal.bcd_min_0)];
	HEX_bits = HEX_bits << 8;

	/* SECONDS */
	HEX_bits = HEX_bits + convertion_array[abs(internal.bcd_sec_1)];
	HEX_bits = HEX_bits << 8;
	HEX_bits = HEX_bits + convertion_array[abs(internal.bcd_sec_0)];

	IOWR_ALTERA_AVALON_PIO_DATA(HEX3_HEX0_ptr, HEX_bits);
	return 0;
}

alt_u8 time_2_hhmmss(internal_time *time)
{
	time->hours = time->time / 3600;
	time->minutes = (time->time % 3600)/60;
	time->seconds = ((time->time % 3600) % 60);
#ifdef DEBUG
	printf("\ntime_2_hhmmss");
	printf("\ntime = %d", time);
#endif
	return 0;
}

alt_u8 time_2_bcd(alt_8 time, alt_8 *decimal, alt_8 *unit)
{

	*decimal = time / 10;
	*unit = time % 10;
#ifdef DEBUG
	printf("\ntime 4 bcd = %d", time);
	printf("\ndec 4 bcd = %d", decimal);
	printf("\nunit 4 bcd = %d", unit);
#endif
	return 0;
}

alt_u8 update_display(internal_time *time)
{
	time_2_hhmmss(time);
	time_2_bcd(time->seconds, time->bcd_sec_1, time->bcd_sec_0);
	// time_2_bcd(time.minutes, &time.bcd_min_1, &time.bcd_min_0);
	// time_2_bcd(time.hours, &time.bcd_hou_1, &time.bcd_hou_0);
#ifdef DEBUG
	printf("\nupdate display ");
	printf("%d", internal.bcd_sec_0);
#endif
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



alt_u8 foo(alt_u8 *fee)
{
	printf("\nfoo");
	printf("\nfee = %d", *fee);
	*fee = 15;
	printf("\nfee = %d", *fee);
	printf("\testvar = %d", test_var);
	sub_foo(fee);

}

alt_u8 sub_foo(alt_u8 *fee)
{
	printf("\nsub_foo");
	printf("\nfee = %d", *fee);
	*fee = 29;
	printf("\nfee = %d", *fee);
	printf("\testvar = %d", test_var);
}
