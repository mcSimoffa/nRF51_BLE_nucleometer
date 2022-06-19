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
#define CYC_TIMER_FREQ                NRF_TIMER_FREQ_1MHz
#define CYC_TIMER_WIDTH_BITS          16      //8 or 16

//HV module default parameters
#define PHASE_ON_NS       10000   //default time mosfet open phase
#define PHASE_ON_TRY_NS   8000    //default time mosfet open phase in first cycle (to prevent overcahrge) 
#define WORK_PAUSE_MS     100     //default timespan between pulses in charging state
#define STEADY_PAUSE_MS   1000    //default timespan between pulses in steady state

// HV module constants
#define FLY_BACK_TIME_NS    20000   //timespan to feedback voltage control
#define HW_PW_UP_DELAYE_MS  1000    //timespan before first pulse after device powering

// ----------------------------------------------------------------------------
//  OTHER DEFINES
// ----------------------------------------------------------------------------
#define NS_TO_TICK(ns)((ns) * 16 / (1 << CYC_TIMER_FREQ) / 1000)

#define CYC_TIMER_WIDTH   CONCAT_2(NRF_TIMER_BIT_WIDTH_, CYC_TIMER_WIDTH_BITS)
#define CYCTIMER_RANGE    (1 << CYC_TIMER_WIDTH_BITS) - 1

// ----------------------------------------------------------------------------
//   PRIVATE TYPES
// ----------------------------------------------------------------------------
typedef struct
{
  struct
  {
    uint16_t  on_phase;
    uint16_t  on_try_phase;
  } cycTimer;

  struct
  {
    uint16_t  work_pause;
    uint16_t  steady_pause;
  } workTimer;
} hv_params_t;

// ----------------------------------------------------------------------------
typedef struct
{
  struct
  {
    uint16_t  CC1_work;
    uint16_t  CC2_work;
    uint16_t  CC1_try;
    uint16_t  CC2_try;
  } cycTimer;

  struct
  {
    uint32_t  work_pause;
    uint32_t  steady_pause;
  } workTimer;  
} hv_data_t;


// ----------------------------------------------------------------------------
//   PRIVATE VARIABLE
// ----------------------------------------------------------------------------
APP_TIMER_DEF(mainHVtmr);
static const nrf_drv_timer_t cycCtrlTmr = NRF_DRV_TIMER_INSTANCE(1);
static nrf_ppi_channel_t ppi_ch_tim1_gpiote_rising, ppi_ch_tim1_gpiote_falling;
static nrf_ppi_channel_t ppi_ch_tim1_gpiote_risingDrv, ppi_ch_tim1_gpiote_fallingDrv;
static bool enough_hv_fb;     //enough high voltage feedback
hv_params_t hv_params =
{
  .cycTimer.on_phase      = PHASE_ON_NS,
  .cycTimer.on_try_phase  = PHASE_ON_TRY_NS,
  .workTimer.work_pause   = WORK_PAUSE_MS,
  .workTimer.steady_pause = STEADY_PAUSE_MS,
};

hv_data_t hv_data;
// ----------------------------------------------------------------------------
//    PRIVATE FUNCTION
// ----------------------------------------------------------------------------
static bool hvDataRefresh(hv_params_t *param)
{
  uint32_t CC1_work = 1 + NS_TO_TICK(param->cycTimer.on_phase);
  uint32_t CC1_try  = 1 + NS_TO_TICK(param->cycTimer.on_try_phase);
  uint32_t CC2_work = CC1_work + NS_TO_TICK(FLY_BACK_TIME_NS);
  uint32_t CC2_try  = CC1_try  + NS_TO_TICK(FLY_BACK_TIME_NS);

  uint32_t work_pause    = MS_TO_TICK(param->workTimer.work_pause);
  uint32_t steady_pause  = MS_TO_TICK(param->workTimer.steady_pause);
  
  if ((CC1_work == 1) ||
      (CC1_try == 1)  ||
      (CC2_work == CC1_work) ||
      (CC2_try == CC1_try)   ||
      (CC2_work >= CYCTIMER_RANGE) ||
      (CC2_try  >= CYCTIMER_RANGE) ||
      (work_pause > steady_pause))
  {
    return false;
  }     
  else
  {
    hv_data.cycTimer.CC1_work = (uint16_t)CC1_work;
    hv_data.cycTimer.CC1_try  = (uint16_t)CC1_try;
    hv_data.cycTimer.CC2_work = (uint16_t)CC2_work;
    hv_data.cycTimer.CC2_try  = (uint16_t)CC2_try;
    hv_data.workTimer.work_pause   = work_pause;
    hv_data.workTimer.steady_pause = steady_pause;
    return true;
  }
}

// ---------------------------------------------------------------------------
static void cycTimer_adjust(uint16_t cc1, uint16_t cc2)
{
  nrf_drv_timer_compare(&cycCtrlTmr, NRF_TIMER_CC_CHANNEL1, cc1, false); // fallig forming ---|___
  nrf_drv_timer_extended_compare(&cycCtrlTmr, NRF_TIMER_CC_CHANNEL2, cc2,
                                  NRF_TIMER_SHORT_COMPARE2_STOP_MASK | TIMER_SHORTS_COMPARE2_CLEAR_Msk,
                                  true); 

}

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

// ****************************************************************************
static void OnMainTmr(void* context)
{
  (void)context;

  // enable LPCOMP - first step
  lpcomp_ctrl(true);
  NRF_LOG_DEBUG("Main timer\n");
}

// ---------------------------------------------------------------------------
static void OnLpcomp(nrf_lpcomp_event_t event)
{
  static uint8_t pulse_num = 0; //0 -first, 1 - second, 2 - another bigger pulses

  if (event == NRF_LPCOMP_EVENT_READY)
  {
    // start work cycle timer - second step
    timer_anomaly_fix(cycCtrlTmr.p_reg, 1);
    if (pulse_num == 1)
    {
      cycTimer_adjust(hv_data.cycTimer.CC1_work, hv_data.cycTimer.CC2_work);
      pulse_num = 2;
    }

    if (pulse_num == 0)
    {
      cycTimer_adjust(hv_data.cycTimer.CC1_try, hv_data.cycTimer.CC2_try);
      nrf_drv_timer_enable(&cycCtrlTmr);
      pulse_num = 1;
    }
    else
    {
      nrf_drv_timer_resume(&cycCtrlTmr);
    }
  }
  else if (event == NRF_LPCOMP_EVENT_UP)
  {
    nrf_drv_timer_disable(&cycCtrlTmr);
    enough_hv_fb = true;
    pulse_num = 0;
    NRF_LOG_DEBUG("LPC Up\n");
  }
}

// ---------------------------------------------------------------------------
static void OnCycCtrlTmr(nrf_timer_event_t event_type, void * p_context)
{
  static uint32_t cnt = 0;
  uint32_t interval;

  if (event_type == NRF_TIMER_EVENT_COMPARE2)
  {

    timer_anomaly_fix(cycCtrlTmr.p_reg, 0);
    lpcomp_ctrl(false);
  
    if (enough_hv_fb)
    {
      enough_hv_fb = false;
      cnt = 0;
      interval = hv_data.workTimer.steady_pause;
      NRF_LOG_INFO("Enough HV\n");
    }
    else
    {
      interval = hv_data.workTimer.work_pause;
      ++cnt;
      NRF_LOG_INFO("NOT Enough (%d) HV\n", cnt);
    }

    ret_code_t err_code = app_timer_start(mainHVtmr, interval, NULL);
    ASSERT(err_code == NRF_SUCCESS);
  }
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
static void cycTimer_init(void)
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
  nrf_drv_timer_compare(&cycCtrlTmr, NRF_TIMER_CC_CHANNEL0, 1, false);  // rising forming ___|---
  cycTimer_adjust(hv_data.cycTimer.CC1_try, hv_data.cycTimer.CC2_try);
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
  if (hvDataRefresh(&hv_params) == false)
  {
    ASSERT(false);
    return;
  }
  mainTimet_init();  
  hv_gpio_init();
  cycTimer_init();
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