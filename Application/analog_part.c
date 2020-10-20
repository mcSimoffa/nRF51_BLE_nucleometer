#include "analog_part.h"

#define NRF_LOG_MODULE_NAME "ANALOG"
//#define NRF_LOG_LEVEL NRF_LOG_LEVEL_ERROR
#define NRF_LOG_LEVEL NRF_LOG_LEVEL_INFO

#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#define MAX_CONTINOUS_PUMPS       10      // maximum times pump cycle continous
#define DUTY_ON_TIME              10      // uS. Time ti pumpON
#define FEDBACK_MEAS_DELAY        10      // uS. Since this time ADC start measure feedback voltage
#define AFTER_ADC_DELAY           50000   // uS. Pause after ADC feedback before next pump cycle

#define PUMP_HV_PIN               0
#define AIN_BATTERY               ADC_CONFIG_PSEL_AnalogInput4  // it's P0.3

//#define PIN_FEEDBACK_HV           2
//#define AIN_FEEDBACK_HV           ADC_CONFIG_PSEL_AnalogInput3  // it's P0.2

#define PULSE_PIN                 30  //for count pulse from Geyger-Miller counter

//#define HV_FEEDBACK_NOMINAL       0x0300  //ADC value what is correspondent to 380V secondary power syplay
//#define HV_FEEDBACK_OVERVOLTAGE   0x0380  //ADC overvoltage value 

// Analog module machine state
#define HVU_STATE_DENY            0
#define HVU_STATE_IDLE            1
#define HVU_STATE_PROCESS         2
#define HVU_STATE_VBAT_MEA        3

// *************************************************************************************************************
uint8_t AnalogModule_state = HVU_STATE_DENY;

static nrf_drv_adc_channel_t Vbat_ch_config =   
{{{
    .resolution = NRF_ADC_CONFIG_RES_10BIT,
    .input      = NRF_ADC_CONFIG_SCALING_INPUT_ONE_THIRD,
    .reference  = NRF_ADC_CONFIG_REF_VBG,
    .ain        = AIN_BATTERY
 }}, NULL};

//static nrf_drv_adc_channel_t fb_HV_ch_config =  NRF_DRV_ADC_DEFAULT_CHANNEL(AIN_FEEDBACK_HV); 

static struct 
{
  uint8_t   enable;
  uint8_t   pump_quant;
  uint16_t  feedbackVoltage;
  uint16_t  BatteryVoltage;
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
  nrf_drv_lpcomp_enable();  // enable and start comparator

  nrf_drv_timer_clear(&TIMER_HV);
  nrf_drv_gpiote_out_task_force(PUMP_HV_PIN, 1);  // rising PUMP_HV_PIN
  nrf_drv_timer_enable(&TIMER_HV);                // start watch for state ON
}

/* *************************************************************
 This handler is called each ANALOGPART_TIMER_INTERVAL (RTC) expiration
 It give start analog process according algoritm:
 make rising on PUMP_HV_PIN -> TIMER1 1 start -> CC[0] -> falling PUMP_HV_PIN
 -> CC[1] ->start ADC -> adc_event_handler ->  TIMER1 1 start -> CC[2] ->
 -> make rising OR start ADC (Vbat) -> adc_event_handler -> HVU_STATE_IDLE
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
  else
    NRF_LOG_INFO("Analog part is late\r\n");
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

  if (AnalogModule_state == HVU_STATE_VBAT_MEA)
  {
    HV_param.BatteryVoltage = p_event->data.sample.sample;
    AnalogModule_state = HVU_STATE_IDLE;
    NRF_LOG_INFO("ADC VBAT %X\r\n", HV_param.BatteryVoltage);
  }
  else 
    AnalogModule_state = HVU_STATE_IDLE;
}

/* *************************************************************
 This handler is called on TIMER_HV timer interrupt occur
 It give start for process:
 if feedback is low
    Next pump cycle 
  else Battery level measurement
************************************************************** */
static void timer1_event_handler(nrf_timer_event_t event_type, void* p_context)
{
  if (event_type == NRF_TIMER_EVENT_COMPARE1)

  if (event_type != NRF_TIMER_EVENT_COMPARE2)
    return;
  
   if ((HV_param.feedbackVoltage < HV_FEEDBACK_NOMINAL) && (++HV_param.pump_quant < MAX_CONTINOUS_PUMPS))
  {
    nrf_gpio_pin_toggle(LED_2);   // (Green) it's temporary for debug
    make_rising_pump();
  }
  else
  {
    AnalogModule_state = HVU_STATE_VBAT_MEA;

    // Prepare and Start ADC for Battery level measure     
    //nrf_adc_config_set(Vbat_ch_config.config.data);
    nrf_adc_enable();
    nrf_adc_int_enable(NRF_ADC_INT_END_MASK);
    nrf_adc_start();
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
  ret_code_t          ret_code;
  nrf_ppi_channel_t   ppi_ch_tim1_gpiote;

  memset(&HV_param,0, sizeof(HV_param));

  //configure PUMP_HV_PIN for controlling by Event
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
    .frequency          = NRF_TIMER_FREQ_1MHz,
    .mode               = NRF_TIMER_MODE_TIMER,
    .bit_width          = NRF_TIMER_BIT_WIDTH_16,
    .interrupt_priority = TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
    .p_context          = NULL
  };
  ret_code = nrf_drv_timer_init(&TIMER_HV, &timer_cfg, timer1_event_handler);
  APP_ERROR_CHECK(ret_code);
  
  //it's temporary.
  nrf_gpio_cfg_output(1);
  nrf_gpio_pin_set(1);

  // setting timing for all steps
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

  //LPCOMP init
  nrf_lpcomp_config_t lpcomp_config;
  lpcomp_config.detection = NRF_LPCOMP_DETECT_UP;
  lpcomp_config.reference = 
  .hal.reference = NRF_LPCOMP_REF_SUPPLY_5_8;
  lpcomp_config.input = NRF_LPCOMP_INPUT_3;
  nrf_lpcomp_configure(&lpcomp_config);
  APP_ERROR_CHECK(ret_code);
  
  HV_param.enable = 1;  //enable work HV sypply source
  AnalogModule_state = HVU_STATE_IDLE;
}

/* *******************************************************
  Battery level get(in %) 
******************************************************** */
uint8_t battery_level_get()
{
  //See the circuit diagramm. Calculation formula there
  return ((HV_param.BatteryVoltage *3) / 25 );
}
