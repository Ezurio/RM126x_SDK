/*!
 * @file      smtc_hal_rtc.c
 *
 * @brief     RTC Hardware Abstraction Layer implementation
 *
 * The Clear BSD License
 * Copyright Semtech Corporation 2021. All rights reserved.
 * Copyright Ezurio 2023. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Semtech corporation nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SEMTECH CORPORATION BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <time.h>
#include <math.h>
#include "smtc_hal.h"
#include "ticker.h"
#include "smtc_modem_hal.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

/*!
 * Calculates ceiling( X / N )
 */
#define DIVC( X, N ) ( ( ( X ) + ( N ) -1 ) / ( N ) )

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

/* MCU Wake Up Time */
#define MIN_ALARM_DELAY_IN_TICKS 3U  // in ticks

/* sub-second number of bits */
#define N_PREDIV_S 10U

/* Synchronous prediv */
#define PREDIV_S ( ( 1U << N_PREDIV_S ) - 1U )

/* Asynchronous prediv */
#define PREDIV_A ( ( 1U << ( 15U - N_PREDIV_S ) ) - 1U )

/* Sub-second mask definition */
#define ALARM_SUBSECOND_MASK ( N_PREDIV_S << RTC_ALRMASSR_MASKSS_Pos )

/* RTC Time base in us */
#define USEC_NUMBER 1000000U
#define MSEC_NUMBER ( USEC_NUMBER / 1000 )

#define COMMON_FACTOR 3U
#define CONV_NUMER ( MSEC_NUMBER >> COMMON_FACTOR )
#define CONV_DENOM ( 1U << ( N_PREDIV_S - COMMON_FACTOR ) )

/*!
 * @brief Days, Hours, Minutes and seconds
 */
#define DAYS_IN_LEAP_YEAR ( ( uint32_t ) 366U )
#define DAYS_IN_YEAR ( ( uint32_t ) 365U )
#define SECONDS_IN_1DAY ( ( uint32_t ) 86400U )
#define SECONDS_IN_1HOUR ( ( uint32_t ) 3600U )
#define SECONDS_IN_1MINUTE ( ( uint32_t ) 60U )
#define MINUTES_IN_1HOUR ( ( uint32_t ) 60U )
#define HOURS_IN_1DAY ( ( uint32_t ) 24U )

/*!
 * @brief Correction factors
 */
#define DAYS_IN_MONTH_CORRECTION_NORM ( ( uint32_t ) 0x99AAA0 )
#define DAYS_IN_MONTH_CORRECTION_LEAP ( ( uint32_t ) 0x445550 )

// clang-format on

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*!
 * @brief RTC timer context
 */
typedef struct
{
    uint32_t        time_ref_in_ticks;  // Reference time
    // calendar_time;      // Reference time in calendar format
    // calendar_date;      // Reference date in calendar format
} rtc_context_t;

/*!
 * @brief RTC structure
 */
typedef struct hal_rtc_s
{
//    handle;
    /*!
     * Keep the value of the RTC timer when the RTC alarm is set
     * Set with the \ref hal_rtc_set_context function
     * Value is kept as a Reference to calculate alarm
     */
    rtc_context_t context;
} hal_rtc_t;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

hal_rtc_t hal_rtc;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */
/*!
 * @brief Get the elapsed time in seconds and milliseconds since RTC initialization
 *
 * @param [out] milliseconds_div_10 Number of 0.1 milliseconds elapsed since RTC
 *                                  initialization
 * @returns seconds Number of seconds elapsed since RTC initialization
 */
static uint32_t hal_rtc_get_calendar_time( uint16_t* milliseconds_div_10 );



/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

void hal_rtc_init( void )
{

}

uint32_t hal_rtc_get_time_s( void )
{
    uint64_t time_s64;
    uint64_t overflow_count;

    time_s64 = (uint64_t)ticker_get_tick_s();
    overflow_count = (uint64_t)ticker_get_overflow_count();
    if (overflow_count)
    {
        time_s64 += (overflow_count * TICKER_SECONDS_MAX);
    }
    if (time_s64 > UINT32_MAX)
    {
        time_s64 -= UINT32_MAX;
        ticker_reset_overflow_count();
    }
    return((uint32_t)time_s64);
}

uint32_t hal_rtc_get_time_100us( void )
{
    uint32_t seconds             = 0;
    uint16_t milliseconds_div_10 = 0;

    seconds = hal_rtc_get_calendar_time( &milliseconds_div_10 );

    return seconds * 10000 + milliseconds_div_10;
}

uint32_t hal_rtc_get_time_ms( void )
{
    return ticker_get_tick_ms();
}

void hal_rtc_stop_alarm( void )
{

}

/*!
 * @brief Sets the alarm
 *
 * @remark The alarm is set at now (read in this function) + timeout
 *
 * @param [in] timeout Duration of the Timer ticks
 */
void hal_rtc_start_alarm( uint32_t timeout )
{
    UNUSED( timeout );
}

uint32_t hal_rtc_get_timer_value( void )
{

    return ( 0 );
}

uint32_t hal_rtc_get_timer_elapsed_value( void )
{

    return (0);
}

void hal_rtc_delay_in_ms( const uint32_t milliseconds )
{
    UNUSED( milliseconds );
}

void hal_rtc_wakeup_timer_set_s( const int32_t seconds )
{
    UNUSED( seconds );
}

void hal_rtc_wakeup_timer_set_ms( const int32_t milliseconds )
{
    UNUSED( milliseconds );
}

void hal_rtc_wakeup_timer_stop(void)
{

}


bool hal_rtc_has_wut_irq_happened( void ) { return 0; }

uint32_t hal_rtc_set_time_ref_in_ticks( void )
{

    return 0;
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

uint32_t hal_rtc_get_time_ref_in_ticks( void ) { return hal_rtc.context.time_ref_in_ticks; }

uint32_t hal_rtc_ms_2_tick( const uint32_t milliseconds )
{
    (void)milliseconds;

    return ( 0 );
}

uint32_t hal_rtc_tick_2_100_us( const uint32_t tick )
{
    UNUSED( tick );
    return (0);
}

uint32_t hal_rtc_tick_2_ms( const uint32_t tick )
{
    uint32_t seconds    = tick >> N_PREDIV_S;
    uint32_t local_tick = tick & PREDIV_S;

    return ( uint32_t )( ( seconds * 1000 ) + ( ( local_tick * 1000 ) >> N_PREDIV_S ) );
}

static uint32_t hal_rtc_get_calendar_time( uint16_t* milliseconds_div_10 )
{
    uint32_t total_ticks = ticker_get_tick_ms();
    uint32_t remaining_ticks = 0;
    uint32_t seconds = 0;

    seconds = (total_ticks / 1000);
    remaining_ticks = (total_ticks - (seconds * 1000));

    if (milliseconds_div_10 != NULL)
    {
      /* The milliseconds_div_10 is in units of .1ms */
      *milliseconds_div_10 = remaining_ticks * 10;
    }

    /* The return value is in units of seconds */
    return (seconds);
}

/*!
 * @brief RTC IRQ Handler of the RTC Alarm
 */
void RTC_Alarm_IRQHandler( void )
{

}

void RTC_WKUP_IRQHandler( void )
{

}

uint32_t hal_rtc_get_minimum_timeout( void ) { return ( MIN_ALARM_DELAY_IN_TICKS ); }

uint32_t hal_rtc_temp_compensation( uint32_t period, float temperature )
{
    float k       = RTC_TEMP_COEFFICIENT;
    float k_dev   = RTC_TEMP_DEV_COEFFICIENT;
    float t       = RTC_TEMP_TURNOVER;
    float t_dev   = RTC_TEMP_DEV_TURNOVER;
    float interim = 0.0;
    float ppm     = 0.0;

    if( k < ( float ) 0.0 )
    {
        ppm = ( k - k_dev );
    }
    else
    {
        ppm = ( k + k_dev );
    }
    interim = ( temperature - ( t - t_dev ) );
    ppm *= interim * interim;

    /* Calculate the drift in time */
    interim = ( ( float ) period * ppm ) / ( ( float ) 1e6 );

    /* Calculate the resulting time period */
    interim += period;
    interim = floor( interim );

    if( interim < ( float ) 0.0 )
    {
        interim = ( float ) period;
    }

    /* Calculate the resulting period */
    return ( uint32_t ) interim;
}
