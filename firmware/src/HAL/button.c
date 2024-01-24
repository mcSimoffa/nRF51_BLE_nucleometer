#include <sdk_common.h>
#include <nrf_drv_gpiote.h>
#include "app_time_lib.h"
#include "sys_alive.h"
#include "ringbuf.h"

#include "button.h"

#define QUEUE_SIZE                    4
#define BUTTONS_TOTAL                 1
#define UNUSED_PIN                    0xFF
#define SW_DETECTION_DELAY_MS         50
#define SW_DEBOUNCE_SUPPRESS_CLOCK_MS 10

#define NRF_LOG_MODULE_NAME     "BTN"
#define NRF_LOG_LEVEL           2
#include "nrf_log.h"

// Helper macros for section variables.
#define BUTTON_SECTION_VARS_GET(i) NRF_SECTION_VARS_GET((i), button_handler_t, button_handlers)

#define BUTTON_SECTION_VARS_COUNT   NRF_SECTION_VARS_COUNT(button_handler_t, button_handlers)

NRF_SECTION_VARS_CREATE_SECTION(button_handlers, button_handler_t);

//-----------------------------------------------------------------------------
// Custom types
//-----------------------------------------------------------------------------
typedef struct
{  
  int16_t b_debounce_timestamp;
  bool    is_debounce;
} ctx_t;

// ----------------------------------------------------------------------------
//    PRIVATE VARIABLE
// ----------------------------------------------------------------------------
static const uint8_t pin_array[BUTTONS_TOTAL]       = BUTTONS;
static const uint8_t pin_act_level[BUTTONS_TOTAL]   = BUTTONS_ACTIVE_LEVEL;
static const uint8_t pin_pull_type[BUTTONS_TOTAL]   = BUTTONS_PULL;

static ctx_t b_ctx[BUTTONS_TOTAL];
static bool  is_timer_active;

APP_TIMER_DEF(debounce_tmr);
RING_BUF_DEF(btn_rb, QUEUE_SIZE);

// ----------------------------------------------------------------------------
//    PRIVATE FUNCTION
// ----------------------------------------------------------------------------
static void sense_pin_cb(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  bool is_need_to_takting = false;

  NRF_LOG_DEBUG("%s: pin %d action %d\n", (uint32_t)__func__, pin, action);
  for (uint8_t i = 0; i < BUTTONS_TOTAL; i++)
  {
    if (pin_array[i] == pin)
    {
      b_ctx[i].is_debounce = true;
      is_need_to_takting = true;
      timestr_t timestr;
      app_time_Ticks_to_struct(app_time_Get_sys_time(), &timestr);
      b_ctx[i].b_debounce_timestamp = timestr.ms;
      break;
    }
  }

  // enable timer if at least one switcher has not stable state
  if (is_need_to_takting && (is_timer_active == false))
  {
    ret_code_t error = app_timer_start(debounce_tmr, MS_TO_TICK(SW_DEBOUNCE_SUPPRESS_CLOCK_MS), NULL);
    ASSERT(error == NRF_SUCCESS);
    is_timer_active = true;
    NRF_LOG_DEBUG("Debounce timer start\n");
  }
}


/*!----------------------------------------------------------------------------
  \brief get pin state according to pin active state
  \param i[in]  - button number in button table
  \return 1 if button pressed (in actibe state).
  \return 0 if button released
*/
static uint8_t getPinState(uint8_t n_btn)
{
  bool phys = nrf_drv_gpiote_in_is_set(pin_array[n_btn]);
  bool soft = phys ^ !(bool)(pin_act_level[n_btn]);
  return (uint8_t)soft;
}

//-----------------------------------------------------------------------------
static void event_to_queue(button_event_t event)
{
  if (ringbufPut(&btn_rb, (uint8_t*)&event, 1) == 1)
  {
    NRF_LOG_DEBUG("%s: Event[btn# %d, state %d]\n", (uint32_t)__func__, event.field.button_num, event.field.pressed);
    sleepLock();
  }
  else
  {
    NRF_LOG_WARNING("%s: NOT stored event[btn# %d, state %d]\n", (uint32_t)__func__, event.field.button_num, event.field.pressed);
  }
}

//-----------------------------------------------------------------------------
static void process(void)
{
  for (uint8_t i = 0; i < BUTTONS_TOTAL; i++)
  {
    if (b_ctx[i].is_debounce)
    {
      timestr_t timestr;
      uint64_t ticks = app_time_Get_sys_time();
      app_time_Ticks_to_struct(ticks, &timestr);
      NRF_LOG_DEBUG("Time ticks=%d sec=%d frac=%d ms=%d\n", ticks, timestr.sec, timestr.frac, timestr.ms);
      int16_t deb_time = timestr.ms - b_ctx[i].b_debounce_timestamp;
      deb_time = deb_time >= 0 ? deb_time : deb_time + 1000;
      if (deb_time >= SW_DETECTION_DELAY_MS)
      {  
        b_ctx[i].is_debounce = false;
        uint8_t b_state = getPinState(i);
        button_event_t evt =
        {
          .field.pressed = b_state,
          .field.button_num = i,
        };
        event_to_queue(evt);
      }
    }
  }
}

/*! ---------------------------------------------------------------------------
  \brief  keeps work debounce suppress timer until all buttons are not handled
*/
static void OnTimerEvent(void * p_context)
{
  process();
  for(uint8_t i = 0; i < BUTTONS_TOTAL; i++)
  {
    if (b_ctx[i].is_debounce)
      // keep takt timer only if even one switch has not stable state (until debounce time expired)
      return;
  }

  ret_code_t error = app_timer_stop(debounce_tmr);
  ASSERT(error == NRF_SUCCESS);
  is_timer_active = false;
}

//-----------------------------------------------------------------------------
static void in_event_init(void)
{  
  ret_code_t err_code;
  nrf_drv_gpiote_in_config_t pin_cfg = GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);  

  for(uint8_t i = 0; i < BUTTONS_TOTAL; i++)
  {
    pin_cfg.pull = pin_pull_type[i];
    err_code = nrf_drv_gpiote_in_init(pin_array[i], &pin_cfg, sense_pin_cb);
    ASSERT(err_code == NRF_SUCCESS);
    nrf_drv_gpiote_in_event_enable(pin_array[i], true);
  }
}




// ----------------------------------------------------------------------------
//    PUBLIC FUNCTION
// ----------------------------------------------------------------------------
void button_Init(void)
{
  ret_code_t error;

  in_event_init();
  error = app_timer_create(&debounce_tmr, APP_TIMER_MODE_REPEATED, OnTimerEvent);
  ASSERT(error == NRF_SUCCESS);

  ringbufInit(&btn_rb);
}


//-----------------------------------------------------------------------------
void button_Startup(void)
{
  for(uint8_t i = 0; i < BUTTONS_TOTAL; i++)
  {
    if(getPinState(i) == 1)
    {
      button_event_t evt =
      {
        .field.pressed = 1,
        .field.button_num = i,
      };
      event_to_queue(evt);
    }
  }
}

//-----------------------------------------------------------------------------
void button_Process(void)
{
  uint8_t         m_next_handler;
  button_event_t  evt;
  size_t  sz = sizeof( button_event_t);

  do
  {
    m_next_handler = 0;
    ret_code_t err_code = ringbufGet(&btn_rb, (uint8_t*)&evt, &sz);
    while ((sz == sizeof(button_event_t)) && (m_next_handler < BUTTON_SECTION_VARS_COUNT))
    {
      (*BUTTON_SECTION_VARS_GET(m_next_handler))(evt);
      m_next_handler++;
    }
  } while (sz == sizeof(button_event_t));
}



//-----------------------------------------------------------------------------
uint8_t button_IsPressed(uint8_t n_btn)
{
  return getPinState(n_btn);
}
