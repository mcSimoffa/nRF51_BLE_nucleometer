#include <stdint.h>
#include <stdbool.h>
#include "nrf_log_ctrl.h"
#include "nrf_drv_timer.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_gpiote.h"
#include "app_timer.h"
#include "app_time_lib.h"

#include "HighVoltagePump.h"

#include "nrf_log.h"
#define PUMP_HV_PIN               0

#define CYC_TIMER_FREQ            NRF_TIMER_FREQ_16MHz
#define DUTY_ON_TIME_NS           100
#define MAIN_HW_TIMER_INTERVAL_MS 1000

#define MAIN_HW_TIMER_INTERVAL    APP_TIMER_TICKS(MAIN_HW_TIMER_INTERVAL_MS, APP_TIMER_PRESCALER)

#define NS_TO_TICK(ns)((ns) * 16 / (1 >> CYC_TIMER_FREQ) / 1000)

APP_TIMER_DEF(mainHVtmr);
const nrf_drv_timer_t cycCtrlTmr = NRF_DRV_TIMER_INSTANCE(1);

// ----------------------------------------------------------------------------
static void OnMainTmr(void* context)
{
  (void)context;
}

// ---------------------------------------------------------------------------
static void OnCycCtrlTmr(nrf_timer_event_t event_type, void * p_context)
{

}


/*! ---------------------------------------------------------------------------
  \brief  
  \details  

  \return  
 ----------------------------------------------------------------------------*/
void HV_pump_Init(void)
{
  ret_code_t err_code = app_timer_create(&mainHVtmr, APP_TIMER_MODE_SINGLE_SHOT, OnMainTmr);
  ASSERT(err_code == NRF_SUCCESS);
  
  //configure PUMP_HV_PIN for controlling by Event
  if (!nrf_drv_gpiote_is_init())
  {
    err_code = nrf_drv_gpiote_init();
    ASSERT(err_code == NRF_SUCCESS);
  }

  nrf_drv_gpiote_out_config_t pumpOut = 
  {
    .init_state = NRF_GPIOTE_INITIAL_VALUE_LOW, 
    .task_pin   = true,
    .action     = NRF_GPIOTE_POLARITY_TOGGLE,
  };
  err_code = nrf_drv_gpiote_out_init(PUMP_HV_PIN, &pumpOut);
  ASSERT(err_code == NRF_SUCCESS);

  // timer for cycle control.
  nrf_drv_timer_config_t timer_cfg = 
  {
    .frequency          = CYC_TIMER_FREQ,
    .mode               = NRF_TIMER_MODE_TIMER,
    .bit_width          = NRF_TIMER_BIT_WIDTH_16,
    .interrupt_priority = TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
    .p_context          = NULL
  };
  err_code = nrf_drv_timer_init(&cycCtrlTmr, &timer_cfg, OnCycCtrlTmr);
  ASSERT(err_code == NRF_SUCCESS);

  // setting timing for 2 steps
  uint32_t tics_duty_on = NS_TO_TICK(DUTY_ON_TIME_NS);
  ASSERT(tics_duty_on);

  nrf_drv_timer_compare(&cycCtrlTmr, NRF_TIMER_CC_CHANNEL0, 1, false);  // rising forming ___|--
  nrf_drv_timer_extended_compare(&cycCtrlTmr, NRF_TIMER_CC_CHANNEL1,
                                  1 + tics_duty_on,
                                  NRF_TIMER_SHORT_COMPARE1_STOP_MASK | TIMER_SHORTS_COMPARE1_CLEAR_Msk,
                                  false); // fallig forming ---|___

  //PPI init and allocate 2 channel
  nrf_ppi_channel_t   ppi_ch_tim1_gpiote_rising, ppi_ch_tim1_gpiote_falling;
  err_code = nrf_drv_ppi_init();
  ASSERT(err_code == NRF_SUCCESS);

  err_code = nrf_drv_ppi_channel_alloc(&ppi_ch_tim1_gpiote_rising);
  ASSERT(err_code == NRF_SUCCESS);

  err_code = nrf_drv_ppi_channel_alloc(&ppi_ch_tim1_gpiote_falling);
  ASSERT(err_code == NRF_SUCCESS);

  // Connecting  CC[0] CC[1] events to GPIOTE task
  // It's need for make a pulse with DUTY_ON
  uint32_t event_CC0_Addr =  nrf_drv_timer_event_address_get(&cycCtrlTmr, NRF_TIMER_EVENT_COMPARE0);
  uint32_t event_CC1_Addr =  nrf_drv_timer_event_address_get(&cycCtrlTmr, NRF_TIMER_EVENT_COMPARE1);
  uint32_t taskAddr =   nrf_drv_gpiote_out_task_addr_get(PUMP_HV_PIN);

  err_code = nrf_drv_ppi_channel_assign (ppi_ch_tim1_gpiote_rising, event_CC0_Addr, taskAddr);
  ASSERT(err_code == NRF_SUCCESS);

  err_code = nrf_drv_ppi_channel_assign (ppi_ch_tim1_gpiote_falling, event_CC1_Addr, taskAddr);
  ASSERT(err_code == NRF_SUCCESS);

}



void HV_pump_Startup()
{
  ret_code_t err_code = app_timer_start(mainHVtmr, MAIN_HW_TIMER_INTERVAL, NULL);
  ASSERT(err_code == NRF_SUCCESS);

}