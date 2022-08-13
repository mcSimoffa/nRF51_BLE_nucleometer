#include <sdk_common.h>
#include <nrf_assert.h>
#include "softdevice_handler.h"
#include "app_time_lib.h"
#include "esm_lib.h"
#include "sys_alive.h"
#include "button.h"
#include "conn.h"
#include "adv.h"

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
} adv_ctrl_state_t;

typedef enum
{
  SIGNAL_NO_ACTION = 0,
  SIGNAL_BUTTON_ACTION,
  SIGNAL_TIMER_EXPIRED,
} adv_ctrl_signal_t;

typedef struct
{
  uint8_t isTimrEvt:1;
  uint8_t isPressedEvt:1;
  uint8_t isReleaseEvt:1;
  uint8_t adv_en:1;
  uint8_t fds_busy:1;
} adv_ctrl_ctx_t;

//------------------------------------------------------------------------------
//        PROTOTYPES
//------------------------------------------------------------------------------
static void button_cb(button_event_t event);

//------------------------------------------------------------------------------
//        PRIVATE VARIABLES
//------------------------------------------------------------------------------
static ESM_ctx_t      dbclick_esm_ctx = {.logLevel = 4};
static adv_ctrl_ctx_t  adv_ctrl_ctx;

BUTTON_REGISTER_HANDLER(m_button_cb) = button_cb;
APP_TIMER_DEF(to_tmr);

//------------------------------------------------------------------------------
//        PRIVATE FUNCTIONS
//------------------------------------------------------------------------------
static void OnTimerEvent(void * p_context)
{
  adv_ctrl_ctx.isTimrEvt = 1;
  sleepLock();
}

//------------------------------------------------------------------------------
static void button_cb(button_event_t event) 
{
  if (event.field.button_num == 0)
  {
    if (event.field.pressed == 1)
    {
      adv_ctrl_ctx.isPressedEvt = 1;
    }
    else
    {
      adv_ctrl_ctx.isReleaseEvt = 1;
    }

    sleepLock();
  }
}

//------------------------------------------------------------------------------
static uint16_t  idleProc(void * ctx)
{
  if(adv_ctrl_ctx.isPressedEvt)
  {
    adv_ctrl_ctx.isPressedEvt = 0;
    adv_ctrl_ctx.isReleaseEvt = 0;
    return SIGNAL_BUTTON_ACTION;
  }
  return SIGNAL_NO_ACTION;
}

//------------------------------------------------------------------------------
static uint16_t  waitPressed(void * ctx)
{
  if(adv_ctrl_ctx.isTimrEvt)
  {
    adv_ctrl_ctx.isTimrEvt = 0;
    return SIGNAL_TIMER_EXPIRED;
  }

  if(adv_ctrl_ctx.isPressedEvt)
  {
    adv_ctrl_ctx.isPressedEvt = 0;
    return SIGNAL_BUTTON_ACTION;
  }
  return SIGNAL_NO_ACTION;
}

//------------------------------------------------------------------------------
static uint16_t  waitReleased(void * ctx)
{
  if(adv_ctrl_ctx.isTimrEvt)
  {
    adv_ctrl_ctx.isTimrEvt = 0;
    return SIGNAL_TIMER_EXPIRED;
  }

  if(adv_ctrl_ctx.isReleaseEvt)
  {
    adv_ctrl_ctx.isReleaseEvt = 0;
    return SIGNAL_BUTTON_ACTION;
  }
  return SIGNAL_NO_ACTION;
}

//------------------------------------------------------------------------------
static void timerRestart(uint32_t duration_tick)
{
  ret_code_t err_code = app_timer_stop(to_tmr);
  APP_ERROR_CHECK(err_code);
  adv_ctrl_ctx.isTimrEvt = 0;
  
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
  NRF_LOG_INFO("set adv mode %d whitelist %d\n", BLE_ADV_MODE_FAST, true);
  advertising_mode_set(BLE_ADV_MODE_FAST, true);
  adv_ctrl_ctx.adv_en = 1;
}

//-----------------------------------------------------------------------------
static void dbclickAction(void *ctx)
{
  NRF_LOG_INFO("set adv mode %d whitelist %d\n", BLE_ADV_MODE_FAST, false);
  advertising_mode_set(BLE_ADV_MODE_FAST, false);
  adv_ctrl_ctx.adv_en = 1;
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



//------------------------------------------------------------------------------
//        EXTERN FUNCTIONS
//------------------------------------------------------------------------------
void adv_ctrl_Init(void)
{
  ret_code_t err_code = app_timer_create(&to_tmr, APP_TIMER_MODE_SINGLE_SHOT, OnTimerEvent);
  APP_ERROR_CHECK(err_code);

  ASSERT (esmGetState(&dbclick_esm_ctx) == STATE_IDLE);
  err_code = esmEnable(&dbclick_esm, true, &dbclick_esm_ctx);
  APP_ERROR_CHECK(err_code);
}

//------------------------------------------------------------------------------
void adv_ctrl_Process(void)
{
  if (esmProcess(&dbclick_esm, &dbclick_esm_ctx) == true)
  {
    sleepLock();
  }

  if ((adv_ctrl_ctx.adv_en == 1) && (adv_ctrl_ctx.fds_busy == 0))
  {
    adv_ctrl_ctx.adv_en = 0;
    if (BLE_conn_handle_get() == BLE_CONN_HANDLE_INVALID)
    {
      advertising_stop();
      NRF_LOG_INFO("ADV START\n");
      if (advertising_start() == NRF_ERROR_BUSY)
      {
        adv_ctrl_ctx.fds_busy = 1;
      }
    }
    else
    {
      NRF_LOG_WARNING("No ADV during connection active");
    }
  }
}

//------------------------------------------------------------------------------
void ble_advertising_on_sys_evt(uint32_t sys_evt)
{
  uint32_t ret;

  switch (sys_evt)
  {
    //When a flash operation finishes, clear certain flag.
    case NRF_EVT_FLASH_OPERATION_SUCCESS: //FALLTHROUGH
    case NRF_EVT_FLASH_OPERATION_ERROR:
      adv_ctrl_ctx.fds_busy = 0;
      sleepLock();
      break;

    default:
      break;
  }
}