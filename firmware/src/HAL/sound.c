#include <sdk_common.h>
#include "nrf_log_ctrl.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_common.h"
#include "nrf_uart.h"
#include "app_time_lib.h"

#define NRF_LOG_MODULE_NAME "Sound"
#include "nrf_log.h"

#define SOUND_MASK       0x55

NRF_UART_Type * p_uart = NRF_UART0;

// ----------------------------------------------------------------------------
void UART0_IRQHandler(void)
{
  if (nrf_uart_event_check(p_uart, NRF_UART_EVENT_TXDRDY))
  {
    nrf_uart_event_clear(p_uart, NRF_UART_EVENT_TXDRDY);
    nrf_uart_txd_set(p_uart, SOUND_MASK);
  }
}


// ----------------------------------------------------------------------------
//    PUBLIC FUNCTION
// ----------------------------------------------------------------------------
void sound_Init(void)
{
  nrf_gpio_pin_set(BUZZER_PIN);
  nrf_gpio_cfg_output(BUZZER_PIN);
  nrf_uart_baudrate_set(p_uart, 0x0020000);
  p_uart->CONFIG = NRF_UART_PARITY_EXCLUDED | NRF_UART_HWFC_DISABLED;
  p_uart->PSELRXD = NRF_UART_PSEL_DISCONNECTED;
  p_uart->PSELTXD = BUZZER_PIN;
  p_uart->PSELCTS = NRF_UART_PSEL_DISCONNECTED;
  p_uart->PSELRTS = NRF_UART_PSEL_DISCONNECTED;
  nrf_uart_event_clear(p_uart, NRF_UART_EVENT_TXDRDY);
  nrf_uart_event_clear(p_uart, NRF_UART_EVENT_RXTO);
  nrf_uart_int_enable(p_uart, NRF_UART_INT_MASK_TXDRDY);
  nrf_drv_common_irq_enable(nrf_drv_get_IRQn((void *)p_uart), UART_DEFAULT_CONFIG_IRQ_PRIORITY);
}


void sound_Start(void)
{
  nrf_uart_enable(p_uart);
  nrf_uart_event_clear(p_uart, NRF_UART_EVENT_TXDRDY);
  nrf_uart_task_trigger(p_uart, NRF_UART_TASK_STARTTX);
  nrf_uart_event_clear(p_uart, NRF_UART_EVENT_TXDRDY);
  nrf_uart_txd_set(p_uart, SOUND_MASK);
}