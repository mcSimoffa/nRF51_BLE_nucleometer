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

File    : main.c
Purpose : Generic application start

*/
#include "nrf.h"
#include "boards.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"

#include "nrf_drv_ppi.h"
#include <stdio.h>
#include <stdlib.h>
/*********************************************************************
*
*       main()
*
*  Function description
*   Application entry point.
*/
int main(void) {
nrf_gpio_cfg_input(BUTTON_1,NRF_GPIO_PIN_PULLUP);
ret_code_t err_code;
err_code = nrf_drv_ppi_init();
err_code = nrf_drv_gpiote_init();

nrf_ppi_channel_t ppi_channel;
err_code = nrf_drv_ppi_channel_alloc(&ppi_channel); //aalocated chanells store in:  m_channels_allocated

  
  nrf_gpiote_task_configure(0,GREEN_LED,NRF_GPIOTE_POLARITY_TOGGLE,1);
  nrf_gpiote_event_configure(1,BUTTON_1,NRF_GPIOTE_POLARITY_LOTOHI);

uint32_t eventaddr = nrf_gpiote_event_addr_get(NRF_GPIOTE_EVENTS_IN_1);
uint32_t taskaddr = nrf_gpiote_task_addr_get(NRF_GPIOTE_TASKS_OUT_0);
err_code = nrf_drv_ppi_channel_assign(ppi_channel, eventaddr, taskaddr); //it assign event addr in CH[x].EEP and task addr in CH[x].TEP
err_code = nrf_drv_ppi_channel_enable(ppi_channel); //it set bit in CHSET

  nrf_gpiote_task_enable(0);
  nrf_gpiote_event_enable(1);
  int i;
/*
  nrf_gpio_cfg_output(ORANGE_LED);
  nrf_gpio_cfg_output(GREEN_LED);
  for (uint8_t i = 0; i < 100; i++) {
    printf("Hello World %d!\n", i);
    nrf_gpio_pin_set(ORANGE_LED);
    nrf_delay_ms(100);
    nrf_gpio_pin_clear(ORANGE_LED);
    nrf_gpio_pin_toggle(GREEN_LED);
    nrf_delay_ms(100);
  }*/

  do {
  asm("nop");
  //nrf_delay_ms(100);
  } while (1);
}

/*************************** End of file ****************************/