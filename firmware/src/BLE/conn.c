#include <sdk_common.h>
#include <nrf_assert.h>
#include <app_error.h>
#include <app_time_lib.h>
#include <ble_gap.h>
#include <ble.h>
#include <ble_hci.h>
#include "ble_conn_params.h"

#define DEVICE_NAME         "NucMe"   //Name of device. Will be included in the advertising data.
#define MANUFACTURER_NAME   "maximo"  //Manufacturer. Will be passed to Device Information Service.

#define SECOND_10_MS_UNITS              100   // Definition of 1 second, when 1 unit is 10 ms.

#define MIN_CONN_INTERVAL               7                           // Minimum acceptable connection interval (0.25 seconds), Connection interval uses 1.25 ms units.
#define MAX_CONN_INTERVAL               400                         // Maximum acceptable connection interval (0.5 second), Connection interval uses 1.25 ms units.
#define SLAVE_LATENCY                   0                           // Slave latency.
#define CONN_SUP_TIMEOUT                (4 * SECOND_10_MS_UNITS)    // Connection supervisory timeout (4 seconds), Supervision Timeout uses 10 ms units.

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(15000, APP_TIMER_PRESCALER) /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER)  /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;

// ----------------------------------------------------------------------------
void static_passkey_def(void) 
{
  static uint8_t static_passkey[] = STATIC_PASSKEY;
  ASSERT(sizeof(static_passkey) == (6 + 1));
  ret_code_t err_code;
  ble_opt_t opt;

  memset(&opt, 0x00, sizeof(opt));
  opt.gap_opt.passkey.p_passkey = static_passkey;
  err_code = sd_ble_opt_set(BLE_GAP_OPT_PASSKEY, &opt);
  APP_ERROR_CHECK(err_code);
}

// ----------------------------------------------------------------------------
/*! \brief Function for handling the Connection Parameter events.
 *
 * \details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail configuration parameter, but instead we use the
 *                event handler mechanism to demonstrate its use.
 *
 * \param[in]   p_evt   Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
  uint32_t err_code;

  if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
  {
    err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
    APP_ERROR_CHECK(err_code);
  }
}

// ----------------------------------------------------------------------------
/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

// ----------------------------------------------------------------------------
/*! \brief Function for the GAP initialization.
 *
 * \details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
void gap_params_init(void)
{
  uint32_t                err_code;
  ble_gap_conn_params_t   gap_conn_params;
  ble_gap_conn_sec_mode_t sec_mode;

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

  err_code = sd_ble_gap_device_name_set(&sec_mode,
                                        (const uint8_t *)DEVICE_NAME,
                                        strlen(DEVICE_NAME));
  ASSERT(err_code == NRF_SUCCESS);

  err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_CYCLING_POWER_SENSOR);
  ASSERT(err_code == NRF_SUCCESS);

  memset(&gap_conn_params, 0, sizeof(gap_conn_params));

  gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
  gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
  gap_conn_params.slave_latency     = SLAVE_LATENCY;
  gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

  err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
  ASSERT(err_code == NRF_SUCCESS);
}



/*! \brief Function for initializing the Connection Parameters module.
 */
void conn_params_init(void)
{
  uint32_t               err_code;
  ble_conn_params_init_t cp_init;

  memset(&cp_init, 0, sizeof(cp_init));

  cp_init.p_conn_params                  = NULL;
  cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
  cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
  cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
  cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
  cp_init.disconnect_on_fail             = false;
  cp_init.evt_handler                    = on_conn_params_evt;
  cp_init.error_handler                  = conn_params_error_handler;

  err_code = ble_conn_params_init(&cp_init);
  APP_ERROR_CHECK(err_code);
}

void BLE_conn_handle_set(uint16_t hnd)
{
  m_conn_handle = hnd;
}

uint16_t BLE_conn_handle_get(void)
{
  return m_conn_handle;
}