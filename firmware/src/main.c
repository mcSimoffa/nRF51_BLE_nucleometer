#include "sdk_common.h"
#include "app_error.h"
#include "nrf_log_ctrl.h"

#include "sys_alive.h"
#include "app_time_lib.h"
#include "common_part.h"
#include "CPU_usage.h"
#include "HighVoltagePump.h"
#include "particle_cnt.h"
#include "button.h"
#include "ble_main.h"
#include "batMea.h"

#define NRF_LOG_MODULE_NAME "APP"
#define NRF_LOG_LEVEL           4
#include "nrf_log.h"

#define DEAD_BEEF    0xDEADBEEF //Value used as error code on stack dump, can be used to identify stack location on stack unwind



void button_cb1(button_event_t event);

BUTTON_REGISTER_HANDLER(m_button_cb1) = button_cb1;

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in]   line_num   Line number of the failing ASSERT call.
 * @param[in]   file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/*! ---------------------------------------------------------------------------
  \brief Function provides a timestamp for log module
  \details app_time_Init() should be invoked before using

  \return milliseconds value after system start (RTC1 was started)
 ----------------------------------------------------------------------------*/
static uint32_t log_time_provider()
{
  timestr_t log_timestr;
  app_time_Ticks_to_struct(app_time_Get_sys_time(), &log_timestr);
  return (log_timestr.sec*1000 + log_timestr.ms);
}


void button_cb1(button_event_t event) 
{
  //NRF_LOG_INFO("EVENT %d\n", (uint8_t)event.num);
}


/*!  ---------------------------------------------------------------------------
  \brief Function for application main entry.
 ---------------------------------------------------------------------------  */
int main(void)
{

  common_drv_init();
  app_time_Init();

  ret_code_t err_code = NRF_LOG_INIT(log_time_provider);
  ASSERT(err_code == NRF_SUCCESS);
  button_Init();
  BLE_Init();

  CPU_usage_Startup();

  HV_pump_Init();
  particle_cnt_Init();
 
  button_Startup();
  particle_cnt_Startup();
  HV_pump_Startup();

  // Enter main loop.
  while (true)
  {
    batMea_Process();
    button_Process();
    BLE_Process();
    bool log_in_process = NRF_LOG_PROCESS();
    bool sleep_is_locked = sleepLock_check();
    if ((log_in_process == false) && (sleep_is_locked == false))
    {
      CPU_usage_Sleep();
    }
  }
}
