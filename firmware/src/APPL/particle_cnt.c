#include <sdk_common.h>
#include "nrf_log_ctrl.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpio_adds.h"
#include "sys_alive.h"
#include "ble_main.h"

#define NRF_LOG_MODULE_NAME "PCNT"
#define NRF_LOG_LEVEL       4
#include "nrf_log.h"

// ----------------------------------------------------------------------------
//  DEFINE MODULE PARAMETER
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//   PRIVATE TYPES
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
//   PRIVATE VARIABLE
// ----------------------------------------------------------------------------
static uint32_t pulse_cnt;
static bool isNewData;
// ----------------------------------------------------------------------------
//    PRIVATE FUNCTION
// ----------------------------------------------------------------------------
static void OnPulsePinEvt(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  if ((pin == PULSE_PIN) && (action == NRF_GPIOTE_POLARITY_LOTOHI))
  {
    nrf_gpio_pull_set(PULSE_PIN, NRF_GPIO_PIN_PULLUP);  // go tiristor to OFF state
    pulse_cnt++;
    isNewData = true;
    __asm("nop");
    __asm("nop");
    __asm("nop");
    __asm("nop");
    __asm("nop");
    __asm("nop");
    sleepLock();
    nrf_gpio_pull_set(PULSE_PIN, NRF_GPIO_PIN_PULLDOWN);

  }
}

// ---------------------------------------------------------------------------
// PULSE_PIN init
static void cnt_gpio_init(void)
{
  nrf_drv_gpiote_in_config_t pulse_conf = GPIOTE_CONFIG_IN_SENSE_LOTOHI(false);
  pulse_conf.pull = NRF_GPIO_PIN_PULLDOWN;
  ret_code_t err_code;

  err_code = nrf_drv_gpiote_in_init(PULSE_PIN, &pulse_conf, OnPulsePinEvt);
  ASSERT(err_code == NRF_SUCCESS);
}


// ----------------------------------------------------------------------------
//    PUBLIC FUNCTION
// ----------------------------------------------------------------------------
void particle_cnt_Init(void)
{
  cnt_gpio_init();
}

// ---------------------------------------------------------------------------
void particle_cnt_Startup()
{
  nrf_drv_gpiote_in_event_enable(PULSE_PIN, true);
}

// ---------------------------------------------------------------------------
void particle_cnt_Process()
{
  if (isNewData)
  {
    isNewData = false;
    NRF_LOG_DEBUG("total pulses %d\n", pulse_cnt);
    ble_ios_pulse_transfer(pulse_cnt);
  }
}

// ---------------------------------------------------------------------------
uint32_t particle_cnt_Get()
{
  return pulse_cnt;
}
