#include <stdint.h>
#include <stdbool.h>
#include "nrf_error.h"
#include "nrf_assert.h"
#include "app_timer.h"
#include "app_time_lib.h"

#define NRF_LOG_MODULE_NAME     RTC
#define NRF_LOG_LEVEL           3
#include "nrf_log.h"

//#define MAX_RTC_COUNTER_VAL     0x00FFFFFF          // maximum value of the RTC counter

typedef struct
{
  uint32_t time;        // system time in units 0x00FFFFFF/32758 = 511 seconds
  uint64_t offset;      // difference time in seconds between user and system time
  uint32_t correction;  // correction in ticks after reset
  bool     enabled;
} app_time_t;

static  app_time_t app_time_s; 

APP_TIMER_DEF(ovfl_tmr);

//------------------------------------------------------------------------------
//        PRIVATE FUNCTIONS
//------------------------------------------------------------------------------
static void on_ovfl_tmr_evt(void* context)
{
  (void)context;
  app_time_s.time++;
}


//-----------------------------------------------------------------------------
//      PUBLIC FUNCTIONS
//------------------------------------------------------------------------------
void app_time_Init(void)
{
  memset(&app_time_s, 0, sizeof(app_time_t));
  ret_code_t err = app_timer_create(&ovfl_tmr, APP_TIMER_MODE_REPEATED, on_ovfl_tmr_evt);
  ASSERT(err == NRF_SUCCESS);
}

//------------------------------------------------------------------------------
void app_time_Startup(void)
{
  ret_code_t err = app_timer_start(ovfl_tmr, MAX_RTC_COUNTER_VAL, NULL);
  ASSERT(err == NRF_SUCCESS);
}

//------------------------------------------------------------------------------
bool app_time_Is_UTC_en(void)
{
  return app_time_s.enabled;
}

//------------------------------------------------------------------------------
bool app_time_Set_UTC(uint32_t seconds, uint32_t ticks)
{
  if (seconds)
  {
    app_time_s.offset = APP_TIMER_TICKS(seconds) - app_time_Get_internal_time();
    app_time_s.enabled = true;
  }
  return app_time_s.enabled;
}

//------------------------------------------------------------------------------
uint32_t app_time_Get_internal_time(void)
{
  return (uint32_t)(app_time_Get_internal_ticks() >> 15);
}

//------------------------------------------------------------------------------
uint64_t RTC_GetUsrTimeInTicks(void)
{
  return app_time_s.enabled ? (RTC_GetSysTimeInTicks() + ((uint64_t)app_time_s.offset << 15)) : 0;
}

//------------------------------------------------------------------------------
uint32_t RTC_GetSysTimeInSeconds(void)
{
  return (uint32_t)(RTC_GetSysTimeInTicks() >> 15);
}

//------------------------------------------------------------------------------
uint32_t RTC_GetSysTimeInMilliSeconds(void)
{
  return (uint32_t)((RTC_GetSysTimeInTicks() * 1000) >> 15);
}

//------------------------------------------------------------------------------
uint64_t app_time_Get_internal_ticks(void)
{
  uint32_t time1 = app_time_s.time;
  uint32_t tick = app_timer_cnt_get();
  uint32_t time2 = app_time_s.time;

  if (time1 != time2)
  {
    tick = app_timer_cnt_get();
  }

  return ((uint64_t)time2 << 24) + (uint64_t)tick + (uint64_t)app_time_s.correction;
}

//------------------------------------------------------------------------------
uint32_t RTC_GetSeconds(uint64_t ticks)
{
  return (uint32_t)(ticks >> 15);
}

//------------------------------------------------------------------------------
uint8_t RTC_GetFraction(uint64_t ticks)
{
  return (uint8_t)((ticks >> 7));
}

//------------------------------------------------------------------------------
uint32_t RTC_GetTickms(uint32_t ms)
{
  uint32_t temp;
  temp = MSECS_TO_TICKS((uint64_t)ms);
  CHK_ASSERT(temp < UINT32_MAX);
  return temp;
}

//------------------------------------------------------------------------------
uint32_t RTC_GetmsTick(uint32_t tick)
{
  uint32_t temp;

  temp = TICKS_TO_MSECS((uint64_t)tick);
  CHK_ASSERT(temp < UINT32_MAX);
  return temp;
}

//------------------------------------------------------------------------------
uint32_t RTC_GetDiffTick(uint32_t refTick)
{
  return app_timer_cnt_diff_compute(RTC_GetTick(), refTick);
}

//------------------------------------------------------------------------------
uint32_t RTC_CalcTickDiff(uint32_t t1, uint32_t t2)
{
  return app_timer_cnt_diff_compute(t1, t2);
}



