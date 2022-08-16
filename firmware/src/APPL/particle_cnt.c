#include <sdk_common.h>
#include "nrf_log_ctrl.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_timer.h"
#include "nrf_drv_ppi.h"
#include "app_timer.h"
#include "app_time_lib.h"
#include "ble_main.h"

#define NRF_LOG_MODULE_NAME "PCNT"
#define NRF_LOG_LEVEL       4
#include "nrf_log.h"

// ----------------------------------------------------------------------------
//  DEFINE MODULE PARAMETER
// ----------------------------------------------------------------------------
#define WATCH_INTERVAL_DEFAULT    1000


// ----------------------------------------------------------------------------
//   PRIVATE TYPES
// ----------------------------------------------------------------------------
typedef union
{
  struct
  {
    uint16_t low;
    uint16_t high;
  } field;
  uint32_t  word;
} pulse_cnt_t;


// ----------------------------------------------------------------------------
//   PRIVATE VARIABLE
// ----------------------------------------------------------------------------
static nrf_ppi_channel_t ppi_pulse_to_count;
static const nrf_drv_timer_t cntTmr = NRF_DRV_TIMER_INSTANCE(2);
APP_TIMER_DEF(main_cnt_timer);
pulse_cnt_t pulse_cnt;
uint32_t    watch_interval =  MS_TO_TICK(WATCH_INTERVAL_DEFAULT);


// ----------------------------------------------------------------------------
//    PRIVATE FUNCTION
// ----------------------------------------------------------------------------
static void OnMainTmr(void* context)
{
  (void)context;

  uint16_t totalPulse = (uint16_t)nrf_drv_timer_capture(&cntTmr, NRF_TIMER_CC_CHANNEL0);
  if (totalPulse < pulse_cnt.field.low)
  {
    pulse_cnt.field.high++;
  }
  pulse_cnt.field.low = totalPulse;

  NRF_LOG_DEBUG("total pulses %d\n", pulse_cnt.word);

  ble_ios_pulse_hvx(pulse_cnt.word);

  ret_code_t err_code = app_timer_start(main_cnt_timer, watch_interval, NULL);
  ASSERT(err_code == NRF_SUCCESS);
}

// --------------------------------------------------------------------------
static void onTimer(nrf_timer_event_t event_type, void * p_context)
{
  NRF_LOG_DEBUG("onTimer");
}

// ---------------------------------------------------------------------------
// PULSE_PIN init
static void cnt_gpio_init(void)
{
  nrf_drv_gpiote_in_config_t pulse_conf = GPIOTE_CONFIG_IN_SENSE_LOTOHI(false);
  ret_code_t err_code;

  err_code = nrf_drv_gpiote_in_init(PULSE_PIN, &pulse_conf, NULL);
  ASSERT(err_code == NRF_SUCCESS);
}

// ---------------------------------------------------------------------------
// pulse counter timer init
static void cnt_timer_init(void)
{
  nrf_drv_timer_config_t timer_cfg = 
  {
    .frequency          = NRF_TIMER_FREQ_31250Hz,
    .mode               = NRF_TIMER_MODE_COUNTER,
    .bit_width          = NRF_TIMER_BIT_WIDTH_16,
    .interrupt_priority = TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
    .p_context          = NULL
  };

  ret_code_t err_code = nrf_drv_timer_init(&cntTmr, &timer_cfg, onTimer);
  ASSERT(err_code == NRF_SUCCESS);
}

// ---------------------------------------------------------------------------
//PPI allocate channel to pass PULSE_PIN event to pulse counter timer
static void cnt_ppi_alloc(void)
{ 
  ret_code_t err_code = nrf_drv_ppi_channel_alloc(&ppi_pulse_to_count);
  ASSERT(err_code == NRF_SUCCESS);

}

// ---------------------------------------------------------------------------
// Connecting  LoToHi PULSE_PIN event to  COUNT task of timer
static void cnt_ppi_bind(void)
{
  uint32_t count_task_addr  =  nrf_drv_timer_task_address_get(&cntTmr, NRF_TIMER_TASK_COUNT);
  uint32_t pulse_event_addr =  nrf_drv_gpiote_in_event_addr_get(PULSE_PIN);

  ret_code_t err_code = nrf_drv_ppi_channel_assign (ppi_pulse_to_count, pulse_event_addr, count_task_addr);
  ASSERT(err_code == NRF_SUCCESS);
}

// ---------------------------------------------------------------------------
static void main_timer_init(void)
{
  ret_code_t err_code = app_timer_create(&main_cnt_timer, APP_TIMER_MODE_SINGLE_SHOT, OnMainTmr);
  ASSERT(err_code == NRF_SUCCESS);
}


// ----------------------------------------------------------------------------
//    PUBLIC FUNCTION
// ----------------------------------------------------------------------------
void particle_cnt_Init(void)
{
  cnt_gpio_init();
  cnt_timer_init();
  cnt_ppi_alloc();
  cnt_ppi_bind();
  main_timer_init();
}

// ---------------------------------------------------------------------------
void particle_cnt_Startup()
{
  nrf_drv_gpiote_in_event_enable(PULSE_PIN, false);

  ret_code_t err_code = nrf_drv_ppi_channel_enable(ppi_pulse_to_count);
  ASSERT(err_code == NRF_SUCCESS);
  
  nrf_drv_timer_enable(&cntTmr);

  err_code = app_timer_start(main_cnt_timer, watch_interval, NULL);
  ASSERT(err_code == NRF_SUCCESS);
}

// ---------------------------------------------------------------------------
void particle_cnt_WatchIntervalSet(uint32_t ms)
{
  if ((ms >= 100) && (ms <= 120000))
  {
    CRITICAL_REGION_ENTER();
    watch_interval = MS_TO_TICK(ms);
    CRITICAL_REGION_EXIT();
  }
}