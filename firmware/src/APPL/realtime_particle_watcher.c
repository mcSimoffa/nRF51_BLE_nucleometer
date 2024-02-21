#include "particle_cnt.h"
#include "app_time_lib.h"


#define   ONE_SEC_TICK    MS_TO_TICK(1000)
#define   DOSE_DURATION   40    // 40 sec measurement according to sensor sensivity


APP_TIMER_DEF(tmr);
static uint8_t  slide_array[DOSE_DURATION];
static uint8_t  pointer;
static uint32_t last_cnt;

// ----------------------------------------------------------------------------
static void OnTmr(void* context)
{
  uint32_t after = particle_cnt_Get();
  uint32_t diff = after - last_cnt;
  last_cnt = after;
  diff = (diff > UINT8_MAX) ? UINT8_MAX : diff;
  slide_array[pointer] = (uint8_t)diff;
  pointer = ++pointer % DOSE_DURATION;
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
  uint16_t sum = 0;
  for (uint8_t i=0; i<DOSE_DURATION; i++)
  {
    sum += slide_array[i];
  }
  return sum;
}