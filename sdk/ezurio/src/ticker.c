/*******************************************************************************
  Filename:       ticker.c
  Revised:        15/07/25
  Revision:       2.0

  Description:    Provides system tick from which ms and s are derived.

  Copyright (c) 2024-25 Ezurio.
 
  All rights reserved.
 
  Redistribution and use in source and binary forms, with or without
  modification, are allowed if following conditions are met:
 
  1. Redistributions of source code must retain the above copyright notice, this
     list of conditions and the disclaimer below.
 
  2. Redistributions in binary form, except as embedded into an Ezurio LLC
     module in a product, an Ezurio LLC product, or a software update for such
     products, must reproduce the above copyright notice, this list of
     conditions and the disclaimer in the documentation and/or other materials
     provided with the distribution.

  3. Neither the name of Ezurio LLC nor the names of its contributors may be
     used to endorse or promote products derived from this software without
     prior written permission.

  4. This software, with or without modification, may only be used with an
     Ezurio LLC module or Ezurio LLC product.

  5. Any software provided in binary form under this license may not be reverse
     engineered, decompiled, modified or disassembled.
 
  THIS SOFTWARE IS PROVIDED BY EZURIO LLC "AS IS" AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
  OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. TO THE MAXIMUM EXTENT ALLOWED BY LAW, IN NO EVENT SHALL EZURIO LLC
  OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
  OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include "sl_sleeptimer.h"
#include "ticker.h"
#include "ez_common.h"
#include "em_cmu.h"
#include <inttypes.h>
#include "em_rtcc.h"

/*******************************************************************************
 ***************************  LOCAL MACROS *************************************
 ******************************************************************************/
#define TICKER_RTCC_CHANNEL_NUMBER   1
#define TICKER_RTCC_CHANNEL_INT_ID   RTCC_IEN_CC1
#define TICKER_RTCC_CHANNEL_INT_FLAG RTCC_IF_CC1

/*******************************************************************************
 ***************************  LOCAL TYPES  *************************************
 ******************************************************************************/
/* Callback data */
typedef struct {
  void (*callback)(void *context);
  void *data;
}timed_callback_function_t;

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/
static timed_callback_function_t timed_callback_function;

static uint32_t overflow_count = 0;

/*******************************************************************************
 ***************************  GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/
void ticker_init(void)
{
  /* Set RTCC clock source */
  CMU_ClockSelectSet(cmuClock_RTCC, cmuSelect_LFXO);

  /* Set the clock for the RTCC running */
  CMU_ClockEnable(cmuClock_RTCC, true);

  /* Set up for the RTCC for millisecond ticks */
  RTCC_Init_TypeDef init = RTCC_INIT_DEFAULT;
  init.presc = rtccCntPresc_1;

  /* Then kick off the RTCC */
  RTCC_Init((const RTCC_Init_TypeDef *)&init);

  /* Enable RTCC Interrupt for appropriate channel */
  RTCC_IntEnable(TICKER_RTCC_CHANNEL_INT_ID);

  /* Enable overflow detection interrupt */
  RTCC_IntEnable(RTCC_IEN_OF);

  /* Clear any spurious flags */
  RTCC_IntClear(RTCC_IF_OF);
  RTCC_IntClear(TICKER_RTCC_CHANNEL_INT_FLAG);

  /* Clear global IRQ bit and then enable */
  NVIC_ClearPendingIRQ(RTCC_IRQn);
  NVIC_EnableIRQ(RTCC_IRQn);
}

uint32_t ticker_get_tick_s(void)
{
  uint32_t ticks_ms;

  ticks_ms = ticker_get_tick_ms();
  return (MS_TO_SEC(ticks_ms));
}

uint32_t ticker_get_ticks(void)
{
  uint32_t ticks;

  /* Get the tick count */
  ticks = RTCC_CounterGet();

  return (ticks);
}

uint32_t ticker_get_tick_ms(void)
{
  uint32_t ticks_ms = ticker_get_ticks();

  ticks_ms = sl_sleeptimer_tick_to_ms(ticks_ms);

  return(ticks_ms);
}

uint32_t ticker_get_overflow_count(void)
{
  return(overflow_count);
}

void ticker_reset_overflow_count(void)
{
  overflow_count = 0;
}

void start_ticker_timed_callback_stop(void)
{
  /* Initialisation data for the RTCC compare channel for ms delays */
  RTCC_CCChConf_TypeDef rtcc_init_compare_channel = RTCC_CH_INIT_COMPARE_DEFAULT;

  /* Disable the channel */
  rtcc_init_compare_channel.chMode = rtccCapComChModeOff;
  RTCC_ChannelInit(TICKER_RTCC_CHANNEL_NUMBER, &rtcc_init_compare_channel);
}

void start_ticker_timed_callback(const uint32_t milliseconds,
                                  void ( *callback )( void* context ),
                                  void* data)
{
  /* Initialisation data for the RTCC compare channel for ms delays */
  RTCC_CCChConf_TypeDef rtcc_init_compare_channel = RTCC_CH_INIT_COMPARE_DEFAULT;
  /* Number of ticks needed to create requested ms delay */
  uint32_t ticks;
  /* Current tick count used as basis for ms delay calculation */
  uint32_t current_ticks;

  /* Store the details of the callback and associated data for later */
  timed_callback_function.callback = callback;
  timed_callback_function.data = data;

  /* Get the equivalent tick count for the ms requested */
  sl_sleeptimer_ms32_to_tick(milliseconds, &ticks);

  /* Current ticks used as basis for offset */
  current_ticks = ticker_get_ticks();

  /* Initialise the channel used to create ms delays */
  RTCC_ChannelInit(TICKER_RTCC_CHANNEL_NUMBER, &rtcc_init_compare_channel);

  /* Wrap around implicit */
  RTCC_ChannelCCVSet(TICKER_RTCC_CHANNEL_NUMBER, ticks + current_ticks);
}

/*******************************************************************************
 ***************************  LOCAL FUNCTIONS  *********************************
 ******************************************************************************/
/*******************************************************************************
 * @fn  RTCC_IRQHandler
 * @brief Override for RTCC IRQ handler. Triggered when a compare match is made
 *        via a ms delay request.
 ******************************************************************************/
void RTCC_IRQHandler(void)
{
  uint32_t flags = RTCC_IntGet();

  if (flags & RTCC_IF_OF)
  {
    RTCC_IntClear(RTCC_IF_OF);
    overflow_count++;
  }

  if (flags & TICKER_RTCC_CHANNEL_INT_FLAG)
  {
    /* Clear interrupt source */
    RTCC_IntClear(TICKER_RTCC_CHANNEL_INT_FLAG);

    /* Call the callback with data. */
    (timed_callback_function.callback)(timed_callback_function.data);
  }
}
