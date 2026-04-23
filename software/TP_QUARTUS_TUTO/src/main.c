/**
 * @file main.c
 * @brief This file contains the main function and the functions used in the main.c file
 * @author A. Frayard
 */

#include "main.h"


/**
 * @brief DEBUG macros
 */

// #define DEBUG
#define INFO
#define FASTCLOCK
#ifdef FASTCLOCK
#define FASTCLOCK_FREQ 20
#endif





static alt_alarm internal_alarm; 	// internal alarm to update the internal time every second and check if the alarm time is reached
static alt_alarm user_alarm;		// user alarm for the sound generation







alt_u8 test_var = 8;


/**
 * @brief main function, initialize the internal alarm and enter in an infinite loop to check the state of the switches and push buttons
 */
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
			activate_alarm();
		}
		else
		{
			deactivate_alarm();
		}
		
		if (SW_value & 0b0000000010)
		{
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
		

		for (size_t delay = 20000; delay != 0; --delay); // delay loop
	}
    return 0;
}





/**
* @brief main alarm callback function, called every second by the internal alarm, 
* @brief updates the internal time and check if the alarm time is reached
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
 * @brief store the switch register value in "SW_value"
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
 * @brief store the push button (key) register value in "KEY_value"
 */
alt_u8 get_key(void)
{
	KEY_value = IORD_ALTERA_AVALON_PIO_DATA(KEY_ptr);
#ifdef DEBUG
	printf("\n switches -> %d", KEY_value);
#endif
    return 0;
}

/**
 * @brief enter blocking loop to display the alarm time and allow the user to change it using the push buttons
 */
alt_u8 set_alarm_time(void)
{
#ifdef INFO
	printf("\n alarm_time = %d", alarm_time);
#endif
	alarm_set = 1;
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
		delay(200000);
		if(alarm_time >= 86400)
		{
			alarm_time = 0;
		}
	}
}

/**
 * @brief allow the user to set the internal time using the 2 push button the the board
 */
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
		delay(200000);
		if(internal_time >= 86400)
		{
			internal_time = 0;
		}
	}
}

/**
 * @brief turn on the "alarm_state" flag and the alarm LED light
 */
alt_u8 activate_alarm(void)
{
	alarm_state = 1;
	LED_bits = 0b000000001;
	IOWR_ALTERA_AVALON_PIO_DATA(LED_ptr, LED_bits);
	return 0;
}

/**
 * @brief turn off the "alarm_state" flag and the alarm LED light
 */
alt_u8 deactivate_alarm(void)
{
	alarm_state = 0;
	LED_bits = 0b000000000;
	IOWR_ALTERA_AVALON_PIO_DATA(LED_ptr, LED_bits);
	return 0;
}

/**
 * @brief display the internal time on the 6 7seg displays
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
 * @brief convert 32bits int in formated time structure
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
 * @brief convert 8bits hex in 2 8bits BCD
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
 * @brief transforme the 32bits value to time format and diplay it on the 6 7seg displays
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

/**
 * @brief drive the HP output pin to generate a sound
 */
alt_u8 hp_out(void)
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


/**
 * @brief simple blocking delay function
 */
alt_u8 delay(alt_u16 delay_ms)
{
	for (size_t i = delay_ms; i != 0; --i); // delay loop
	return 0;
}

