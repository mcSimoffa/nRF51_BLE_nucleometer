#include "sdk_common.h"
#include "app_error.h"
#include "nrf_log_ctrl.h"
#include "app_time_lib.h"
#include "particle_cnt.h"
#include "ringbuf.h"

#include "event_queue.h"

#define NRF_LOG_MODULE_NAME     "evqb"
#define NRF_LOG_LEVEL           3
#include "nrf_log.h"

// ----------------------------------------------------------------------------
#define PERIOD_1H           3600u
#define RB_SIZE_ELEM        1100


// ----------------------------------------------------------------------------
typedef struct
{
  uint8_t msb;
  uint8_t mid;
  uint8_t lsb;
} event_t;

// ----------------------------------------------------------------------------
APP_TIMER_DEF(tmr);
RING_BUF_DEF(m_rb, RB_SIZE_ELEM * sizeof(event_t));
static uint32_t cnt_prev;
static uint32_t diff_cnt;
static uint16_t events_cnt;
static uint64_t last_timeframe_timestamp;

// ----------------------------------------------------------------------------
static void refresh(bool finalize_period)
{
  CRITICAL_REGION_ENTER();
  uint32_t cnt_now = particle_cnt_Get();
  diff_cnt = cnt_now - cnt_prev;
  if (finalize_period)
  {
    cnt_prev = cnt_now;
    last_timeframe_timestamp = app_time_Get_UTC();
  }
  CRITICAL_REGION_EXIT();
}

// ----------------------------------------------------------------------------
static uint32_t  get_event(void)
{
  uint32_t e;
  size_t size = sizeof(event_t);
  ret_code_t err_code = ringbufGet(&m_rb, (uint8_t*)&e, &size);
  ASSERT((err_code == NRF_SUCCESS) && (size == sizeof(event_t)));
  events_cnt--;
  return e;
}

// ----------------------------------------------------------------------------
static void OnTmr(void* context)
{
  refresh(true);

  if (ringbufGetFree(&m_rb) < sizeof(event_t))
  {
  /*  Throw one event out for release place.
      This case is occured if Event Queue is completelly full
  */
    NRF_LOG_INFO("Get event rid\n");
    get_event();
  }

  if (ringbufPut(&m_rb, (uint8_t*)&diff_cnt, sizeof(event_t)) != sizeof(event_t))
  {
    ASSERT(false);
  }
  events_cnt++;
}

// ----------------------------------------------------------------------------
//    PUBLIC FUNCTION
// ----------------------------------------------------------------------------
void EVQ_Init(void)
{
  ret_code_t ret_code = app_timer_create(&tmr, APP_TIMER_MODE_REPEATED, OnTmr);
  APP_ERROR_CHECK(ret_code);
}

// ----------------------------------------------------------------------------
void EVQ_Startup(void)
{
  ret_code_t ret_code = app_timer_start(tmr, MS_TO_TICK(PERIOD_1H * 1000), NULL);
  APP_ERROR_CHECK(ret_code);
  ringbufInit(&m_rb);
  last_timeframe_timestamp = app_time_Get_UTC();
}

// ----------------------------------------------------------------------------
uint16_t EVQ_GetEventsAmount(void)
{
  return events_cnt;
}

// ----------------------------------------------------------------------------
uint64_t EVQ_GetCurrentEventTimestamp(void)
{
  return last_timeframe_timestamp;
}

// ----------------------------------------------------------------------------
uint32_t EVQ_GetEvt(void)
{
  if (events_cnt == 0)
  {
    NRF_LOG_INFO("Uncomplete event\n");
    refresh(false);

    if (last_timeframe_timestamp)
    {
      return diff_cnt | 0x80000000;
    }
    else
    {
      return 0x80000000;
    }
  }
  else
  {
    NRF_LOG_INFO("Complete event\n");
    return get_event() & 0x7FFFFFFF;
  }
}