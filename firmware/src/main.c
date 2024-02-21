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
#include "particle_watcher.h"
#include "sound.h"
#include "hw_test.h"
#include "realtime_particle_watcher.h"


#define NRF_LOG_MODULE_NAME     app
#define NRF_LOG_LEVEL           4
#include "nrf_log.h"

#define DEAD_BEEF    0xDEADBEEF //Value used as error code on stack dump, can be used to identify stack location on stack unwind



static void button_cb1(button_event_t event);

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
static uint32_t log_time_provider() //TODO probably here should be not in millisecons
{
  return (uint32_t)app_time_Get_sys_time();
}


static void button_cb1(button_event_t event)
{
  //NRF_LOG_INFO("EVENT %d\n", (uint8_t)event.num);
}

static void chip_check(void)
{
  uint32_t deviceID[2] =
  {
    NRF_FICR->DEVICEID[0],
    NRF_FICR->DEVICEID[1]
  };
  while ((deviceID[0] != TARGET_DEVID_0) && (deviceID[1] != TARGET_DEVID_1));
  // preventing run a configuration to another chip
}

/*!  ---------------------------------------------------------------------------
  \brief Function for application main entry.
 ---------------------------------------------------------------------------  */
int main(void)
{
  chip_check();
  common_drv_init();
  app_time_Init();

  ret_code_t err_code = NRF_LOG_INIT(log_time_provider);
  ASSERT(err_code == NRF_SUCCESS);
#if defined(HW_TEST) && (HW_TEST == 1)
  hw_test_Run();
  return 1;
#endif

  button_Init();
  BLE_Init(button_IsPressed(BUTTON_PIN));
  sound_Init();

  CPU_usage_Startup();

  HV_pump_Init();
  particle_cnt_Init();
  PWT_Init();
  RPW_Init();

  sound_Startup();
  button_Startup();
  particle_cnt_Startup();
  HV_pump_Startup();
  PWT_Startup();
  RPW_Startup();

  sound_hello();


  // Enter main loop.
  while (true)
  {
    batMea_Process();
    button_Process();
    particle_cnt_Process();
    PWT_Process();
    BLE_Process();

    bool log_in_process = NRF_LOG_PROCESS();
    bool sleep_is_locked = sleepLock_check();
    if ((log_in_process == false) && (sleep_is_locked == false))
    {
      CPU_usage_Sleep();
    }
  }
}
