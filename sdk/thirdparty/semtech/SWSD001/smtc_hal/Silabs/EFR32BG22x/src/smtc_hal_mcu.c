/*!
 * \file      smtc_hal_mcu.c
 *
 * \brief     MCU Hardware Abstraction Layer implementation
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

#include <stdint.h>   // C99 types
#include <stdbool.h>  // bool type

#include "smtc_hal_mcu.h"
//#include "modem_pinout.h"

#if defined( __GNUC__ )
    #include "cmsis_gcc.h"
#elif defined( __ICCARM__ )
    #include "cmsis_iccarm.h"
#endif

//#include "smtc_board.h"
#include "em_device.h"
#include "efr32bg22xx_hal.h"
#include "smtc_hal.h"
#include "sl_udelay.h"
#include "ticker.h"
#include "smtc_modem_hal.h"
#if( HAL_DBG_TRACE == HAL_FEATURE_ON )
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#endif
/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

/*!
 * Watchdog counter reload value during sleep
 *
 * \remark The period must be lower than MCU watchdog period
 */
#define WATCHDOG_RELOAD_PERIOD_SECONDS 20
/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*!
 * @brief Low Power options
 */
typedef enum low_power_mode_e
{
    LOW_POWER_ENABLE,
    LOW_POWER_DISABLE
} low_power_mode_t;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

static volatile bool             exit_wait            = false;
static volatile low_power_mode_t lp_current_mode      = LOW_POWER_ENABLE;
static bool                      partial_sleep_enable = false;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */
#if 0
/*!
 * @brief init the MCU clock tree
 */
static void hal_mcu_system_clock_config( void );

/*!
 * @brief init the GPIO
 */
static void hal_mcu_gpio_init( void );

/*!
 * @brief init the power voltage detector
 */
static void hal_mcu_pvd_config( void );

#if( HAL_LOW_POWER_MODE == HAL_FEATURE_ON )
/*!
 * @brief reinit the MCU clock tree after a stop mode
 */
static void hal_mcu_system_clock_re_config_after_stop( void );

/*!
 * @brief Deinit the MCU
 */
static void hal_mcu_lpm_mcu_deinit( void );

/*!
 * @brief Initializes MCU after a stop mode
 */
static void hal_mcu_lpm_mcu_reinit( void );
#endif

/*!
 * @brief MCU enter in low power sleep mode
 */
static void hal_mcu_lpm_enter_sleep_mode( void );

/*!
 * @brief MCU exit low power sleep mode
 */
static void hal_mcu_lpm_exit_sleep_mode( void );

#if( HAL_DBG_TRACE == HAL_FEATURE_ON )
/*!
 * @brief printf
 */
static void vprint( const char* fmt, va_list argp );
#endif
#endif
/*!
 * @brief Function runing the low power mode handler
 */
static void hal_mcu_sleep_handler( void );

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

void hal_mcu_critical_section_begin( uint32_t* mask )
{
    *mask = __get_PRIMASK( );
    __disable_irq( );
}

void hal_mcu_critical_section_end( uint32_t* mask ) { __set_PRIMASK( *mask ); }

void hal_mcu_disable_irq( void ) { __disable_irq( ); }

void hal_mcu_enable_irq( void ) { __enable_irq( ); }

void hal_mcu_delay_ms( uint32_t delay_ms ) { sl_udelay_wait( delay_ms * 1000 ); }

uint32_t hal_mcu_get_tick( void ) { return ticker_get_ticks( ); }

void hal_mcu_init( void )
{

}

void hal_mcu_reset( void )
{
    __disable_irq( );
    __NVIC_SystemReset(); // Restart system
}

void __attribute__( ( optimize( "O0" ) ) ) hal_mcu_wait_us( const int32_t microseconds )
{
    // Work @80MHz
    const uint32_t nb_nop = microseconds * 1000 / 137;
    for( uint32_t i = 0; i < nb_nop; i++ )
    {
        __asm("NOP");
    }
}

void hal_mcu_set_sleep_for_ms( const int32_t milliseconds )
{
    bool last_sleep_loop = false;

    if( milliseconds <= 0 )
    {
        return;
    }
    CRITICAL_SECTION_BEGIN( );

    int32_t time_counter = milliseconds;

#if( HAL_USE_WATCHDOG == HAL_FEATURE_ON )
    hal_watchdog_reload( );
#endif  // HAL_USE_WATCHDOG == HAL_FEATURE_ON

    do
    {
        if( ( time_counter > ( WATCHDOG_RELOAD_PERIOD_SECONDS * 1000 ) ) )
        {
            time_counter -= WATCHDOG_RELOAD_PERIOD_SECONDS * 1000;
            hal_rtc_wakeup_timer_set_ms( WATCHDOG_RELOAD_PERIOD_SECONDS * 1000 );
        }
        else
        {
             hal_rtc_wakeup_timer_set_ms( time_counter );
            // if the sleep time is less than the wdog reload period, this is the last sleep loop
            last_sleep_loop = true;
        }
        hal_mcu_sleep_handler( );

#if( HAL_USE_WATCHDOG == HAL_FEATURE_ON )
        hal_watchdog_reload( );
#endif  // HAL_USE_WATCHDOG == HAL_FEATURE_ON
    } while( ( hal_rtc_has_wut_irq_happened( ) == true ) && ( last_sleep_loop == false ) );
    if( last_sleep_loop == false )
    {
        // in case sleep mode is interrupted by an other irq than the wake up timer, stop it and exit
        hal_rtc_wakeup_timer_stop( );
    }
    CRITICAL_SECTION_END( );
}

uint16_t hal_mcu_get_vref_level( void ) { return 0; }

int16_t hal_mcu_get_temperature( void ) { return 0; }

void hal_mcu_disable_low_power_wait( void )
{
    exit_wait       = true;
    lp_current_mode = LOW_POWER_DISABLE;
}

void hal_mcu_enable_low_power_wait( void )
{
    exit_wait       = false;
    lp_current_mode = LOW_POWER_ENABLE;
}
#define HAL_DBG_TRACE HAL_FEATURE_ON
void hal_mcu_trace_print( const char* fmt, ... )
{
#if HAL_DBG_TRACE == HAL_FEATURE_ON
    va_list argp;
    va_start( argp, fmt );
    vprintf( fmt, argp );
    va_end( argp );
#endif
}

#ifdef USE_FULL_ASSERT
/*
 * Function Name  : assert_failed
 * Description    : Reports the name of the source file and the source line
 * number where the assert_param error has occurred. Input          : - file:
 * pointer to the source file name
 *                  - line: assert_param error line source number
 * Output         : None
 * Return         : None
 */
void assert_failed( uint8_t* file, uint32_t line )
{
    // User can add his own implementation to report the file name and line
    // number,
    // ex: printf("Wrong parameters value: file %s on line %lu\r\n", file, line)

    SMTC_HAL_TRACE_PRINTF( "Wrong parameters value: file %s on line %lu\r\n", ( const char* ) file, line );
    // Infinite loop
    while( 1 )
    {
    }
}
#endif



void HAL_MspInit( void )
{

}

void hal_mcu_partial_sleep_enable( bool enable ) { partial_sleep_enable = enable; }

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

#if( HAL_LOW_POWER_MODE == HAL_FEATURE_ON )

/**
 * @brief Exits Sleep Mode
 *
 */

#endif
/**
 * @brief Low power handler
 *
 */
static void hal_mcu_sleep_handler( void )
{

}
/* --- EOF ------------------------------------------------------------------ */
