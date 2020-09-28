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
#include <stdio.h>
#include <stdlib.h>
#include "app_timer.h"
#include "boards.h"
#include "bsp.h"
#include "custom_board.h"
#include "nrf.h"
#include "nrf_log_ctrl.h"
#include "softdevice_handler.h"
#include "ble.h"
#include "softdevice_handler.h"

//#include "nrf_delay.h"
//#include "nrf_gpio.h"
//#include "nrf_drv_gpiote.h"

/* *******************************************************************
                    Function prototypes
******************************************************************** */
static void ble_stack_init(void);

// ********************************************************************
#define DEAD_BEEF 0xDEADBEEF      // Value used as error code on stack dump, can be used to identify stack location on stack unwind.
#define APP_TIMER_PRESCALER     0   // Value of the RTC1 PRESCALER register.
#define APP_TIMER_OP_QUEUE_SIZE 4   // Size of timer operation queues
#define CENTRAL_LINK_COUNT      0   // Number of central links used by the application. When changing this number remember to adjust the RAM settings*/
#define PERIPHERAL_LINK_COUNT   0   // Number of peripheral links used by the application. When changing this number remember to adjust the RAM settings*/
//in global defines needs BLE_STACK_SUPPORT_REQD https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk5.v12.3.0%2Flib_softdevice_handler.html

int main(void) 
{
  uint32_t err_code;
  // Initialize.
  err_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(err_code);

  APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false); //why this timer needs ?
  err_code = bsp_init(BSP_INIT_LED, APP_TIMER_TICKS(100, APP_TIMER_PRESCALER), NULL); //custom_board.h
  APP_ERROR_CHECK(err_code);
  ble_stack_init();
}

/* ******************************************************************
Initializes the SoftDevice and the BLE event interrupt.
https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk51.v10.0.0%2Fgroup__peer__manager.html
SoftDevice use a resource:
https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.s130.sds%2Fdita%2Fsoftdevices%2Fs130%2Fsd_resource_reqs%2Fhw_block_interrupt_vector.html&anchor=hw_block_interrupt_vector
interrupt
https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.s130.sds%2Fdita%2Fsoftdevices%2Fs130%2Fprocessor_avail_interrupt_latency%2Finterrupt_forwarding_to_application.html&anchor=interrupt_forwarding_to_application

******************************************************************** */
static void ble_stack_init(void) 
{
  uint32_t err_code;
  nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC; //this struct define clock source for Softdevice

  // Initialize the SoftDevice handler module.
  
  SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);

  ble_enable_params_t ble_enable_params; // ble.h
  err_code = softdevice_enable_get_default_config(CENTRAL_LINK_COUNT,
      PERIPHERAL_LINK_COUNT,
      &ble_enable_params);  //default param (it possible manual tune also)
  APP_ERROR_CHECK(err_code);

  //Check the ram settings against the used number of links
  CHECK_RAM_START_ADDR(CENTRAL_LINK_COUNT, PERIPHERAL_LINK_COUNT);

  // Enable BLE stack.
  err_code = softdevice_enable(&ble_enable_params);
  APP_ERROR_CHECK(err_code);
}