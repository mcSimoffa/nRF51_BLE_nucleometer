/*********************************************************************
*                    SEGGER Microcontroller GmbH                     *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*            (c) 2014 - 2020 SEGGER Microcontroller GmbH             *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
* All rights reserved.                                               *
*                                                                    *
* Redistribution and use in source and binary forms, with or         *
* without modification, are permitted provided that the following    *
* condition is met:                                                  *
*                                                                    *
* - Redistributions of source code must retain the above copyright   *
*   notice, this condition and the following disclaimer.             *
*                                                                    *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             *
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,        *
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           *
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
* DISCLAIMED. IN NO EVENT SHALL SEGGER Microcontroller BE LIABLE FOR *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  *
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;    *
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF      *
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT          *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE  *
* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
* DAMAGE.                                                            *
*                                                                    *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
*/
#include "nrf.h"
#include "boards.h"
//#include "nrf_delay.h"
//#include "nrf_gpio.h"
//#include "nrf_drv_gpiote.h"
#include "app_timer.h"
#include "nrf_log_ctrl.h"
#include <stdio.h>
#include <stdlib.h>
// ********************************************************************
#define DEAD_BEEF                       0xDEADBEEF  // Value used as error code on stack dump, can be used to identify stack location on stack unwind.

#define APP_TIMER_PRESCALER             0 // Value of the RTC1 PRESCALER register.
#define APP_TIMER_OP_QUEUE_SIZE         4 // Size of timer operation queues


int main(void)
{
   uint32_t err_code;
    // Initialize.
    err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false); //what it does ?
    err_code = bsp_init(BSP_INIT_LED, APP_TIMER_TICKS(100, APP_TIMER_PRESCALER), NULL);
}

/*************************** End of file ****************************/