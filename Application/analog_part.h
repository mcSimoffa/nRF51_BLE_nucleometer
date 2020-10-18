#ifndef ANALOG_PART_H__
#define ANALOG_PART_H__

#include <stdint.h>
#include <stdbool.h>
#include "boards.h"
#include "nrf_drv_adc.h"
#include "app_error.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf.h"
#include "nrf_drv_timer.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_gpiote.h"

#define ANALOGPART_TIMER_INTERVAL     APP_TIMER_TICKS(1000, APP_TIMER_PRESCALER)

// Analog module machine state
#define HVU_STATE_IDLE            0
#define HVU_STATE_PROCESS         1


void analogPart_timeout_handler(void * p_context);
static void adc_event_handler(nrf_drv_adc_evt_t const * p_event);
void analog_part_init();
void battery_charge_measure_start();

#endif	//ANALOG_PART_H__