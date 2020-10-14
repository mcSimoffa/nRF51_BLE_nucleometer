#include "analog_part.h"
#define ADC_BUFFER_SIZE 2     // Size of buffer for ADC samples for battery voltage.

static nrf_adc_value_t       adc_buffer_Vbat[ADC_BUFFER_SIZE]; // ADC battery voltage buffer
static nrf_drv_adc_channel_t Vbat_channel_config = NRF_DRV_ADC_DEFAULT_CHANNEL(NRF_ADC_CONFIG_INPUT_3); //Channel instance. Default configuration used.
//static nrf_drv_adc_channel_t m_channel_config = NRF_DRV_ADC_DEFAULT_CHANNEL(NRF_ADC_CONFIG_INPUT_4); //Channel instance. Default configuration used.

static void adc_event_handler(nrf_drv_adc_evt_t const * p_event)
{
  if (p_event->type == NRF_DRV_ADC_EVT_DONE)
  {
    uint32_t i;
    nrf_gpio_cfg_default(BATTERY_MEAS_PIN);
    for (i = 0; i < p_event->data.done.size; i++)
    {
      NRF_LOG_INFO("Current sample value: %d\r\n", p_event->data.done.p_buffer[i]);
    }
  }
}

void analog_part_init()
{
  ret_code_t ret_code;
  nrf_drv_adc_config_t config = NRF_DRV_ADC_DEFAULT_CONFIG;
  // ADC interrupt priority and event handler
  ret_code = nrf_drv_adc_init(&config, adc_event_handler);
  APP_ERROR_CHECK(ret_code);
 
  ret_code = nrf_drv_adc_sample_convert(&Vbat_channel_config, NULL);
  APP_ERROR_CHECK(ret_code);
}

/* *******************************************************
  Start a single ADC convertion 
******************************************************** */
void battery_charge_measure_start()
{
  nrf_gpio_cfg_output(BATTERY_MEAS_PIN);
  nrf_drv_adc_sample_convert(&Vbat_channel_config, NULL);
}
