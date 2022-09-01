#include "sdk_common.h"
#include "app_error.h"
#include "nrf_log_ctrl.h"
#include "app_time_lib.h"
#include "particle_cnt.h"
#include "sys_alive.h"
#include "sound.h"

#include "particle_watcher.h"

#define NRF_LOG_MODULE_NAME   "PWTCH"
#define NRF_LOG_LEVEL         4
#define NRF_LOG_DEBUG_COLOR   5
#include "nrf_log.h"

#define PERIOD_10S    10
#define PERIOD_40S    40    //the base timeframe
#define PERIOD_4M     240
#define PERIOD_20M    1200
#define PERIOD_2H     7200
#define PERIOD_8H    (8*3600)

#define TIMEFRAMES_TOTAL      5

typedef struct
{
  uint32_t  *accum;
  uint32_t  size;
  uint32_t  volume;
  uint8_t   shift_cnt;
} particle_time_frame_t;


#define CNT_FRAME_DEF(_size)                  \
{                                             \
  .accum = (uint32_t*)&(uint32_t[_size]){0},  \
  .size = _size,                              \
  .volume = 0,                                \
  .shift_cnt = 0,                             \
 }

// ----------------------------------------------------------------------------
//    PRIVATE VARIABLE
// ----------------------------------------------------------------------------
static particle_time_frame_t frames[TIMEFRAMES_TOTAL]=
{
  CNT_FRAME_DEF(PERIOD_40S  / PERIOD_10S),
  CNT_FRAME_DEF(PERIOD_4M   / PERIOD_40S),
  CNT_FRAME_DEF(PERIOD_20M  / PERIOD_4M),
  CNT_FRAME_DEF(PERIOD_2H   / PERIOD_20M),
  CNT_FRAME_DEF(PERIOD_8H   / PERIOD_2H),
};

static bool is_timer_evt;
static uint8_t active_tf = 0;
static uint16_t particle_danger = 42;   //42*4 = 168uRh/h ~10 times over
static uint16_t particle_alarm  = 15;   //14*4 = 60uRh/h  ~3.3 times over

APP_TIMER_DEF(tmr);

// ----------------------------------------------------------------------------
//    PRIVATE FUNCTION
// ----------------------------------------------------------------------------
static void frame_serv(uint8_t fr_num, uint32_t val)
{
  NRF_LOG_DEBUG("frame %d val %d\n", fr_num, val);
  if (fr_num < TIMEFRAMES_TOTAL)
  {
    particle_time_frame_t *fr = &frames[fr_num];

    fr->volume = 0;
    for (uint8_t i=0; i<fr->size - 1; i++)
    {
      fr->accum[i] = fr->accum[i+1];
      fr->volume +=  fr->accum[i];
    }
    fr->accum[fr->size - 1] = val;
    fr->volume += val;

    if (++fr->shift_cnt >= fr->size)
    {
      fr->shift_cnt = 0;
      frame_serv(fr_num + 1, fr->volume); // recursive call
    }
  }
}


// ----------------------------------------------------------------------------
static void OnTmr(void* context)
{
  (void)context;
  is_timer_evt = true;
  sleepLock();
}

static void alarm(uint16_t particle_cnt_10s)
{
    if (particle_cnt_10s > particle_danger)
    {
      sound_danger();
    }
    else if (particle_cnt_10s > particle_alarm)
    {
      sound_alarm();
    }
}

// ----------------------------------------------------------------------------
//    PUBLIC FUNCTION
// ----------------------------------------------------------------------------
void PWT_Init(void)
{
  ret_code_t ret_code = app_timer_create(&tmr, APP_TIMER_MODE_REPEATED, OnTmr);
  APP_ERROR_CHECK(ret_code);
}

// ----------------------------------------------------------------------------
void PWT_Startup(void)
{
  ret_code_t ret_code = app_timer_start(tmr, MS_TO_TICK(PERIOD_10S * 1000), NULL);
  APP_ERROR_CHECK(ret_code);
}

// ----------------------------------------------------------------------------
void PWT_Process(void)
{
  static uint32_t pr_old = 0;
  if (is_timer_evt)
  {
    is_timer_evt = false;
    uint32_t pr_new = particle_cnt_Get();
    uint16_t delta = pr_new - pr_old;
    pr_old = pr_new;
    alarm(delta);
    frame_serv(0, delta);
  }
}

// ----------------------------------------------------------------------------
uint32_t  PWT_GetBarVol()
{
  return frames[active_tf].volume;
}

// ----------------------------------------------------------------------------
void PWT_Set_active_tf(uint8_t tf)
{
  ASSERT(tf <TIMEFRAMES_TOTAL);
  active_tf = tf;
}