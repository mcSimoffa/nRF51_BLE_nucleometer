#include <sdk_common.h>
#include "app_time_lib.h"
#include "nrf_log_ctrl.h"

#include "CPU_usage.h"

#define NRF_LOG_MODULE_NAME     "CPU_usage"
#define NRF_LOG_LEVEL           3
#define NRF_LOG_INFO_COLOR      5          
#include "nrf_log.h"

APP_TIMER_DEF(timer);
uint32_t  loops;
uint32_t  workTime;
uint32_t  sleepTime;
bool      onTimerEvt;

//------------------------------------------------------------------------------
static void OnTimerEvt(void* context)
{
  onTimerEvt = true;
}

//------------------------------------------------------------------------------
static void pwr_mgmt_run(void)
{
    // Wait for an event.
#if defined(SOFTDEVICE_PRESENT) && !defined(DISABLE_SOFTDEVICE)
    ret_code_t ret_code = sd_app_evt_wait();
    if (ret_code == NRF_ERROR_SOFTDEVICE_NOT_ENABLED)
    {
        __WFE();
        __SEV();
        __WFE();
    }
    else
    {
        APP_ERROR_CHECK(ret_code);
    }
#else
    __WFE();
    __SEV();
    __WFE();
#endif // SOFTDEVICE_PRESENT
}

//------------------------------------------------------------------------------
void CPU_usage_Startup(void)
{
#ifdef CPU_USAGE_MONITOR
  ret_code_t error = app_timer_create(&timer, APP_TIMER_MODE_REPEATED, OnTimerEvt);
  ASSERT(error == NRF_SUCCESS);
  onTimerEvt = false;
  error = app_timer_start(timer, MS_TO_TICK(CPU_USAGE_REPORT_PERIOD_MS), NULL);
  ASSERT(error == NRF_SUCCESS);
#endif

#if !defined (SOFTDEVICE_PRESENT) || defined(DISABLE_SOFTDEVICE)
    SCB->SCR |= SCB_SCR_SEVONPEND_Msk;
#endif
}

//------------------------------------------------------------------------------
void CPU_usage_Sleep(void)
{
#ifdef CPU_USAGE_MONITOR
  static uint32_t tickBeforeSleep;
  static uint32_t tickAfterSleep;
  tickBeforeSleep = app_timer_cnt_get();
  workTime += tickBeforeSleep - tickAfterSleep;
  pwr_mgmt_run();
  tickAfterSleep = app_timer_cnt_get();
  sleepTime += tickAfterSleep - tickBeforeSleep;
  loops++;
  if (onTimerEvt)
  {
     onTimerEvt = false;
     uint8_t duty =  (uint8_t)(100 * workTime / (workTime + sleepTime));
     NRF_LOG_INFO("cycles %d. Duty %d [Work %d. Sleep %d]\n", loops, duty, workTime, sleepTime);
     loops = workTime = sleepTime = 0;
  }
#else
  pwr_mgmt_run();
#endif
}

