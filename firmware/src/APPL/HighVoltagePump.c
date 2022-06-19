#include <sdk_common.h>
#include "nrf_log_ctrl.h"
#include "nrf_drv_timer.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpio_adds.h"
#include "nrf_drv_lpcomp.h"
#include "nrf_timer.h"
#include "app_timer.h"
#include "Timer_anomaly_fix.h"
#include "app_time_lib.h"

#include "HighVoltagePump.h"

#define NRF_LOG_MODULE_NAME "HVP"
#define NRF_LOG_LEVEL       3
#include "nrf_log.h"


// ----------------------------------------------------------------------------
//  DEFINE MODULE PARAMETER
// ----------------------------------------------------------------------------
#define CYC_TIMER_FREQ                NRF_TIMER_FREQ_16MHz
#define CYC_TIMER_WIDTH_BITS          16      //8 or 16 or 24 or 32
#define DUTY_ON_TIME_NS               10000
#define FLY_BACK_TIME_NS              20000
#define MAIN_HW_TIMER_INTERVAL_MS     100
#define MAIN_HW_TIMER_PAUSE_MS        2000
#define HW_PW_UP_DELAYE_MS            1000

// ----------------------------------------------------------------------------
//  OTHER DEFINES
// ----------------------------------------------------------------------------
#define NS_TO_TICK(ns)((ns) * 16 / (1 << CYC_TIMER_FREQ) / 1000)

// timings static check
#if ((DUTY_ON_TIME_NS + FLY_BACK_TIME_NS) * 2 > MAIN_HW_TIMER_INTERVAL_MS * 1000000)
#error "HV PUMP CONTROL TIMING ERROR"
#endif

#define CYC_TIMER_WIDTH   CONCAT_2(NRF_TIMER_BIT_WIDTH_, CYC_TIMER_WIDTH_BITS)
#define CYCTIMER_RANGE    (1 << CYC_TIMER_WIDTH_BITS) - 1

// ----------------------------------------------------------------------------
//   PRIVATE VARIABLE
// ----------------------------------------------------------------------------
APP_TIMER_DEF(mainHVtmr);
static const nrf_drv_timer_t cycCtrlTmr = NRF_DRV_TIMER_INSTANCE(1);
static nrf_ppi_channel_t ppi_ch_tim1_gpiote_rising, ppi_ch_tim1_gpiote_falling;
static nrf_ppi_channel_t ppi_ch_tim1_gpiote_risingDrv, ppi_ch_tim1_gpiote_fallingDrv;
static bool enough_hv_fb;     //enough high voltage feedback
// ----------------------------------------------------------------------------
//    PRIVATE FUNCTION
// ----------------------------------------------------------------------------

static void lpcomp_ctrl(bool en)
{
  if (en)
  {
    nrf_lpcomp_enable();
    nrf_lpcomp_task_trigger(NRF_LPCOMP_TASK_START);
  }
  else
  {
    nrf_lpcomp_disable();
    nrf_lpcomp_task_trigger(NRF_LPCOMP_TASK_STOP);
  }
}

// ---------------------------------------------------------------------------
static void OnMainTmr(void* context)
{
  (void)context;

  // enable LPCOMP - first step
  lpcomp_ctrl(true);
  NRF_LOG_INFO("Main timer\n");
}

// ---------------------------------------------------------------------------
static void OnLpcomp(nrf_lpcomp_event_t event)
{
  static bool isFirstCycle = true;

  if (event == NRF_LPCOMP_EVENT_READY)
  {
    // start work cycle timer - second step
    timer_anomaly_fix(cycCtrlTmr.p_reg, 1);
    if (isFirstCycle)
    {
      isFirstCycle = false;
      nrf_drv_timer_enable(&cycCtrlTmr);
    }
    else
    {
      nrf_drv_timer_resume(&cycCtrlTmr);
  }
  }
  else if (event == NRF_LPCOMP_EVENT_UP)
  {
    enough_hv_fb = true;
    NRF_LOG_INFO("LPC\n");
  }
}

// ---------------------------------------------------------------------------
static void OnCycCtrlTmr(nrf_timer_event_t event_type, void * p_context)
{
  static uint32_t cnt = 0;
  uint32_t interval = MS_TO_TICK(MAIN_HW_TIMER_INTERVAL_MS);

  timer_anomaly_fix(cycCtrlTmr.p_reg, 0);
  lpcomp_ctrl(false);
  
  if (enough_hv_fb)
  {
    enough_hv_fb = false;
    cnt = 0;
    interval = MS_TO_TICK(MAIN_HW_TIMER_PAUSE_MS);
    NRF_LOG_INFO("Enough HV\n");
  }
  else
  {
    ++cnt;
    NRF_LOG_INFO("NOT Enough (%d) HV\n", cnt);
  }

  ret_code_t err_code = app_timer_start(mainHVtmr, interval, NULL);
  ASSERT(err_code == NRF_SUCCESS);
}

// ****************************************************************************
static void mainTimet_init(void)
{
  ret_code_t err_code = app_timer_create(&mainHVtmr, APP_TIMER_MODE_SINGLE_SHOT, OnMainTmr);
  ASSERT(err_code == NRF_SUCCESS);
}

// ---------------------------------------------------------------------------
static void lpcomp_init(void)
{
  uint32_t err_code;
  nrf_drv_lpcomp_config_t config = NRF_DRV_LPCOMP_DEFAULT_CONFIG;

  err_code = nrf_drv_lpcomp_init(&config, OnLpcomp);
  ASSERT(err_code == NRF_SUCCESS);
  nrf_lpcomp_int_enable(LPCOMP_INTENSET_READY_Msk);
}

// ---------------------------------------------------------------------------
//configure PUMP_HV_PIN for controlling by Event
static void hv_gpio_init(void)
{
  ret_code_t err_code;
  if (!nrf_drv_gpiote_is_init())
  {
    err_code = nrf_drv_gpiote_init();
    ASSERT(err_code == NRF_SUCCESS);
  }

  nrf_drv_gpiote_out_config_t pumpOut = 
  {
    .init_state = PUMP_HV_PIN_INACTIVE_STATE, 
    .task_pin   = true,
    .action     = NRF_GPIOTE_POLARITY_TOGGLE,
  };

  err_code = nrf_drv_gpiote_out_init(PUMP_HV_PIN, &pumpOut);
  ASSERT(err_code == NRF_SUCCESS);
  nrf_gpio_cfg_strong_output(PUMP_HV_PIN);

  err_code = nrf_drv_gpiote_out_init(PUMP_HV_PIN_DRV, &pumpOut);
  ASSERT(err_code == NRF_SUCCESS);
  nrf_gpio_cfg_strong_output(PUMP_HV_PIN_DRV);
}

// ---------------------------------------------------------------------------
// cycle control timer init
static void cycTimet_init(void)
{
  // timer for cycle control.
  nrf_drv_timer_config_t timer_cfg = 
  {
    .frequency          = CYC_TIMER_FREQ,
    .mode               = NRF_TIMER_MODE_TIMER,
    .bit_width          = CYC_TIMER_WIDTH,
    .interrupt_priority = TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
    .p_context          = NULL
  };
  ret_code_t err_code = nrf_drv_timer_init(&cycCtrlTmr, &timer_cfg, OnCycCtrlTmr);
  ASSERT(err_code == NRF_SUCCESS);

  // setting timing for 2 steps
  uint32_t tics_duty_on = NS_TO_TICK(DUTY_ON_TIME_NS);
  ASSERT(tics_duty_on);

  uint32_t tics_flyBack = NS_TO_TICK(FLY_BACK_TIME_NS);
  ASSERT(tics_flyBack);

  ASSERT(1 + tics_duty_on + tics_flyBack < CYCTIMER_RANGE);
   
  nrf_drv_timer_compare(&cycCtrlTmr, NRF_TIMER_CC_CHANNEL0, 1, false);                // rising forming ___|---
  nrf_drv_timer_compare(&cycCtrlTmr, NRF_TIMER_CC_CHANNEL1, 1 + tics_duty_on, false); // fallig forming ---|___
  nrf_drv_timer_extended_compare(&cycCtrlTmr, NRF_TIMER_CC_CHANNEL2,
                                  1 + tics_duty_on + tics_flyBack,
                                  NRF_TIMER_SHORT_COMPARE2_STOP_MASK | TIMER_SHORTS_COMPARE2_CLEAR_Msk,
                                  true); 


}

// ---------------------------------------------------------------------------
//PPI init and allocate 2 channel to control hv gpio pin from timer
static void hv_ppi_init(void)
{ 
  ret_code_t err_code = nrf_drv_ppi_init();
  ASSERT(err_code == NRF_SUCCESS);

  err_code = nrf_drv_ppi_channel_alloc(&ppi_ch_tim1_gpiote_rising);
  ASSERT(err_code == NRF_SUCCESS);

  err_code = nrf_drv_ppi_channel_alloc(&ppi_ch_tim1_gpiote_falling);
  ASSERT(err_code == NRF_SUCCESS);

  err_code = nrf_drv_ppi_channel_alloc(&ppi_ch_tim1_gpiote_risingDrv);
  ASSERT(err_code == NRF_SUCCESS);

  err_code = nrf_drv_ppi_channel_alloc(&ppi_ch_tim1_gpiote_fallingDrv);
  ASSERT(err_code == NRF_SUCCESS);
}

// ---------------------------------------------------------------------------
// Connecting  CC[0] CC[1] events to GPIOTE task
// It's need for make a pulse with DUTY_ON
static void bind_gpio_to_timer(void)
{
  uint32_t event_CC0_Addr =  nrf_drv_timer_event_address_get(&cycCtrlTmr, NRF_TIMER_EVENT_COMPARE0);
  uint32_t event_CC1_Addr =  nrf_drv_timer_event_address_get(&cycCtrlTmr, NRF_TIMER_EVENT_COMPARE1);
  uint32_t taskAddr =   nrf_drv_gpiote_out_task_addr_get(PUMP_HV_PIN);
  uint32_t taskAddrDrv =   nrf_drv_gpiote_out_task_addr_get(PUMP_HV_PIN_DRV);

  ret_code_t err_code = nrf_drv_ppi_channel_assign (ppi_ch_tim1_gpiote_rising, event_CC0_Addr, taskAddr);
  ASSERT(err_code == NRF_SUCCESS);

  err_code = nrf_drv_ppi_channel_assign (ppi_ch_tim1_gpiote_risingDrv, event_CC0_Addr, taskAddrDrv);
  ASSERT(err_code == NRF_SUCCESS);

  err_code = nrf_drv_ppi_channel_assign (ppi_ch_tim1_gpiote_falling, event_CC1_Addr, taskAddr);
  ASSERT(err_code == NRF_SUCCESS);

  err_code = nrf_drv_ppi_channel_assign (ppi_ch_tim1_gpiote_fallingDrv, event_CC1_Addr, taskAddrDrv);
  ASSERT(err_code == NRF_SUCCESS);
}


// ----------------------------------------------------------------------------
//    PUBLIC FUNCTION
// ----------------------------------------------------------------------------
void HV_pump_Init(void)
{
  mainTimet_init();  
  hv_gpio_init();
  cycTimet_init();
  hv_ppi_init();
  lpcomp_init();
  bind_gpio_to_timer();
}


// ----------------------------------------------------------------------------
void HV_pump_Startup(void)
{
  ret_code_t err_code = nrf_drv_ppi_channel_enable(ppi_ch_tim1_gpiote_rising);
  ASSERT(err_code == NRF_SUCCESS);

  err_code = nrf_drv_ppi_channel_enable(ppi_ch_tim1_gpiote_falling);
  ASSERT(err_code == NRF_SUCCESS);
 
  err_code = nrf_drv_ppi_channel_enable(ppi_ch_tim1_gpiote_risingDrv);
  ASSERT(err_code == NRF_SUCCESS);

  err_code = nrf_drv_ppi_channel_enable(ppi_ch_tim1_gpiote_fallingDrv);
  ASSERT(err_code == NRF_SUCCESS);
   
  nrf_drv_gpiote_out_task_enable(PUMP_HV_PIN);
  nrf_drv_gpiote_out_task_enable(PUMP_HV_PIN_DRV);

  err_code = app_timer_start(mainHVtmr, MS_TO_TICK(HW_PW_UP_DELAYE_MS), NULL);
  ASSERT(err_code == NRF_SUCCESS);

}