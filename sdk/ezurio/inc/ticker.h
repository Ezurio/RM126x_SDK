/*******************************************************************************
  Filename:       ticker.h
  Revised:        15/07/2025
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

  3. Neither the name of Ezurio LLC nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission.
 
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
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/
#ifndef EZ_COMPONENTS_HDR_TICKER_H_
#define EZ_COMPONENTS_HDR_TICKER_H_

#define TICKER_SECONDS_MAX (uint64_t)((1.0f / 32768.0f) * (float)(0xFFFFFFFF))

/*****************************************************************************
 * @fn void ticker_init
 * @brief   Initialize the tick timer.
 ****************************************************************************/
void ticker_init(void);

/*****************************************************************************
 * @fn uint32_t ticker_get_tick_s
 * @brief Get the current second tick count.
 * @return Seconds tick.
 ****************************************************************************/
uint32_t ticker_get_tick_s(void);

/*****************************************************************************
 * @fn uint32_t ticker_get_tick_ms
 * @brief Get the current millisecond tick count.
 * @return Milliseconds tick
 ****************************************************************************/
uint32_t ticker_get_tick_ms(void);

/*****************************************************************************
 * @fn uint32_t ticker_get_overflow_count
 * @brief Get the number of overflows of the ms timer.
 * @return Overflow count
 ****************************************************************************/
uint32_t ticker_get_overflow_count(void);

/*****************************************************************************
 * @fn void ticker_reset_overflow_count
 * @brief Clear the count of overflows of the ms timer.
 ****************************************************************************/
void ticker_reset_overflow_count(void);

/*****************************************************************************
 * @fn uint32_t ticker_get_ticks
 * @brief Get the current tick count.
 * @return Tick count
 ****************************************************************************/
uint32_t ticker_get_ticks(void);

/*****************************************************************************
 * @fn void start_ticker_timed_callback_stop
 * @brief Stops a previously triggered ms delay callback.
 ****************************************************************************/
void start_ticker_timed_callback_stop(void);

/*****************************************************************************
 * @fn void start_ticker_timed_callback
 * @param milliseconds - Delay in ms when to invoke the passed callback.
 * @param callback - Callback function pointer.
 * @param data - Callback function data.
 * @brief This function executes to handler timer events.
 *****************************************************************************/
void start_ticker_timed_callback(const uint32_t milliseconds,
                                 void (*callback)(void *context),
                                 void *data);

#endif /* EZ_COMPONENTS_HDR_TICKER_H_ */
