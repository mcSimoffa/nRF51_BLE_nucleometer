#include "analog_part.h"

#define NRF_LOG_MODULE_NAME "ANALOG"
//#define NRF_LOG_LEVEL NRF_LOG_LEVEL_ERROR
#define NRF_LOG_LEVEL NRF_LOG_LEVEL_INFO

#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#define MAX_CONTINOUS_PUMPS       300     // maximum times pump cycle continous
#define DUTY_ON_TIME              100    // uS. Time ti pumpON
#define FEDBACK_MEAS_DELAY        1000   // uS. Since this time ADC start measure feedback voltage

#define PUMP_HV_PIN               3
#define PULSE_PIN                 30                      //for count pulse from Geyger-Miller counter
#define FEEDBACK                  NRF_LPCOMP_INPUT_3      //it's P0.2 pin
#define LPCOM_REF                 NRF_LPCOMP_REF_EXT_REF0 //it's P0.0 pin

// Analog module machine state
#define HVU_STATE_DENY            0
#define HVU_STATE_IDLE            1
#define HVU_STATE_PROCESS         2
#define HVU_STATE_VBAT_MEA        3

// *************************************************************************************************************
void nrf_drv_lpcomp_event_handler_register(lpcomp_events_handler_t lpcomp_events_handler); //this definition in components\drivers_nrf\lpcomp\nrf_drv_lpcomp.c

static nrf_drv_adc_channel_t Vbat_ch_config =   
{{{
    .resolution = NRF_ADC_CONFIG_RES_10BIT,
    .input      = NRF_ADC_CONFIG_SCALING_SUPPLY_ONE_THIRD,
    .reference  = NRF_ADC_CONFIG_REF_VBG,
    .ain        = ADC_CONFIG_PSEL_Disabled
 }}, NULL};

static struct 
{
  uint16_t  BatteryVoltage;
  uint16_t   pump_quant;
  uint8_t   enable;
} HV_param;

uint8_t AnalogModule_state = HVU_STATE_DENY;
const nrf_drv_timer_t TIMER_HV = NRF_DRV_TIMER_INSTANCE(1);
uint32_t tics_duty_on, tics_meas_fb;

/* *************************************************************
 This handler LPCOMP interrupt
 when READY event occur the new pump cycle will launch
************************************************************** */
void lpcomp_event_handler(nrf_lpcomp_event_t event)
{
  if (event == NRF_LPCOMP_EVENT_READY)
  {
    if(nrf_lpcomp_event_check(NRF_LPCOMP_EVENT_UP))
    {
      nrf_lpcomp_disable();
      NRF_LOG_ERROR("Feedback capasitor not ready\r\n");
      AnalogModule_state = HVU_STATE_IDLE;
    }
    else
    {
      nrf_drv_timer_clear(&TIMER_HV);
      __disable_irq();
      nrf_drv_gpiote_out_task_force(PUMP_HV_PIN, 1);  // rising PUMP_HV_PIN
      nrf_timer_task_trigger(TIMER_HV.p_reg, NRF_TIMER_TASK_START);
      //nrf_drv_timer_enable(&TIMER_HV);                // start watch for state ON
      __enable_irq();
    }
  }
}

/* *************************************************************
 This handler TIMER_HV timer interrupt
 if feedback is low
    Next pump cycle 
  else Battery level measurement
************************************************************** */
static void timer1_event_handler(nrf_timer_event_t event_type, void* p_context)
{
  if (event_type == NRF_TIMER_EVENT_COMPARE1)
  {
    nrf_drv_lpcomp_disable();
    //if((!nrf_lpcomp_event_check(NRF_LPCOMP_EVENT_UP)) && (++HV_param.pump_quant < MAX_CONTINOUS_PUMPS))
    if (++HV_param.pump_quant < MAX_CONTINOUS_PUMPS)
    {
      nrf_drv_lpcomp_enable();      // enable and start comparator
    }
    else
    {
      NRF_LOG_INFO("Pump cycles %d\r\n", HV_param.pump_quant);
      AnalogModule_state = HVU_STATE_VBAT_MEA;
      // Prepare and Start ADC for Battery level measure     
      nrf_adc_enable();
      nrf_adc_int_enable(NRF_ADC_INT_END_MASK);
      nrf_adc_start();
    }
  }
}


/* *************************************************************
 This handler for ADC event handling for battery voltage sample
************************************************************** */
static void adc_event_handler(nrf_drv_adc_evt_t const * p_event)
{
  if ((p_event->type == NRF_DRV_ADC_EVT_SAMPLE) && (AnalogModule_state == HVU_STATE_VBAT_MEA))
  {
    HV_param.BatteryVoltage = p_event->data.sample.sample;
    AnalogModule_state = HVU_STATE_IDLE;
    NRF_LOG_INFO("ADC VBAT %X\r\n", HV_param.BatteryVoltage);
  }  
}


/* *************************************************************
 This handler is called each ANALOGPART_TIMER_INTERVAL (RTC) expiration
 It give start analog process according algoritm: LPCOMP Ready
 make rising on PUMP_HV_PIN -> TIMER1 1 start -> CC[0] -> falling PUMP_HV_PIN
 -> CC[1] -> enable LPCOMP OR start ADC (Vbat) -> adc_event_handler -> HVU_STATE_IDLE
************************************************************** */
void analogPart_timeout_handler(void * p_context)
{
  UNUSED_PARAMETER(p_context);
  if (HV_param.enable)
  {
    if (AnalogModule_state == HVU_STATE_IDLE)
    {
      HV_param.pump_quant = 0;
      AnalogModule_state = HVU_STATE_PROCESS;
      nrf_drv_lpcomp_enable();  // enable and start comparator
    }
  }
  else
    NRF_LOG_INFO("Analog part is late\r\n");
}

/* *************************************************************
 Analog part initialize:
 TIMER1 for timing fast flyback DC-DC convertor process
 GPIOTE for DC-DC driver control
 LPCOMP for High Voltage Feedback control
 ADC for measure battery charge level
 PPI for pass Timers event from GPIOTE task
************************************************************** */
void analog_part_init()
{
  uint32_t eventAddr, taskAddr;  
  ret_code_t          ret_code;
  nrf_ppi_channel_t   ppi_ch_tim1_gpiote;

  memset(&HV_param,0, sizeof(HV_param));

  //configure GPIOTE PUMP_HV_PIN for controlling by TIMER1 Event
  nrf_drv_gpiote_out_config_t pumpOut = 
  {
    .init_state = NRF_GPIOTE_INITIAL_VALUE_LOW, 
    .task_pin   = true,
    .action     = NRF_GPIOTE_POLARITY_HITOLO,
  };
  ret_code = nrf_drv_gpiote_out_init(PUMP_HV_PIN, &pumpOut);
  APP_ERROR_CHECK(ret_code);

  // timer for cycle control.
  nrf_drv_timer_config_t timer_cfg = 
  {
    .frequency          = NRF_TIMER_FREQ_16MHz,
    .mode               = NRF_TIMER_MODE_TIMER,
    .bit_width          = NRF_TIMER_BIT_WIDTH_16,
    .interrupt_priority = TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
    .p_context          = NULL
  };
  ret_code = nrf_drv_timer_init(&TIMER_HV, &timer_cfg, timer1_event_handler);
  APP_ERROR_CHECK(ret_code);
  
  // setting timing for all steps
  tics_duty_on = nrf_drv_timer_us_to_ticks(&TIMER_HV, DUTY_ON_TIME);
  tics_meas_fb =    tics_duty_on + nrf_drv_timer_us_to_ticks(&TIMER_HV, FEDBACK_MEAS_DELAY);

  nrf_drv_timer_compare(&TIMER_HV, NRF_TIMER_CC_CHANNEL0, tics_duty_on, false); // pump pulse
  nrf_drv_timer_extended_compare(&TIMER_HV, NRF_TIMER_CC_CHANNEL1, tics_meas_fb, NRF_TIMER_SHORT_COMPARE1_STOP_MASK, true); // pause for charge feedback capacitor
    
  //PPI init and allocate
  ret_code = nrf_drv_ppi_init();
  APP_ERROR_CHECK(ret_code);
  ret_code = nrf_drv_ppi_channel_alloc(&ppi_ch_tim1_gpiote);
  APP_ERROR_CHECK(ret_code);
  
  // Connecting TIMER_HV CC[0] event to GPIOTE task (turn OFF PUMP_HV_PIN).
  // It's need for make a pulse with DUTY_ON
  eventAddr =  nrf_drv_timer_event_address_get (&TIMER_HV, NRF_TIMER_EVENT_COMPARE0);
  taskAddr =   nrf_drv_gpiote_out_task_addr_get (PUMP_HV_PIN);
  ret_code = nrf_drv_ppi_channel_assign (ppi_ch_tim1_gpiote, eventAddr, taskAddr);
  APP_ERROR_CHECK(ret_code);
  
  ret_code = nrf_drv_ppi_channel_enable(ppi_ch_tim1_gpiote);
  APP_ERROR_CHECK(ret_code);
  nrf_drv_gpiote_out_task_enable(PUMP_HV_PIN);

  // ADC init
  nrf_drv_adc_config_t adc_config = NRF_DRV_ADC_DEFAULT_CONFIG;
  ret_code = nrf_drv_adc_init(&adc_config, adc_event_handler);
  APP_ERROR_CHECK(ret_code);
  nrf_adc_config_set(Vbat_ch_config.config.data);

  // LPCOMP init version hal
  nrf_lpcomp_config_t   lpcomp_config=
  {
    .reference = LPCOM_REF,
    .detection = NRF_LPCOMP_DETECT_UP
  };
  nrf_lpcomp_configure(&lpcomp_config);
  nrf_lpcomp_input_select(FEEDBACK);
  nrf_lpcomp_int_enable(LPCOMP_INTENSET_READY_Msk);
  nrf_lpcomp_shorts_enable(NRF_LPCOMP_SHORT_UP_STOP_MASK);
  nrf_lpcomp_shorts_enable(NRF_LPCOMP_SHORT_READY_SAMPLE_MASK);
  nrf_drv_common_irq_enable(LPCOMP_IRQn,  LPCOMP_CONFIG_IRQ_PRIORITY);
  nrf_drv_lpcomp_event_handler_register(lpcomp_event_handler); 
  
  //enable work HV sypply source
  HV_param.enable = 1;  
  AnalogModule_state = HVU_STATE_IDLE;

  while(1)
  {
    for(uint32_t i=0; i<400000; i++)
      NRF_LOG_PROCESS();
    analogPart_timeout_handler(NULL);
  }
}

/* *******************************************************
  Battery level get(in %) 
******************************************************** */
uint8_t battery_level_get()
{
  // adc value / 8.5
  return ((HV_param.BatteryVoltage *2) / 17 );
}
