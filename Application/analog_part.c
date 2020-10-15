#include "analog_part.h"

#define ADC_BUFFER_SIZE 2     // Size of buffer for ADC samples for battery voltage.
#define DUTY_ON         5     // 5 uS time ti pumpON
#define PUMP_HV_PIN     LED_2

static nrf_adc_value_t       adc_buffer_Vbat[ADC_BUFFER_SIZE]; // ADC battery voltage buffer
static nrf_drv_adc_channel_t Vbat_channel_config = NRF_DRV_ADC_DEFAULT_CHANNEL(NRF_ADC_CONFIG_INPUT_3); //Channel instance. Default configuration used.
//static nrf_drv_adc_channel_t m_channel_config = NRF_DRV_ADC_DEFAULT_CHANNEL(NRF_ADC_CONFIG_INPUT_4); //Channel instance. Default configuration used.

static struct 
{
  uint8_t   enable;
  uint8_t   pump_quant;
  uint16_t  feedbackVoltage;
} HV_param;

const nrf_drv_timer_t TIMER_PUMP_ON = NRF_DRV_TIMER_INSTANCE(1);
nrf_ppi_channel_t ppi_channel1;

/* *************************************************************
 This function will be called each ANALOGPART_TIMER_INTERVAL
 It give start for all analog process:
    HV convertor
    Battery level measurement
************************************************************** */
void analogPart_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    //if (HV_param.enable)
    {
      nrf_drv_timer_clear(&TIMER_PUMP_ON);
      nrf_drv_gpiote_out_task_force(PUMP_HV_PIN, 1);
      nrf_drv_timer_enable(&TIMER_PUMP_ON); //  start watch ON part of duty cycle


    }

    //battery_charge_measure_start();
}


void timer_pumpON_event_handler(nrf_timer_event_t event_type, void* p_context)
{
    static uint32_t i;
    uint32_t led_to_invert = ((i++) % LEDS_NUMBER);
return;
    switch (event_type)
    {
        case NRF_TIMER_EVENT_COMPARE0:
            bsp_board_led_invert(led_to_invert);
            break;

        default:
            //Do nothing.
            break;
    }
}


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
  //configure PUMP_HV_PIN for controlling by Event
  ret_code_t ret_code;
  nrf_ppi_channel_t ppi_channel_HV;
  nrf_drv_gpiote_out_config_t pumpOut = GPIOTE_CONFIG_OUT_TASK_LOW;

  ret_code = nrf_drv_gpiote_out_init(PUMP_HV_PIN, &pumpOut);
  APP_ERROR_CHECK(ret_code);

  ret_code = nrf_drv_ppi_init();
  APP_ERROR_CHECK(ret_code);

  // timer for duty cycle. Part ON
  nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
  ret_code = nrf_drv_timer_init(&TIMER_PUMP_ON, &timer_cfg, timer_pumpON_event_handler);
  APP_ERROR_CHECK(ret_code);
  // setting single triggered
  uint32_t time_ticks = nrf_drv_timer_us_to_ticks(&TIMER_PUMP_ON, DUTY_ON);
  nrf_drv_timer_extended_compare(
        &TIMER_PUMP_ON, NRF_TIMER_CC_CHANNEL0, time_ticks, NRF_TIMER_SHORT_COMPARE0_STOP_MASK, true);
  
  ret_code = nrf_drv_ppi_channel_alloc(&ppi_channel_HV);
  APP_ERROR_CHECK(ret_code);


  ret_code = nrf_drv_ppi_channel_assign (ppi_channel_HV,
                                          nrf_drv_timer_event_address_get (&TIMER_PUMP_ON, NRF_TIMER_EVENT_COMPARE0),
                                          nrf_drv_gpiote_out_task_addr_get (PUMP_HV_PIN));
  APP_ERROR_CHECK(ret_code);

  ret_code = nrf_drv_ppi_channel_enable(ppi_channel_HV);
  APP_ERROR_CHECK(ret_code);

  nrf_drv_gpiote_out_task_enable(PUMP_HV_PIN);
  
// it's temporary
while(1)
{
 for (uint32_t i=0;i<1000000; i++)
   asm("nop");
  analogPart_timeout_handler(NULL);
}

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
