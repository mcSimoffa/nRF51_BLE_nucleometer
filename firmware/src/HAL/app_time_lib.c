#include <sdk_common.h>
#include "nrf_error.h"
#include "nrf_assert.h"
#include "app_timer.h"
#include "nrf_drv_clock.h"
#include "app_time_lib.h"

#define NRF_LOG_MODULE_NAME     RTC
#define NRF_LOG_LEVEL           3
#include "nrf_log.h"

#define MAX_RTC_COUNTER_VAL       0x00FFFFFF  // (inherit from app_timer.c) maximum value of the RTC counter
#define HALF_RTC_COUNTER_VAL      (MAX_RTC_COUNTER_VAL / 2)
#define APP_TIMER_OP_QUEUE_SIZE         6     // Size of timer operation queues.

typedef struct
{
  uint64_t time;        // system time in ticks (1/32768 sec)
  uint64_t offset;      // difference between system time and user (UTC) time
  bool     enabled;     // is user time enabled ?
} app_time_t;

static  app_time_t app_time_s; 

APP_TIMER_DEF(ovfl_tmr);

#if (APP_TIMER_PRESCALER != 0)
#error "APP_TIMER_PRESCALER must be zero to correct work app_time_lib"
#endif
//------------------------------------------------------------------------------
//        PRIVATE FUNCTIONS
//------------------------------------------------------------------------------
static void refresh_64bit_value()
{
  static uint64_t ovflRTC = 0;
  static uint32_t prevRTC = 0;
  uint32_t now = app_timer_cnt_get();
  if (now < prevRTC)
  {
    prevRTC = now;
    ovflRTC++;
  }
  app_time_s.time = (ovflRTC << 32) + now;
}

//-----------------------------------------------------------------------------
static void onHalfRangeTimerEvt(void* context)
{
  (void)context;
  refresh_64bit_value();
}

/*! ---------------------------------------------------------------------------
  \brief Function for starting lfclk needed by APP_TIMER.
---------------------------------------------------------------------------  */
static void lfclk_init(void)
{
  ret_code_t err_code = nrf_drv_clock_init();
  ASSERT(err_code == NRF_SUCCESS);

  nrf_drv_clock_lfclk_request(NULL);
}


//-----------------------------------------------------------------------------
//      PUBLIC FUNCTIONS
//------------------------------------------------------------------------------
void app_time_Init(void)
{
#ifdef DISABLE_SOFTDEVICE
  lfclk_init();
#endif
  // Initialize timer module
  APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, NULL);
  memset(&app_time_s, 0, sizeof(app_time_t));

  //create timer to watch overflow RTC scale 
  ret_code_t err = app_timer_create(&ovfl_tmr, APP_TIMER_MODE_REPEATED, onHalfRangeTimerEvt);
  ASSERT(err == NRF_SUCCESS);
}

//------------------------------------------------------------------------------
void app_time_Startup(void)
{
  ret_code_t err = app_timer_start(ovfl_tmr, HALF_RTC_COUNTER_VAL, NULL);
  ASSERT(err == NRF_SUCCESS);
}

//------------------------------------------------------------------------------
bool app_time_Is_UTC_en(void)
{
  return app_time_s.enabled;
}

//------------------------------------------------------------------------------
bool app_time_Set_UTC(uint64_t now_ticks)
{
  if (now_ticks)
  {
    app_time_s.offset = now_ticks - app_time_s.time;
    app_time_s.enabled = true;
  }
  return app_time_s.enabled;
}

//------------------------------------------------------------------------------
uint64_t app_time_Get_sys_time(void)
{
  refresh_64bit_value();
  return app_time_s.time;
}

//------------------------------------------------------------------------------
uint64_t app_time_Get_UTC(void)
{
  return app_time_Get_sys_time() + app_time_s.offset;
}

//-----------------------------------------------------------------------------
void app_time_Ticks_to_struct(uint64_t ticks, timestr_t *timestr)
{
  timestr->sec = ticks >> 15;
  timestr->frac = (uint32_t)(ticks & 0x7FFF);
  timestr->ms = timestr->frac * 1000 / APP_TIMER_CLOCK_FREQ;
}



