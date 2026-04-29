/**
 * @file main.c
 * @author Antoine.F
 * @brief this file contains the main function of the project, it initializes the internal alarm and enter in an infinite loop to check the state of the switches and push buttons
 * @version 0.1
 * @date 23/04/2026
 * 
 * @copyright Copyright (c) 2026
 * 
 */



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

/*
 * @brief user includes
 */
#include "main.h"
#include "melodies.h"





/**
 * @brief DEBUG macros
 */

// #define DEBUG
//#define INFO
//#define FASTCLOCK
#ifdef FASTCLOCK
#define FASTCLOCK_FREQ 4
#endif





static alt_alarm internal_alarm; 	// internal alarm to update the internal time every second and check if the alarm time is reached
static alt_alarm user_alarm;		// user alarm for the sound generation







/**
 * @brief main function, initialize the internal alarm and enter in an infinite loop to check the state of the switches and push buttons
 */
int main(void) {
	user_timer_setup();
#ifdef FASTCLOCK
	printf("WARINING FASTCLOCK IS ENABLED\n");
	if (alt_alarm_start (&internal_alarm, alt_ticks_per_second()/FASTCLOCK_FREQ, internal_alarm_callback, NULL) < 0)
#else
	if (alt_alarm_start (&internal_alarm, alt_ticks_per_second(), internal_alarm_callback, NULL) < 0)
#endif
	{
		printf ("No system clock available\n");
	}
	printf("\n NIOSII start");
	for(;;)
	{
		// reset the leds states at each loop
		LED_bits = 0b000000000;
		IOWR_ALTERA_AVALON_PIO_DATA(LED_ptr, LED_bits);
		get_key();
		get_switch();
		

		if(SW_value & 0b0000000001) // sw 0
		{
			activate_alarm();
		}
		else
		{
			deactivate_alarm();
		}
		
		if (SW_value & 0b0000000010) // sw 1
		{
			set_alarm_time();
		}
		else
		{
			alarm_set = 0;
		}

		if (SW_value & 0b0000000100) // sw 2
		{
			internal_time_set = 1;
			set_internal_time();
		}
		else
		{
			internal_time_set = 0;
		}

		if (SW_value & 0b0000001000) // sw 3
		{
			launch_alarm();
		}

		select_melody = (SW_value >> 8) & 0b00000011;
		printf("\n select_melody = %d", select_melody);
		

		for (size_t delay = 20000; delay != 0; --delay); // delay loop
	}
    return 0;
}





/**
 * @brief main alarm callback function, called every second by the internal alarm, 
 * @brief updates the internal time and check if the alarm time is reached
 * 
 * @param context the context for the callback
 * @return alt_u32 
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
			launch_alarm();
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

/**
 * @brief multi-purpose alarm callback function
 * 
 * @param context the context for the callback
 * @return alt_u32 
 */
alt_u32 user_alarm_callback (void* context)
{
	user_alarm_flag = 1;
	if (user_alarm_en)
	{
		set_user_timer((alt_u16)melody_freq/2);
		return context;
	}
	return 0;
}

/**
 * @brief store the switch register value in "SW_value"
 * 
 * @return alt_u8 
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
 * 
 * @return alt_u8 
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
 * 
 * @return alt_u8 
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
	return 0;
}

/**
 * @brief allow the user to set the internal time using the 2 push button the the board
 * 
 * @return alt_u8 
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
	return 0;
}

/**
 * @brief turn on the "alarm_state" flag and the alarm LED light
 * 
 * @return alt_u8 
 */
alt_u8 activate_alarm(void)
{
	if (alarm_time == 0)
	{
		return ERR_ALARM_TIME_NOT_SET;
	}
	if (alarm_state)
	{
		return ERR_ALARM_ALREADY_SET;
	}
	alarm_state = 1;
	LED_bits = 0b000000001;
	IOWR_ALTERA_AVALON_PIO_DATA(LED_ptr, LED_bits);
	return 0;
}

/**
 * @brief turn off the "alarm_state" flag and the alarm LED light
 * 
 * @return alt_u8 
 */
alt_u8 deactivate_alarm(void)
{
	if (!alarm_time)
	{
		return ERR_ALARM_NOT_SET;
	}
	alarm_state = 0;
	LED_bits = 0b000000000;
	IOWR_ALTERA_AVALON_PIO_DATA(LED_ptr, LED_bits);
	return 0;
}

/**
 * @brief output the choosen melodie and blink an led
 */
alt_u8 launch_alarm(void)
{
	LED_bits = 0b100000000;
	IOWR_ALTERA_AVALON_PIO_DATA(LED_ptr, LED_bits);
	

	switch (select_melody)
	{
	case 0:
		hp_out(tetris, sizeof(tetris)/sizeof(tetris[0]));
		break;
	case 1:
		hp_out(mario, sizeof(mario)/sizeof(mario[0]));
		break;
	case 2:
		hp_out(Zelda, sizeof(Zelda)/sizeof(Zelda[0]));
		break;
	case 3:
		hp_out(base_melody, sizeof(base_melody)/sizeof(base_melody[0]));
		break;
	
	default:
		break;
	}
	return 0;
}


/**
 * @brief display the internal time on the 6 7seg displays
 * 
 * @return alt_u8 
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
 * 
 * @param time 32bits time in seconds
 * @param hour pointer to store the hour value
 * @param min pointer to store the minute value
 * @param sec pointer to store the second value
 * @return alt_u8 
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
 * 
 * @param bin input binary number (must be <= 99)
 * @param decimal pointer to store the decimal value (tens)
 * @param unit pointer to store the unit value
 * @return alt_u8 error code
 */
alt_u8 bin_2_bcd(alt_u8 bin, alt_u8 *decimal, alt_u8 *unit)
{
	if (bin > 99)
	{
		return ERR_OVERFLOW;
	}
	*decimal = bin / 10;
	*unit = bin % 10;
#ifdef DEBUG
	printf("\nbin 4 bcd = %d", bin);
	printf("\ndec 4 bcd = %d", *decimal);
	printf("\nunit 4 bcd = %d", *unit);
#endif
	return 0;
}


/**
 * @brief transforme the 32bits value to time format and diplay it on the 6 7seg displays
 * 
 * @param time 
 * @param format 
 * @return alt_u8 
 */
alt_u8 update_display(alt_u32 time, alt_u8 format)
{
	time_2_hhmmss(time, &display.hours, &display.minutes, &display.seconds);
	bin_2_bcd(display.seconds, &display.bcd_sec_1, &display.bcd_sec_0);
	bin_2_bcd(display.minutes, &display.bcd_min_1, &display.bcd_min_0);
	if (format)
	{
		if(display.hours > 11)
		{
			display.hours = display.hours - 12;
		}
	}
	
	bin_2_bcd(display.hours, &display.bcd_hou_1, &display.bcd_hou_0);
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
 * @brief the sound is generated by a square wave with a frequency corresponding to the note to be played
 * @brief frequency is generated by a blocking delay
 * 
 * @return alt_u8 
 */
alt_u8 hp_out(alt_u16 *melody, alt_u16 melody_count)
{
    printf("\nHP_out");
	alt_u32 sec_test = 0;
	printf("\n\n\n MELODY START \n\n\n");
	alt_alarm_stop(&user_alarm);
	user_alarm_en = 1;
	alt_alarm_start(&user_alarm, alt_ticks_per_second()/MELODY_FREQ, user_alarm_callback, alt_ticks_per_second()/MELODY_FREQ);
    for(size_t i = 0; i < melody_count; i++)
    {
		melody_freq = melody[i];
		user_alarm_flag = 0; // reset the user_alarm flag
		while(!user_alarm_flag)
		{
			if (IORD_ALTERA_AVALON_TIMER_STATUS(TIMER_0_BASE) & 0b00000001)
			{
				hp_output_state = !hp_output_state;
				IOWR_ALTERA_AVALON_PIO_DATA(HP_ptr, hp_output_state);
				IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_0_BASE, 0); //clear status register
			}
		}
    }
	user_alarm_en = 0;
    printf("\n\n\n MELODY STOP \n\n\n");
    return 0;
}

alt_u8 user_timer_setup(void)
{
	printf("\n timer status %d", IORD_ALTERA_AVALON_TIMER_STATUS(TIMER_0_BASE));
	IOWR_ALTERA_AVALON_TIMER_PERIODH(TIMER_0_BASE, (alt_u16) 0x02fa);
	IOWR_ALTERA_AVALON_TIMER_PERIODL(TIMER_0_BASE, (alt_u16) 0xf080);
	printf("\n PERIODH %d", IORD_ALTERA_AVALON_TIMER_PERIODH(TIMER_0_BASE));
	printf("\n PERIODL %d", IORD_ALTERA_AVALON_TIMER_PERIODL(TIMER_0_BASE));
	/**
	 * Control bits
	 * 0 - ITO
	 * 1 - CONT
	 * 2 - START
	 * 3 - STOP
	 */
	IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_0_BASE, 0b00000110);
	printf("\n timer control %d", IORD_ALTERA_AVALON_TIMER_CONTROL(TIMER_0_BASE));
	printf("\n timer status %d", IORD_ALTERA_AVALON_TIMER_STATUS(TIMER_0_BASE));
	return 0;
}

alt_u8 set_user_timer(alt_16 frequency)
{
	alt_u32 period = 50000000/frequency;
	//IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_0_BASE, 0b00000000);
	IOWR_ALTERA_AVALON_TIMER_PERIODL(TIMER_0_BASE, (alt_u16) (period & 0x0000ffff));
	IOWR_ALTERA_AVALON_TIMER_PERIODH(TIMER_0_BASE, (alt_u16) (period>>16 & 0x0000ffff));
	IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_0_BASE, 0b00000110);
	return 0;
}



/**
 * @brief simple blocking delay function
 * @param delay_ms the delay in milliseconds
 * @return 0 when the delay is over
 */
alt_u8 delay(alt_u16 delay_ms)
{
	for (size_t i = delay_ms; i != 0; --i); // delay loop
	return 0;
}

