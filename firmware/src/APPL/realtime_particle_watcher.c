#include "particle_cnt.h"
#include "app_time_lib.h"
#include "sound.h"
#include "HighVoltagePump.h"

#define NRF_LOG_MODULE_NAME "RPW"
#define NRF_LOG_LEVEL       3
#include "nrf_log.h"

#define   ONE_SEC_TICK            MS_TO_TICK(1000)
#define   DOSE_DURATION           40    // 40 sec measurement according to sensor sensivity
#define   ALART_REPEAT_PERIOD     1800  // 30 min
#define   WARNING_THRESHOLD       85
#define   DANGER_THRESHOLD        170
#define   CRITICAL_DISCHARCE_CNT  15    //15 pulses per second usually discharge capacitor too low. New pump cycle is required


typedef enum
{
  NO_ALARM,
  WARNING_ALARM,
  DANGER_ALARM,
} alarm_level_t;


APP_TIMER_DEF(tmr);
static uint8_t  slide_array[DOSE_DURATION];
static uint8_t  pointer;
static uint32_t last_cnt;
static uint16_t realtime_summ;

// ----------------------------------------------------------------------------
static void alarmer(uint16_t cnt)
{
  static uint16_t alarm_repeat_counter = 0;
  static alarm_level_t level = NO_ALARM;
NRF_LOG_INFO("cnt=%d\n", cnt);
  if ((cnt > DANGER_THRESHOLD) && ((level < DANGER_ALARM) || (alarm_repeat_counter == 0)))
  {
    alarm_repeat_counter = ALART_REPEAT_PERIOD;
    level = DANGER_ALARM;
    sound_danger();
    NRF_LOG_INFO("Danger due to cnt=%d\n", cnt);
  }
  else if ((cnt > WARNING_THRESHOLD) && ((level < WARNING_ALARM) || (alarm_repeat_counter == 0)))
  {
    alarm_repeat_counter = ALART_REPEAT_PERIOD;
    level = WARNING_ALARM;
    sound_alarm();
    NRF_LOG_INFO("Alarm due to cnt=%d\n", cnt);
  }
  if (alarm_repeat_counter)
  {
    alarm_repeat_counter--;
  }
  else
  {
    level = NO_ALARM;
  }
}


// ----------------------------------------------------------------------------
static void OnTmr(void* context)
{
  uint32_t after = particle_cnt_Get();
  uint32_t diff = after - last_cnt;
  if (diff > CRITICAL_DISCHARCE_CNT)
  {
    HV_instantKick();
  }
  last_cnt = after;
  diff = (diff > UINT8_MAX) ? UINT8_MAX : diff;
  slide_array[pointer] = (uint8_t)diff;
  pointer = ++pointer % DOSE_DURATION;
  realtime_summ = 0;
  for (uint8_t i=0; i<DOSE_DURATION; i++)
  {
    realtime_summ += slide_array[i];
  }
  alarmer(realtime_summ);
}


// ----------------------------------------------------------------------------
void RPW_Init(void)
{
  ret_code_t ret_code = app_timer_create(&tmr, APP_TIMER_MODE_REPEATED, OnTmr);
  APP_ERROR_CHECK(ret_code);
}

// ----------------------------------------------------------------------------
void RPW_Startup(void)
{
  ret_code_t ret_code = app_timer_start(tmr, ONE_SEC_TICK, NULL);
  APP_ERROR_CHECK(ret_code);
}

// ----------------------------------------------------------------------------
uint16_t RPW_GetInstant(void)
{
  NRF_LOG_INFO("RPW=%d\n", realtime_summ);
  return realtime_summ;
}