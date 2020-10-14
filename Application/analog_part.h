#ifndef ANALOG_PART_H__
#define ANALOG_PART_H__

#include <stdint.h>
#include <stdbool.h>
#include "boards.h"
#include "nrf_drv_adc.h"
#include "app_error.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

static void adc_event_handler(nrf_drv_adc_evt_t const * p_event);
void analog_part_init();
void battery_charge_measure_start();

#endif	//ANALOG_PART_H__