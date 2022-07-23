#include <sdk_common.h>
#include "app_error.h"
#include "nrf_log_ctrl.h"

#include "app_time_lib.h"
#include "common_part.h"
#include "CPU_usage.h"
#include "HighVoltagePump.h"
#include "particle_cnt.h"
#include "ble_main.h"


#define NRF_LOG_MODULE_NAME "APP"
#include "nrf_log.h"

#define IS_SRVC_CHANGED_CHARACT_PRESENT 0                                           /**< Include or not the service_changed characteristic. if not enabled, the server's database cannot be changed for the lifetime of the device*/

#if (NRF_SD_BLE_API_VERSION == 3)
#define NRF_BLE_MAX_MTU_SIZE            GATT_MTU_SIZE_DEFAULT                       /**< MTU size used in the softdevice enabling and to reply to a BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST event. */
#endif


#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

typedef enum
{
    BLE_NO_ADV,                                                                     /**< No advertising running. */
    BLE_DIRECTED_ADV,                                                               /**< Direct advertising to the latest central. */
    BLE_FAST_ADV_WHITELIST,                                                         /**< Advertising with whitelist. */
    BLE_FAST_ADV,                                                                   /**< Fast advertising running. */
    BLE_SLOW_ADV,                                                                   /**< Slow advertising running. */
    BLE_SLEEP,                                                                      /**< Go to system-off. */
} ble_advertising_mode_t;




// Persistent storage system event handler.
void pstorage_sys_event_handler(uint32_t sys_evt);

//#define USE_AUTHORIZATION_CODE 1

#if USE_AUTHORIZATION_CODE
static uint8_t m_auth_code[] = {'A', 'B', 'C', 'D'}; //0x41, 0x42, 0x43, 0x44
static int m_auth_code_len = sizeof(m_auth_code);
#endif

uint16_t m_last_connected_central;

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





/**@brief Function for handling the Security Request timer timeout.
 *
 * @details This function will be called each time the Security Request timer expires.
 *
 * @param[in]   p_context   Pointer used for passing some arbitrary information (context) from the
 *                          app_start_timer() call to the timeout handler.
 */
/*static void sec_req_timeout_handler(void * p_context)
{
    uint32_t                err_code;
    uint16_t                conn_handle;
    pm_conn_sec_status_t    status;

    if (m_peer_id != PM_PEER_ID_INVALID)
    {
        err_code = pm_conn_handle_get(m_peer_id, &conn_handle);
        APP_ERROR_CHECK(err_code);

        err_code = pm_conn_sec_status_get(conn_handle, &status);
        APP_ERROR_CHECK(err_code);

        // If the link is still not secured by the peer, initiate security procedure.
        if (!status.encrypted)
        {
            err_code = pm_conn_secure(conn_handle, false);
            APP_ERROR_CHECK(err_code);
        }
    }
}*/






/**@brief Function for the Timer initialization.
 *
* @details Initializes the timer module. This creates and starts application timers.
*/
static void timers_init(void)
{
    uint32_t err_code;

    APP_ERROR_CHECK(err_code);

    // Create Security Request timer.
    /*err_code = app_timer_create(&m_sec_req_timer_id,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                sec_req_timeout_handler);*/
    APP_ERROR_CHECK(err_code);
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

/*!  ---------------------------------------------------------------------------
  \brief Function for application main entry.``
 ---------------------------------------------------------------------------  */
int main(void)
{
  common_drv_init();
  app_time_Init();

  ret_code_t err_code = NRF_LOG_INIT(log_time_provider);
  ASSERT(err_code == NRF_SUCCESS);
  
  BLE_Init();

  CPU_usage_Startup();

  HV_pump_Init();
  particle_cnt_Init();
 
  //particle_cnt_Startup();
  //HV_pump_Startup();


  // Enter main loop.
  while (true)
  {
    if (NRF_LOG_PROCESS() == false)
    {
      CPU_usage_Sleep();
    }
  }
}
