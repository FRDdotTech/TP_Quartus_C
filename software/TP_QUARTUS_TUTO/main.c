/**
 * 
 * @file main.c
 * @author Antoine.F
 * @brief this file contains the main function of the project, it initializes the internal alarm and enter in an infinite loop to check the state of the switches and push buttons
 * @version 0.1
 * @date 23/04/2026
 * @copyright Copyright (c) 2026
 * 
 * @mainpage main.c
 * 
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

/** 
 * @section user includes
 */
#include "main.h"
#include "melodies.h"





/**
 * @section DEBUG macros
 */

// #define DEBUG
// #define INFO
#define FASTCLOCK
#ifdef FASTCLOCK
#define FASTCLOCK_FREQ 5
#endif





/**
 * @brief main function, initialize the internal alarm and enter in an infinite loop to check the state of the switches and push buttons
 */
int main(void) {
	alt_u8 rc = ERR_OK;
	rc = user_timer_setup();

#ifdef FASTCLOCK
	printf("\n WARNING FASTCLOCK IS ENABLED\n");
	if (alt_alarm_start (&internal_alarm, alt_ticks_per_second()/FASTCLOCK_FREQ, internal_alarm_callback, NULL) < 0)
#else
	if (alt_alarm_start (&internal_alarm, alt_ticks_per_second(), internal_alarm_callback, NULL) < 0)
#endif
	{
		printf ("\n No system clock available\n");
	}
	printf("\n NIOSII start");
	LED_bits = 0b000000000;
	IOWR_ALTERA_AVALON_PIO_DATA(LED_ptr, LED_bits);
	for(;;)
	{
		// reset the leds states at each loop
		rc = get_key();
		rc = get_switch();

		if(SW_value & 0b0000000001) // sw 0
		{
			rc = activate_alarm();
		}
		else
		{
			rc = deactivate_alarm();
		}
		
		if (SW_value & 0b0000000010) // sw 1
		{
			alarm_set = 1;
			rc = set_time(&alarm_time);
		}
		else
		{
			alarm_set = 0;
		}

		if (SW_value & 0b0000000100) // sw 2
		{
			internal_time_set = 1;
			rc = set_time(&internal_time);
		}
		else
		{
			internal_time_set = 0;
		}

		if (SW_value & 0b0000001000) // sw 3
		{
			//launch_alarm();
		}

		time_format = (SW_value >> 4) & 0b00000001;
		select_melody = (SW_value >> 8) & 0b00000011;
		rc = delay(200);
		if (alarm_set)
		{
			update_display(alarm_time, time_format);
		}
		else
		{
			update_display(internal_time, time_format);
		}
		

		/**
		 * @brief error management
		 * 
		 */

		switch (rc)
		{
		case ERR_LAUNCH_ALARM:
			launch_alarm();
			break;
		 
		case ERR_DELAY_WATCHDOG:
			printf(("\n ERROR OCCURRED : ERROR_DELAY_WATCHDOG"));
			break;

		case ERR_WRONG_MELODY:
			printf(("\n ERROR OCCURRED : ERR_WRONG_MELODY"));
			break;

		case ERR_ALARM_ALREADY_SET:
			printf(("\n ERROR OCCURRED : ERR_ALARM_ALREADY_SET"));
			break;

		case ERR_INVALID_KEY:
			printf(("\n ERROR OCCURRED : ERR_INVALID_KEY"));
			break;

		case ERR_INVALID_SW:
			printf(("\n ERROR OCCURRED : ERR_INVALID_SW"));
			break;
			
		case ERR_INVALID_TIME:
			printf(("\n ERROR OCCURRED : ERR_INVALID_TIME"));
			break;

		case ERR_ALARM_TIME_NOT_SET:
			printf(("\n ERROR OCCURRED : ERR_ALARM_TIME_NOT_SET"));
			break;

		case ERR_ALARM_NOT_SET:
			printf(("\n ERROR OCCURRED : ERR_ALARM_NOT_SET"));
			break;

		case ERR_OVERFLOW:
			printf(("\n ERROR OCCURRED : ERR_OVERFLOW"));
			break;

		 default:
			break;
		 }
	}
    return ERR_OK;
}





/**
 * @brief main alarm callback function, called every second by the internal alarm, 
 * @brief updates the internal time and check if the alarm time is reached
 * 
 * @param context NULL pointer
 * @return alt_u32 the alarm period (in ticks) for the next alarm interruption
 */
alt_u32 internal_alarm_callback (void* context)
{
	if(!internal_time_set)
	{
		internal_time++;
	}
	if(alarm_state)
	{
		if(internal_time == alarm_time)
		{
			launch_alarm_flag = 1;
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
 * @brief hp alarm callback function, called by the hp_alarm to generate the sound, 
 * @brief it set the "hp_alarm_flag" to 1 to indicate that the alarm is ready to generate the sound
 * 
 * @param context the period for the next alarm interruption
 * @return alt_u32 the alarm period (in ticks) for the next alarm interruption
 */
alt_u32 hp_alarm_callback (void* context)
{
	
	if (hp_alarm_en)
	{
		set_user_timer((alt_u16)melody_freq/2);
		hp_alarm_flag = 1;
		return context;
	}
	return ERR_OK;
}

/**
 * @brief alarm callback function for the blocking delay, it set the "delay_alarm_flag" to 1 to indicate that the delay is over
 * 
 * @param context NULL pointer
 * @return alt_u32 return 0;
 */
alt_u32 delay_alarm_callback (void* context)
{
	
	delay_alarm_flag = 1;
	return ERR_OK;
}

/**
 * @brief store the switch register value in "SW_value"
 * 
 * @return alt_u8 return code (ERR_OK)
 */
alt_u8 get_switch(void)
{
	SW_value = IORD_ALTERA_AVALON_PIO_DATA(SW_switch_ptr); //*(SW_switch_ptr); // read the SW slider switch values
#ifdef DEBUG
	printf("\n switches -> %x", SW_value);
#endif
    return ERR_OK;
}

/**
 * @brief store the push button (key) register value in "KEY_value"
 * 
 * @return alt_u8 return code (ERR_OK)
 */
alt_u8 get_key(void)
{
	KEY_value = IORD_ALTERA_AVALON_PIO_DATA(KEY_ptr);
#ifdef DEBUG
	printf("\n switches -> %d", KEY_value);
#endif
    return ERR_OK;
}


/**
 * @brief Set the time of the internal time or the alarm time depending on the time pointer
 * 
 * @param time pointer to the time variable to be set (internal_time or alarm_time)
 * @return alt_u8 return code (ERR_OK / ERR_DELAY_WATCHDOG / ERR_LAUNCH_ALARM)
 */
alt_u8 set_time(alt_u32 *time)
{
#ifdef INFO
	printf("\n internal_time = %d", internal_time);
#endif
	alt_u8 rc = ERR_OK;
	update_display(*time, time_format);
	while(SW_value & 0b0000000110)
	{
		get_switch();
		get_key();
		switch (KEY_value)
		{
		case NO_KEY:
			break;

		case KEY_0:
			*time = *time + 60; // add 1 hour
			rc = delay(300); //debouncing
			break;

		case KEY_1:
			*time = *time + 3600; // add 1 minute
			rc = delay(300); //debouncing
			break;

		case KEY_0_1:
			*time = *time - 60; // remove 1 minute
			rc = delay(300); //debouncing
			break;
		default:
			break;
		}
		if(*time >= 86400)
		{
			*time = 0;
		}
		if(rc != ERR_OK)
			{
				return rc;
			}
	}
	return rc;
}

/**
 * @brief turn on the "alarm_state" flag and the alarm LED light
 * 
 * @return alt_u8 return code (ERR_ALARM_TIME_NOT_SET / ERR_ALARM_ALREADY_SET / ERR_OK)
 */
alt_u8 activate_alarm(void)
{
	if (alarm_time == 0)
	{
		return ERR_ALARM_TIME_NOT_SET;
	}
	alarm_state = 1;
	LED_bits = 0b0000000111;
	IOWR_ALTERA_AVALON_PIO_DATA(LED_ptr, LED_bits);
	return ERR_OK;
}

/**
 * @brief turn off the "alarm_state" flag and the alarm LED light
 * 
 * @return alt_u8 return code (ERR_ALARM_NOT_SET / ERR_OK)
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
	//printf("\n alarm_deactivated");
	return ERR_OK;
}

/**
 * @brief 
 * @brief launch the alarm by calling the "hp_out" function with the selected melody, 
 * @brief the melody is selected using the SW slider switch 8 and 9
 * 
 * @return alt_u8 return code (ERR_OK / ERR_WRONG_MELODY)
 */
alt_u8 launch_alarm(void)
{
	alt_u8 rc = ERR_OK;
	LED_bits = 0b1110000000;
	IOWR_ALTERA_AVALON_PIO_DATA(LED_ptr, LED_bits);
	

	switch (select_melody)
	{
	case 0:
		rc = hp_out(tetris, sizeof(tetris)/sizeof(tetris[0]));
		break;
	case 1:
		rc = hp_out(mario, sizeof(mario)/sizeof(mario[0]));
		break;
	case 2:
		rc = hp_out(Zelda, sizeof(Zelda)/sizeof(Zelda[0]));
		break;
	case 3:
		rc = hp_out(base_melody, sizeof(base_melody)/sizeof(base_melody[0]));
		break;
	default:
		hp_out(base_melody, sizeof(base_melody)/sizeof(base_melody[0]));
		rc = ERR_WRONG_MELODY;
		break;
	}

	LED_bits = 0b000000000;
	IOWR_ALTERA_AVALON_PIO_DATA(LED_ptr, LED_bits);
	return rc;
}


/**
 * @brief convert 32bits int in formatted time structure
 * 
 * @param time 32bits time in seconds
 * @param hour pointer to store the hour value
 * @param min pointer to store the minute value
 * @param sec pointer to store the second value
 * @return alt_u8 return code (ERR_OK)
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
	return ERR_OK;
}


/**
 * @brief convert 8bits hex in 2 8bits BCD 
 * 
 * @param bin input binary number (must be <= 99)
 * @param decimal pointer to store the decimal value (tens)
 * @param unit pointer to store the unit value
 * @return alt_u8 error code (ERR_OK / ERR_OVERFLOW)
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
	return ERR_OK;
}


/**
 * @brief transform the 32bits value to time format and displays it on the 6 7seg displays
 * 
 * @param time 32bits time in seconds
 * @param format time format (0 for 24h, 1 for 12h)
 * @return alt_u8 return code (ERR_OK)
 */
alt_u8 update_display(alt_u32 time, alt_u8 format)
{
	time_2_hhmmss(time, &display.hours, &display.minutes, &display.seconds);
	if(format)
	{
		/* MERIDIAN */
		HEX_bits = SEG_M;
		HEX_bits = HEX_bits << 8;
		if(display.hours > 11)
		{
			display.hours = display.hours - 12;
			HEX_bits = HEX_bits + SEG_P;
		}
		else
		{
			HEX_bits = HEX_bits + SEG_A;
		}
		IOWR_ALTERA_AVALON_PIO_DATA(HEX5_HEX4_ptr, HEX_bits);
	}
	else
	{
		bin_2_bcd(display.seconds, &display.bcd_sec_1, &display.bcd_sec_0);
	}

	bin_2_bcd(display.minutes, &display.bcd_min_1, &display.bcd_min_0);
	bin_2_bcd(display.hours, &display.bcd_hou_1, &display.bcd_hou_0);

	if(format)
	{

		/* HOURS */
		HEX_bits = conversion_array[abs(display.bcd_hou_1)];
		HEX_bits = HEX_bits << 8;
		HEX_bits = HEX_bits + conversion_array[abs(display.bcd_hou_0)];
		HEX_bits = HEX_bits << 8;

		/* MINUTES */
		HEX_bits = HEX_bits + conversion_array[abs(display.bcd_min_1)];
		HEX_bits = HEX_bits << 8;
		HEX_bits = HEX_bits + conversion_array[abs(display.bcd_min_0)];
		IOWR_ALTERA_AVALON_PIO_DATA(HEX3_HEX0_ptr, HEX_bits);
	}
	else
	{
		/* HOURS */
		HEX_bits = conversion_array[abs(display.bcd_hou_0)];
		HEX_bits = HEX_bits << 8;
		HEX_bits = HEX_bits + conversion_array[abs(display.bcd_hou_1)];
		IOWR_ALTERA_AVALON_PIO_DATA(HEX5_HEX4_ptr, HEX_bits);

		/* MINUTES */
		HEX_bits = conversion_array[abs(display.bcd_min_1)];
		HEX_bits = HEX_bits << 8;
		HEX_bits = HEX_bits + conversion_array[abs(display.bcd_min_0)];
		HEX_bits = HEX_bits << 8;

		/* SECONDS */
		HEX_bits = HEX_bits + conversion_array[abs(display.bcd_sec_1)];
		HEX_bits = HEX_bits << 8;
		HEX_bits = HEX_bits + conversion_array[abs(display.bcd_sec_0)];
		IOWR_ALTERA_AVALON_PIO_DATA(HEX3_HEX0_ptr, HEX_bits);
	}
	return ERR_OK;
}

/**
 * @brief drive the HP output pin to generate a sound
 * @brief the sound is generated by a square wave with a frequency corresponding to the note to be played
 * @brief frequency is generated by a blocking delay
 * 
 * @return alt_u8 return code (ERR_OK)
 */
alt_u8 hp_out(alt_u16 *melody, alt_u16 melody_count)
{
	launch_alarm_flag = 0;
	alt_alarm_stop(&hp_alarm);
	hp_alarm_en = 1;
	printf("\n\n\n MELODY START \n\n\n");
	alt_alarm_start(&hp_alarm, alt_ticks_per_second()/MELODY_FREQ, hp_alarm_callback, alt_ticks_per_second()/MELODY_FREQ); 
    for(size_t i = 0; i < melody_count; i++)
    {
		get_key();
		if(KEY_value)
		{
			break;
		}
		melody_freq = melody[i];
		hp_alarm_flag = 0; // reset the hp_alarm flag
		while(!hp_alarm_flag)
		{
			if (IORD_ALTERA_AVALON_TIMER_STATUS(TIMER_0_BASE) & 0b00000001)
			{
				hp_output_state = !hp_output_state;
				IOWR_ALTERA_AVALON_PIO_DATA(HP_ptr, hp_output_state);
				IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_0_BASE, 0); //clear status register
			}
		}
    }
	alt_alarm_stop(&hp_alarm);
	hp_alarm_en = 0;
    printf("\n\n\n MELODY STOP \n\n\n");
    return ERR_OK;
}

/**
 * @brief initialize the user timer with default values
 * 
 * @return alt_u8 return code (ERR_OK)
 */
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
	return ERR_OK;
}

/**
 * @brief Set the user timer period to generate the desired frequency
 * 
 * @param frequency the frequency of the square wave to be generated
 * @return alt_u8 return code (ERR_OK)
 */
alt_u8 set_user_timer(alt_16 frequency)
{
	alt_u32 period = 50000000/frequency;
	//IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_0_BASE, 0b00000000);
	IOWR_ALTERA_AVALON_TIMER_PERIODL(TIMER_0_BASE, (alt_u16) (period & 0x0000ffff));
	IOWR_ALTERA_AVALON_TIMER_PERIODH(TIMER_0_BASE, (alt_u16) (period>>16 & 0x0000ffff));
	IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_0_BASE, 0b00000110);
	return ERR_OK;
}


/**
 * @brief simple blocking delay function
 * @param delay_ms the delay in milliseconds
 * @return alt_u8 return code (ERR_OK / ERR_DELAY_WATCHDOG / ERR_LAUNCH_ALARM)
 */
alt_u8 delay(alt_u16 delay_ms)
{
	alt_u32 current_internal = internal_time;
	alt_alarm_stop(&delay_alarm);
	delay_alarm_flag = 0;
	alt_alarm_start(&delay_alarm, alt_ticks_per_second()/(1000/delay_ms), delay_alarm_callback, 0);
	while (!delay_alarm_flag)
	{
		if ((current_internal < (internal_time-2)) & (internal_time > 2))
		{

			alt_alarm_stop(&delay_alarm);
			return ERR_DELAY_WATCHDOG;
		}
		if(launch_alarm_flag)
		{
			//launch_alarm();
			return ERR_LAUNCH_ALARM;
		}

		if (alarm_set)
		{
			update_display(alarm_time, time_format);
		}
		else
		{
			update_display(internal_time, time_format);
		}
	}
	return ERR_OK;
}

