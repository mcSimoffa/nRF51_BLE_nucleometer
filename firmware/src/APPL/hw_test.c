#include "sdk_common.h"
#include "app_error.h"
#include "nrf_log_ctrl.h"
#include "nrf_gpio_adds.h"
#include "hw_test.h"

#define NRF_LOG_MODULE_NAME   "HW_TEST"
#define NRF_LOG_LEVEL         4
#define NRF_LOG_DEBUG_COLOR   5
#include "nrf_log.h"

void hw_test_Run(void)
{
  nrf_gpio_cfg_output(PUMP_HV_PIN);
  nrf_gpio_cfg_output(PUMP_HV_PIN_DRV);
  while (1)
  {
    nrf_gpio_pin_toggle(PUMP_HV_PIN);
    nrf_gpio_pin_toggle(PUMP_HV_PIN_DRV);
  }
}



