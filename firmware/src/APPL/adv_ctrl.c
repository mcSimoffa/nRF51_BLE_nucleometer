#include <sdk_common.h>
#include <nrf_assert.h>
#include "app_time_lib.h"
#include "esm_lib.h"
#include "sys_alive.h"
#include "button.h"

#include "adv_ctrl.h"

#define NRF_LOG_MODULE_NAME "ADV_CTRL"
#define NRF_LOG_LEVEL        4
#include "nrf_log.h"

// handling fast 3x press button to advertising without whitelist
#define CLICK_PAUSE_DURATION           MS_TO_TICK(500)
#define AFTER_SEQUENCE_DEAD_INTERVAL   MS_TO_TICK(1500)

typedef enum
{
  STATE_IDLE,
  STATE_PRESSED1,
  STATE_RELEASED1,
  STATE_PRESSED2,
  STATE_RELEASED2,
} dbclick_state_t;

typedef enum
{
  SIGNAL_NO_ACTION = 0,
  SIGNAL_BUTTON_ACTION,
  SIGNAL_TIMER_EXPIRED,
} dbclick_signal_t;

typedef struct
{
  uint8_t isTimrEvt:1;
  uint8_t isPressedEvt:1;
  uint8_t isReleaseEvt:1;
} dbclick_ctx_t;

//------------------------------------------------------------------------------
//        PROTOTYPES
//------------------------------------------------------------------------------
static void button_cb(button_event_t event);


static ESM_ctx_t      dbclick_esm_ctx = {.logLevel = 4};
static dbclick_ctx_t  dbclick_ctx;

BUTTON_REGISTER_HANDLER(m_button_cb) = button_cb;
APP_TIMER_DEF(to_tmr);

//------------------------------------------------------------------------------
static void OnTimerEvent(void * p_context)
{
  dbclick_ctx.isTimrEvt = 1;
  sleepLock();
}

//------------------------------------------------------------------------------
static void button_cb(button_event_t event) 
{
  if (event.field.button_num == 0)
  {
    if (event.field.pressed == 1)
      dbclick_ctx.isPressedEvt = 1;
    else
      dbclick_ctx.isReleaseEvt = 1;

    sleepLock();
  }
}

//------------------------------------------------------------------------------
static uint16_t  idleProc(void * ctx)
{
  if(dbclick_ctx.isPressedEvt)
  {
    dbclick_ctx.isPressedEvt = 0;
    dbclick_ctx.isReleaseEvt = 0;
    return SIGNAL_BUTTON_ACTION;
  }
  return SIGNAL_NO_ACTION;
}

//------------------------------------------------------------------------------
static uint16_t  waitPressed(void * ctx)
{
  if(dbclick_ctx.isTimrEvt)
  {
    dbclick_ctx.isTimrEvt = 0;
    return SIGNAL_TIMER_EXPIRED;
  }

  if(dbclick_ctx.isPressedEvt)
  {
    dbclick_ctx.isPressedEvt = 0;
    return SIGNAL_BUTTON_ACTION;
  }
  return SIGNAL_NO_ACTION;
}

//------------------------------------------------------------------------------
static uint16_t  waitReleased(void * ctx)
{
  if(dbclick_ctx.isTimrEvt)
  {
    dbclick_ctx.isTimrEvt = 0;
    return SIGNAL_TIMER_EXPIRED;
  }

  if(dbclick_ctx.isReleaseEvt)
  {
    dbclick_ctx.isReleaseEvt = 0;
    return SIGNAL_BUTTON_ACTION;
  }
  return SIGNAL_NO_ACTION;
}

//------------------------------------------------------------------------------
static void timerRestart(uint32_t duration_tick)
{
  ret_code_t err_code = app_timer_stop(to_tmr);
  APP_ERROR_CHECK(err_code);
  dbclick_ctx.isTimrEvt = 0;
  
  err_code = app_timer_start(to_tmr, duration_tick, NULL);
  APP_ERROR_CHECK(err_code);
}

//-----------------------------------------------------------------------------
static void shortInterval(void *ctx)
{
  timerRestart(CLICK_PAUSE_DURATION);
}

//-----------------------------------------------------------------------------
static void longInterval(void *ctx)
{
  timerRestart(AFTER_SEQUENCE_DEAD_INTERVAL);
}

//-----------------------------------------------------------------------------
static void singleClickAction(void *ctx)
{
  NRF_LOG_INFO("ADV START WITH WHITELIST\n");

}

//-----------------------------------------------------------------------------
static void dbclickAction(void *ctx)
{
  NRF_LOG_INFO("ADV START WITHOUT WHITELIST\n");
}

//------------------------------------------------------------------------------
static const ESM_t dbclick_esm =
{
  ESM_DEF(5, "WHLdis")
  {
    {
      ESM_STATE_DEF(STATE_IDLE, "IDLE", idleProc, 2)
          { {SIGNAL_BUTTON_ACTION, STATE_PRESSED1, shortInterval},
            {SIGNAL_TIMER_EXPIRED, STATE_IDLE,  NULL } }
    },
    {
      ESM_STATE_DEF(STATE_PRESSED1, "PRESSED1", waitReleased, 2)
          { {SIGNAL_BUTTON_ACTION,  STATE_RELEASED1, shortInterval},
            {SIGNAL_TIMER_EXPIRED,  STATE_IDLE,  NULL } }
    },
    {
      ESM_STATE_DEF(STATE_RELEASED1, "RELEASED1", waitPressed, 2)
          { {SIGNAL_BUTTON_ACTION,  STATE_PRESSED2, shortInterval},
            {SIGNAL_TIMER_EXPIRED,  STATE_IDLE,  singleClickAction } }
    },
    {
      ESM_STATE_DEF(STATE_PRESSED2, "PRESSED2", waitReleased, 2)
          { {SIGNAL_BUTTON_ACTION,  STATE_RELEASED2, longInterval},
            {SIGNAL_TIMER_EXPIRED,  STATE_IDLE,  NULL } }
    },
    {
      ESM_STATE_DEF(STATE_RELEASED2, "RELEASED2", waitPressed, 2)
          { {SIGNAL_BUTTON_ACTION,  STATE_IDLE, NULL},
            {SIGNAL_TIMER_EXPIRED,  STATE_IDLE, dbclickAction } }
    },
  }
};



/*! ---------------------------------------------------------------------------
 \brief Function for initializing button double click processing.
 */
void dbclichHandler_Init(void)
{
  ret_code_t err_code = app_timer_create(&to_tmr, APP_TIMER_MODE_SINGLE_SHOT, OnTimerEvent);
  APP_ERROR_CHECK(err_code);

  ASSERT (esmGetState(&dbclick_esm_ctx) == STATE_IDLE);
  err_code = esmEnable(&dbclick_esm, true, &dbclick_esm_ctx);
  APP_ERROR_CHECK(err_code);
}

//------------------------------------------------------------------------------
void dbclick_Process(void)
{
  if (esmProcess(&dbclick_esm, &dbclick_esm_ctx) == true)
    sleepLock();
}