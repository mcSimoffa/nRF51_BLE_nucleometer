#include <sdk_common.h>
#include "nrf_log_ctrl.h"
#include "nrf_drv_timer.h"
#include "Timer_anomaly_fix.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_common.h"
#include "app_time_lib.h"

#include "sound.h"

#define NRF_LOG_MODULE_NAME "Sound"
#include "nrf_log.h"

#define BASE_FREQ_DIVIDER   NRF_TIMER_FREQ_500kHz
#define CLOCK_FREQ_16MHz    16000000


APP_TIMER_DEF(note_delay_tmr);
static const nrf_drv_timer_t tone_tmr = NRF_DRV_TIMER_INSTANCE(2);
static nrf_ppi_channel_t ppi_A, ppi_B;
static uint16_t   curr_note_play;
static uint16_t   notes_total;
static bool isPause;
static const sound_note_t *p_seq;

static const sound_note_t hello[] =
{
  {50, 1047},
  {50, 0},
  {50, 1109},
  {50, 0},
  {50, 1175},
};

static const sound_note_t alarm[] =
{
  {100, 1568},
  {100, 0},
  {100, 1568},
  {100, 0},
  {100, 1568},
  {100, 0},
  {300, 1318},
};

static const sound_note_t danger[] =
{
  {500,  1800},
  {1000, 2400},
  {500,  1800},
  {500,  1200},
  {2000, 0},
  {500,  1800},
  {1000, 2400},
  {500,  1800},
  {500,  1200},
};

// ----------------------------------------------------------------------------
//    PRIVATE FUNCTION
// ----------------------------------------------------------------------------
static void OnToneTmr(nrf_timer_event_t event_type, void * p_context)
{
}

// ---------------------------------------------------------------------------
static void frequency_set(uint16_t freq)
{
  uint16_t cc = (CLOCK_FREQ_16MHz >> BASE_FREQ_DIVIDER) / freq / 2;
  nrf_drv_timer_extended_compare(&tone_tmr, NRF_TIMER_CC_CHANNEL0, cc,NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);
}

// ----------------------------------------------------------------------------
static void note_play_start(uint16_t freq, uint32_t duration)
{
  if (freq > 0)
  {
    frequency_set(freq);
    timer_anomaly_fix(tone_tmr.p_reg, 1);
    nrf_drv_timer_enable(&tone_tmr);
  }
  else
  {
    isPause = true;
  }

  ret_code_t err_code = app_timer_start(note_delay_tmr, MS_TO_TICK(duration), NULL);
  ASSERT(err_code == NRF_SUCCESS);
}

// ----------------------------------------------------------------------------
static void OnNoteTmr(void* context)
{
  (void)context;

  if (isPause == false)
  {
    nrf_drv_timer_disable(&tone_tmr);
    timer_anomaly_fix(tone_tmr.p_reg, 0);
  }
  isPause = false;

  if (++curr_note_play < notes_total)
  {
    note_play_start(p_seq[curr_note_play].freq, p_seq[curr_note_play].duration);
  }
}


// ---------------------------------------------------------------------------
// cycle control timer init
static void toneTimer_init(void)
{
  // timer for cycle control.
  nrf_drv_timer_config_t timer_cfg =
  {
    .frequency          = BASE_FREQ_DIVIDER,
    .mode               = NRF_TIMER_MODE_TIMER,
    .bit_width          = NRF_TIMER_BIT_WIDTH_16,
    .interrupt_priority = TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
    .p_context          = NULL
  };
  ret_code_t err_code = nrf_drv_timer_init(&tone_tmr, &timer_cfg, OnToneTmr);
  ASSERT(err_code == NRF_SUCCESS);
}

// ---------------------------------------------------------------------------
//configure 2 GPIO like a bridge
static void sound_gpio_init(void)
{
  ret_code_t err_code;
  nrf_drv_gpiote_out_config_t confA = 
  {
    .init_state = NRF_GPIOTE_INITIAL_VALUE_LOW,
    .task_pin   = true,
    .action     = NRF_GPIOTE_POLARITY_TOGGLE,
  };

  err_code = nrf_drv_gpiote_out_init(BUZZER_PIN_A, &confA);
  ASSERT(err_code == NRF_SUCCESS);

  confA.init_state = NRF_GPIOTE_INITIAL_VALUE_HIGH;
  err_code = nrf_drv_gpiote_out_init(BUZZER_PIN_B, &confA);
  ASSERT(err_code == NRF_SUCCESS);
}

// ---------------------------------------------------------------------------
static void sound_ppi_init(void)
{ 
  ret_code_t err_code;

  err_code = nrf_drv_ppi_channel_alloc(&ppi_A);
  ASSERT(err_code == NRF_SUCCESS);

  err_code = nrf_drv_ppi_channel_alloc(&ppi_B);
  ASSERT(err_code == NRF_SUCCESS);
}

// ---------------------------------------------------------------------------
static void bind_gpio_to_timer(void)
{
  uint32_t event_CC0_Addr =  nrf_drv_timer_event_address_get(&tone_tmr, NRF_TIMER_EVENT_COMPARE0);
  uint32_t task_addr_A =   nrf_drv_gpiote_out_task_addr_get(BUZZER_PIN_A);
  uint32_t task_addr_B =   nrf_drv_gpiote_out_task_addr_get(BUZZER_PIN_B);

  ret_code_t err_code = nrf_drv_ppi_channel_assign (ppi_A, event_CC0_Addr, task_addr_A);
  ASSERT(err_code == NRF_SUCCESS);

  err_code = nrf_drv_ppi_channel_assign (ppi_B, event_CC0_Addr, task_addr_B);
  ASSERT(err_code == NRF_SUCCESS);
}

// ---------------------------------------------------------------------------
static void note_delay_timer_init(void)
{
  ret_code_t err_code = app_timer_create(&note_delay_tmr, APP_TIMER_MODE_SINGLE_SHOT, OnNoteTmr);
  ASSERT(err_code == NRF_SUCCESS);
}

// ----------------------------------------------------------------------------
static void play(const sound_note_t *sequence, uint16_t seq_long)
{
  ASSERT(seq_long > 0);
  p_seq = sequence;
  notes_total = seq_long;
  curr_note_play = 0;
  note_play_start(p_seq[0].freq, p_seq[0].duration);
}
// ----------------------------------------------------------------------------
//    PUBLIC FUNCTION
// ----------------------------------------------------------------------------
void sound_Init(void)
{
  note_delay_timer_init();
  toneTimer_init();
  sound_gpio_init();
  sound_ppi_init();
  bind_gpio_to_timer();
}

// ----------------------------------------------------------------------------
void sound_Startup(void)
{
  ret_code_t err_code = nrf_drv_ppi_channel_enable(ppi_A);
  ASSERT(err_code == NRF_SUCCESS);

  err_code = nrf_drv_ppi_channel_enable(ppi_B);
  ASSERT(err_code == NRF_SUCCESS);

  nrf_drv_gpiote_out_task_enable(BUZZER_PIN_A);
  nrf_drv_gpiote_out_task_enable(BUZZER_PIN_B);
}

// ----------------------------------------------------------------------------
void sound_hello(void)
{
  play(&hello[0], ARRAY_SIZE(hello));
}

// ----------------------------------------------------------------------------
void sound_alarm(void)
{
  play(&alarm[0], ARRAY_SIZE(alarm));
}

// ----------------------------------------------------------------------------
void sound_danger(void)
{
  play(&danger[0], ARRAY_SIZE(danger));
}