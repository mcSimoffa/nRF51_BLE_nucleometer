#include "sdk_common.h"
#include "nrf.h"
#include "nrf_drv_adc.h"
#include "nordic_common.h"
#include "app_error.h"
#include "app_util_platform.h"

#define NRF_LOG_MODULE_NAME "BAT"
#define NRF_LOG_LEVEL           4
#include "nrf_log.h"

static nrf_drv_adc_channel_t m_channel_config =
{{{
    .resolution = NRF_ADC_CONFIG_RES_10BIT,
    .input      = NRF_ADC_CONFIG_SCALING_SUPPLY_ONE_THIRD,
    .reference  = NRF_ADC_CONFIG_REF_VBG,
    .ain        = 0
 }}, NULL};

//NRF_DRV_ADC_DEFAULT_CHANNEL(NRF_ADC_CONFIG_INPUT_2);

/**
 * @brief ADC interrupt handler.
 */
static void adc_event_handler(nrf_drv_adc_evt_t const * p_event)
{
    if (p_event->type == NRF_DRV_ADC_EVT_SAMPLE)
    {
      uint32_t mv = (3600 * p_event->data.sample.sample) >> 10;
      uint8_t unit = (mv - 1500) >> 3;  //1 unit = 8mV after 1500mV
      NRF_LOG_INFO("Battery = %d mv, %d unit", mv, unit);
    }
}


/**
 * @brief Function for main application entry.
 */
void batMea_Start(void)
{
  ret_code_t ret_code;
  nrf_drv_adc_config_t config = NRF_DRV_ADC_DEFAULT_CONFIG;

  ret_code = nrf_drv_adc_init(&config, adc_event_handler);
  APP_ERROR_CHECK(ret_code);

  nrf_drv_adc_channel_enable(&m_channel_config);

  ret_code = nrf_drv_adc_sample_convert(&m_channel_config, NULL);
  APP_ERROR_CHECK(ret_code);
}
/** @} */
