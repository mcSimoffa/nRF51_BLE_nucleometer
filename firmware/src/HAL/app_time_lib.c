#include <sdk_common.h>
#include "nrf_error.h"
#include "nrf_assert.h"
#include "app_timer.h"
#include "nrf_drv_clock.h"
#include "app_time_lib.h"

#define NRF_LOG_MODULE_NAME     "time_lib"
#define NRF_LOG_LEVEL           0
#include "nrf_log.h"

#define MAX_RTC_COUNTER_VAL       0x00FFFFFF  // (inherit from app_timer.c) maximum value of the RTC counter
#define HALF_RTC_COUNTER_VAL      (MAX_RTC_COUNTER_VAL / 2)
#define APP_TIMER_OP_QUEUE_SIZE   6     // Size of timer operation queues.

typedef struct
{
  union
  {
    struct
    {
      uint64_t  low:24;
      uint64_t  high:40;
    } field;
    uint64_t time;        // system time in ticks (1/32768 sec)
  } time;
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
  uint32_t now = app_timer_cnt_get();
  if (now <  (uint32_t)app_time_s.time.field.low)
  {
    app_time_s.time.field.high++;
  }
  app_time_s.time.field.low = now;
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
#if defined(DISABLE_SOFTDEVICE) && DISABLE_SOFTDEVICE
  lfclk_init();
#endif
  // Initialize timer module
  APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, NULL);
  memset(&app_time_s, 0, sizeof(app_time_t));

  //create timer to watch overflow RTC scale 
  ret_code_t err = app_timer_create(&ovfl_tmr, APP_TIMER_MODE_REPEATED, onHalfRangeTimerEvt);
  ASSERT(err == NRF_SUCCESS);

  err = app_timer_start(ovfl_tmr, HALF_RTC_COUNTER_VAL, NULL);
  ASSERT(err == NRF_SUCCESS);
}

//------------------------------------------------------------------------------
bool app_time_Is_UTC_en(void)
{
  return app_time_s.enabled;
}

//------------------------------------------------------------------------------
bool app_time_Set_UTC(uint64_t utc_sec) //TODO probably not corresponded with function declaration
{
  if (utc_sec)
  {
    app_time_s.offset = (utc_sec << 15) - app_time_s.time.time;
    app_time_s.enabled = true;
  }
  return app_time_s.enabled;
}

//------------------------------------------------------------------------------
uint64_t app_time_Get_sys_time(void)
{
  refresh_64bit_value();
  return app_time_s.time.time;
}

//------------------------------------------------------------------------------
uint64_t app_time_Get_UTC(void)
{
  uint64_t retval = 0;
  if (app_time_s.enabled)
  {
    retval = (app_time_Get_sys_time() + app_time_s.offset) >> 15;
  }
  return retval;  //TODO probably not corresponded with function declaration
}

//-----------------------------------------------------------------------------
void app_time_Ticks_to_struct(uint64_t ticks, timestr_t *timestr)
{
  timestr->sec = ticks >> 15;
  timestr->frac = (int32_t)(ticks & 0x7FFF);
  timestr->ms = timestr->frac * 1000 / APP_TIMER_CLOCK_FREQ;
}



