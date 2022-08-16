#include "sdk_common.h"
#include "nrf_drv_adc.h"
#include "app_error.h"
#include "sys_alive.h"

#include "batMea.h"

#define NRF_LOG_MODULE_NAME "BAT"
#define NRF_LOG_LEVEL           4
#include "nrf_log.h"

typedef enum
{
  BAT_IDLE = 0,
  BAT_IN_PROCESS,
} mea_state_t;

//------------------------------------------------------------------------------
//        PRIVATE VARIABLE
//------------------------------------------------------------------------------
static nrf_drv_adc_channel_t m_channel_config =
{{{
  .resolution = NRF_ADC_CONFIG_RES_10BIT,
  .input      = NRF_ADC_CONFIG_SCALING_SUPPLY_ONE_THIRD,
  .reference  = NRF_ADC_CONFIG_REF_VBG,
  .ain        = 0
 }}, NULL};

static uint16_t mv = INVALID_BATT_VOLTAGE;
static mea_state_t mea_state;
static bat_cb_t cb = NULL;
static void *ctx = NULL;
static bool isMeaCompleted = false;

//------------------------------------------------------------------------------
//        PRIVATE FUNCTIONS
//------------------------------------------------------------------------------
static void adc_event_handler(nrf_drv_adc_evt_t const * p_event)
{
  if (p_event->type == NRF_DRV_ADC_EVT_SAMPLE)
  {
    uint32_t voltage = (3600 * p_event->data.sample.sample) >> 10;
    mv = (uint16_t)voltage;
    NRF_LOG_INFO("Battery = %d mv\n", mv);
    isMeaCompleted = true;
    sleepLock();
  }
}

//-----------------------------------------------------------------------------
void runmea(void)
{
  ret_code_t ret_code;
  nrf_drv_adc_config_t config = NRF_DRV_ADC_DEFAULT_CONFIG;

  ret_code = nrf_drv_adc_init(&config, adc_event_handler);
  APP_ERROR_CHECK(ret_code);

  nrf_drv_adc_channel_enable(&m_channel_config);

  ret_code = nrf_drv_adc_sample_convert(&m_channel_config, NULL);
  APP_ERROR_CHECK(ret_code);
}

//-----------------------------------------------------------------------------
//      PUBLIC FUNCTIONS
//-----------------------------------------------------------------------------
bool batMea_Start(bat_cb_t bat_cb, void *bat_ctx)
{
  bool retval = false;
  if (mea_state == BAT_IDLE)
  {
    mea_state = BAT_IN_PROCESS;
    cb = bat_cb;
    ctx = bat_ctx;
    runmea();
    retval = true;
  }
  return retval;
}

//-----------------------------------------------------------------------------
void batMea_Process(void)
{
  if (isMeaCompleted)
  {
    isMeaCompleted = false;
    if (cb)
    {
      cb(mv, ctx);
    }
    cb = NULL;
    ctx = NULL;
    nrf_drv_adc_uninit();
    mea_state = BAT_IDLE;
  }
}
