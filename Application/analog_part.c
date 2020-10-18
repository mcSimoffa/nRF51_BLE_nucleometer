#include "analog_part.h"

#define MAX_CONTINOUS_PUMPS       10
#define DUTY_ON_TIME              100000   //10  // uS time ti pumpON
#define FEDBACK_MEAS_DELAY        200000   //10  //uS. Since this time ADC start measure feedback voltage
#define AFTER_ADC_DELAY           1000000   //10  //uS. Pause after ADC feedback before next pump cycle

#define PUMP_HV_PIN               LED_2

#define AIN_BATTERY               ADC_CONFIG_PSEL_AnalogInput4  // it's P0.3

#define PIN_FEEDBACK_HV           2
#define AIN_FEEDBACK_HV           ADC_CONFIG_PSEL_AnalogInput3  // it's P0.2

#define HV_FEEDBACK_NOMINAL       0x0300  //ADC value what is correspondent to 380V secondary power syplay

// *************************************************************************************************************
uint8_t AnalogModule_state;
static nrf_drv_adc_channel_t Vbat_ch_config =   NRF_DRV_ADC_DEFAULT_CHANNEL(AIN_BATTERY);
static nrf_drv_adc_channel_t fb_HV_ch_config =  NRF_DRV_ADC_DEFAULT_CHANNEL(AIN_FEEDBACK_HV); 

static struct 
{
  uint8_t   enable;
  uint8_t   pump_quant;
  uint16_t  feedbackVoltage;
} HV_param;

const nrf_drv_timer_t TIMER_HV = NRF_DRV_TIMER_INSTANCE(1);
uint32_t tics_duty_on, tics_meas_fb, tics_after_adc;


/* *************************************************************
 Preparing timer and GPIOTE pin to one pulse performing:
 seting PUMP_HV_PIN to 1, and run timer
 falling PUMP_HV_PIN perform by PPI by timer compare event
************************************************************** */
__STATIC_INLINE void make_rising_pump()
{
  AnalogModule_state = HVU_STATE_PROCESS;
  nrf_gpio_cfg_default (PIN_FEEDBACK_HV);        //release pin for ADC may use it
  nrf_adc_enable();
  nrf_adc_int_enable(NRF_ADC_INT_END_MASK);
  nrf_drv_gpiote_out_task_force(PUMP_HV_PIN, 1);  // rising PUMP_HV_PIN
  nrf_drv_timer_enable(&TIMER_HV);                // start watch for state ON
}

/* *************************************************************
 This handler is called each ANALOGPART_TIMER_INTERVAL expiration
 It give start for all analog process:
    HV convertor
    Battery level measurement
************************************************************** */
void analogPart_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    if (HV_param.enable)
    {
      if (AnalogModule_state == HVU_STATE_IDLE)
      {
        HV_param.pump_quant = 0;
        make_rising_pump();
      }
    }
    //battery_charge_measure_start();
}

/* *************************************************************
 This handler is called on TIMER_HV timer interrupt occur
 It give start for all analog process:
    HV convertor
    Battery level measurement
************************************************************** */
static void timer_pumpON_event_handler(nrf_timer_event_t event_type, void* p_context)
{
  if (event_type != NRF_TIMER_EVENT_COMPARE3)
    return;
  switch (AnalogModule_state)
  {
    //Pump time has expirations
     //case HVCONVERTER_STATE_PUMP:
     nrf_timer_cc_write(TIMER_HV.p_reg, NRF_TIMER_CC_CHANNEL0, tics_meas_fb);
     //AnalogModule_state = HVCONVERTER_STATE_DELAY_FEEDBACK;
     nrf_drv_timer_enable(&TIMER_HV);           // start delay before feedback voltage measure
     break;

    //delay before feedback voltage measure has expiration
   // case HVCONVERTER_STATE_DELAY_FEEDBACK:
      nrf_gpio_pin_toggle(LED_1);         //it's temporary for debug
      
      //AnalogModule_state = HVCONVERTER_STATE_ADC_IN_PROCESS;
      ret_code_t ret_code = nrf_drv_adc_sample_convert (&fb_HV_ch_config, NULL);
      APP_ERROR_CHECK(ret_code);
      break;

    //case HVCONVERTER_STATE_DELAY_AFTER_ADC:
      make_rising_pump();
      break;

    //default:
      //AnalogModule_state = HVCONVERTER_STATE_IDLE;
  }
}

/* *************************************************************
 This handler for ADC event handling:
 It have 2 way:
    feedback voltage sample
    battery voltage sample
************************************************************** */
static void adc_event_handler(nrf_drv_adc_evt_t const * p_event)
{
  if (p_event->type != NRF_DRV_ADC_EVT_SAMPLE)
    return;

  switch (AnalogModule_state)
  {
    case HVU_STATE_PROCESS:
      HV_param.feedbackVoltage = p_event->data.sample.sample;
      nrf_gpio_cfg_output(PIN_FEEDBACK_HV);
      nrf_gpio_pin_clear(PIN_FEEDBACK_HV);  //it's discharge of HV feedbacks capasitor

      if ((HV_param.feedbackVoltage < HV_FEEDBACK_NOMINAL) && (++HV_param.pump_quant < MAX_CONTINOUS_PUMPS))
        nrf_drv_timer_enable(&TIMER_HV);           // start after ADC delay
      break;
  }
}

/* *************************************************************
 Analog part initialize:
 TIMER1 for timing fast flyback DC-DC convertor process
 GPIOTE as DC-DC driver
 ADC for measure feedback High Voltage and battery charge level
 PPI for pass Timers event from GPIOTE task
************************************************************** */
void analog_part_init()
{
  uint32_t eventAddr, taskAddr; 
  AnalogModule_state = HVU_STATE_IDLE;
  //configure PUMP_HV_PIN for controlling by Event
  ret_code_t          ret_code;
  nrf_ppi_channel_t   ppi_ch_tim1_gpiote, ppi_ch_tim1_adc;

  nrf_drv_gpiote_out_config_t pumpOut =  {
      .init_state = NRF_GPIOTE_INITIAL_VALUE_LOW, 
      .task_pin   = true,
      .action     = NRF_GPIOTE_POLARITY_HITOLO,
      };
  ret_code = nrf_drv_gpiote_out_init(PUMP_HV_PIN, &pumpOut);
  APP_ERROR_CHECK(ret_code);

  // timer for duty cycle. Part ON
  nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
  timer_cfg.frequency = NRF_TIMER_FREQ_31250Hz;                                 //temporary

  ret_code = nrf_drv_timer_init(&TIMER_HV, &timer_cfg, timer_pumpON_event_handler);
  APP_ERROR_CHECK(ret_code);

  // setting single triggered
  tics_duty_on = nrf_drv_timer_us_to_ticks(&TIMER_HV, DUTY_ON_TIME);
  tics_meas_fb =    tics_duty_on + nrf_drv_timer_us_to_ticks(&TIMER_HV, FEDBACK_MEAS_DELAY);
  tics_after_adc =  tics_meas_fb + nrf_drv_timer_us_to_ticks(&TIMER_HV, AFTER_ADC_DELAY);

  nrf_drv_timer_compare(&TIMER_HV, NRF_TIMER_CC_CHANNEL0, tics_duty_on, false); // pump pulse
  nrf_drv_timer_extended_compare(&TIMER_HV, NRF_TIMER_CC_CHANNEL1, tics_meas_fb, NRF_TIMER_SHORT_COMPARE1_STOP_MASK, false); // pause before feedback meas
  nrf_drv_timer_extended_compare(&TIMER_HV, NRF_TIMER_CC_CHANNEL2, tics_after_adc, NRF_TIMER_SHORT_COMPARE2_STOP_MASK, true); // pause after feedback meas
  
  //PPI init and allocate 2 channel
  ret_code = nrf_drv_ppi_init();
  APP_ERROR_CHECK(ret_code);
  ret_code = nrf_drv_ppi_channel_alloc(&ppi_ch_tim1_gpiote);
  APP_ERROR_CHECK(ret_code);
  ret_code = nrf_drv_ppi_channel_alloc(&ppi_ch_tim1_adc);
  APP_ERROR_CHECK(ret_code);
  
  // Connecting TIMER_HV CC[0] event to GPIOTE task (turn OFF PUMP_HV_PIN).
  // It's need for make a pulse with DUTY_ON uS
  eventAddr =  nrf_drv_timer_event_address_get (&TIMER_HV, NRF_TIMER_EVENT_COMPARE0);
  taskAddr =   nrf_drv_gpiote_out_task_addr_get (PUMP_HV_PIN);
  ret_code = nrf_drv_ppi_channel_assign (ppi_ch_tim1_gpiote, eventAddr, taskAddr);
  APP_ERROR_CHECK(ret_code);
  
  // Connecting TIMER_HV CC[1] event to ADC task (START)
  // It's need for measure feedback voltage
  eventAddr =  nrf_drv_timer_event_address_get (&TIMER_HV, NRF_TIMER_EVENT_COMPARE1);
  taskAddr =   nrf_adc_task_address_get(NRF_ADC_TASK_START);
  ret_code = nrf_drv_ppi_channel_assign (ppi_ch_tim1_adc, eventAddr, taskAddr);
  APP_ERROR_CHECK(ret_code);

  ret_code = nrf_drv_ppi_channel_enable(ppi_ch_tim1_gpiote);
  APP_ERROR_CHECK(ret_code);
  nrf_drv_gpiote_out_task_enable(PUMP_HV_PIN);

  ret_code = nrf_drv_ppi_channel_enable(ppi_ch_tim1_adc);
  APP_ERROR_CHECK(ret_code);

  
  // ADC confifure
  nrf_drv_adc_config_t config = NRF_DRV_ADC_DEFAULT_CONFIG;
  ret_code = nrf_drv_adc_init(&config, adc_event_handler);
  APP_ERROR_CHECK(ret_code);
  
  nrf_adc_config_set(fb_HV_ch_config.config.data);
  nrf_adc_enable();
  nrf_adc_int_enable(NRF_ADC_INT_END_MASK);
  

  HV_param.enable = 1;  //enable work HV sypply source
  // it's temporary
  while(1)
  {
   for (uint32_t i=0;i<2000000; i++)
     asm("nop");
    analogPart_timeout_handler(NULL);
  }  
}

/* *******************************************************
  Start a single ADC convertion 
******************************************************** */
void battery_charge_measure_start()
{
  //nrf_gpio_cfg_output(BATTERY_MEAS_PIN);
  //nrf_drv_adc_sample_convert(&Vbat_ch_config, NULL);
}
