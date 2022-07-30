#include <sdk_common.h>
#include <app_error.h>
#include "nrf_log_ctrl.h"
#include <app_time_lib.h>
#include <ble.h>
#include "ble_srv_common.h"
#include "softdevice_handler.h"
#include "ble_main.h"
#include "ble_advertising.h"
#include "ble_conn_state.h"
#include "ble_conn_params.h"
#include "fstorage.h"

#include "adv.h"
#include "adv_ctrl.h"
#include "conn.h"
#include "bond_mgmt_srv.h"
#include "pm.h"

#define NRF_LOG_MODULE_NAME "BLEmain"
#include "nrf_log.h"


#define NRF_CLOCK_LFCLKSRC                            \
{                                                     \
  .source = NRF_CLOCK_LF_SRC_XTAL,                    \
  .rc_ctiv       = 0,                                 \
  .rc_temp_ctiv  = 0,                                 \
  .xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM  \
}

#define CENTRAL_LINK_COUNT              0      //Number of central links used by the application. When changing this number remember to adjust the RAM settings
#define PERIPHERAL_LINK_COUNT           1      //Number of peripheral links used by the application. When changing this number remember to adjust the RAM settings


//------------------------------------------------------------------------------
//        PRIVATE FUNCTIONS
//------------------------------------------------------------------------------
/**@brief Function for handling the Application's BLE Stack events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
  ret_code_t err_code = NRF_SUCCESS;

  switch (p_ble_evt->header.evt_id)
  {
    case BLE_GAP_EVT_CONNECTED:
      NRF_LOG_INFO("%s: Connected\n", (uint32_t)__func__);
      uint16_t conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
      BMS_set_conn_handle(conn_handle);
      BLE_conn_handle_set(conn_handle);
      pm_secure_initiate(conn_handle);
      break;

    case BLE_GAP_EVT_DISCONNECTED:
      NRF_LOG_INFO("%s: Disconnected\n", (uint32_t)__func__);
      BMS_delete_disconnected_bonds();
      BLE_conn_handle_set(BLE_CONN_HANDLE_INVALID);
      break;

    case BLE_GATTC_EVT_TIMEOUT:
      NRF_LOG_DEBUG("%s: GATT Client Timeout\n", (uint32_t)__func__);
      err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                       BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
      APP_ERROR_CHECK(err_code);
      break;

    case BLE_GATTS_EVT_TIMEOUT:
      NRF_LOG_DEBUG("%s: GATT Server Timeout\n", (uint32_t)__func__);
      err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                       BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
      APP_ERROR_CHECK(err_code);
      break;

    case BLE_GAP_EVT_PASSKEY_DISPLAY:
    {
      char passkey[7];
      memcpy(passkey, p_ble_evt->evt.gap_evt.params.passkey_display.passkey, 6);
      passkey[6] = 0;
      NRF_LOG_INFO("Passkey: %s\n", nrf_log_push(passkey));
      break;
    }

    default:
      NRF_LOG_DEBUG("%s: event 0x%X\n", (uint32_t)__func__, p_ble_evt->header.evt_id);
      break;
    }
}

void ble_advertising_on_ble_evt(ble_evt_t const * p_ble_evt)
{
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_DEBUG("Connect");
            break;

        // Upon disconnection, whitelist will be activated and direct advertising is started.
        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_DEBUG("Disconnect");
            break;

        // Upon time-out, the next advertising mode is started.
        case BLE_GAP_EVT_TIMEOUT:
            NRF_LOG_DEBUG("Timeout");
            break;

        default:
            break;
    }
}


/*! \brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * \details This function is called from the BLE Stack event interrupt handler after a BLE stack
 *          event has been received.
 *
 * \param[in]   p_ble_evt   Bluetooth stack event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    ble_conn_state_on_ble_evt(p_ble_evt);
    pm_on_ble_evt(p_ble_evt);
    BMS_on_ble_evt(p_ble_evt);
    ble_conn_params_on_ble_evt(p_ble_evt);
    on_ble_evt(p_ble_evt);
    ble_advertising_on_ble_evt(p_ble_evt);
}


/*! ------------------------------------------------------------------------------
 * \brief Function for dispatching a system event to interested modules.
 *
 * \details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * \param[in]   sys_evt   System stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt)
{
    // Dispatch the system event to the fstorage module, where it will be
    // dispatched to the Flash Data Storage (FDS) module.
    fs_sys_event_handler(sys_evt);

    // Dispatch to the Advertising module last, since it will check if there are any
    // pending flash operations in fstorage. Let fstorage process system events first,
    // so that it can report correctly to the Advertising module.
    ble_advertising_on_sys_evt(sys_evt);
}


/**@brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated by button press.
 */
/*static void bsp_event_handler(bsp_event_t event)
{
    uint32_t err_code;
    switch (event)
    {
        case BSP_EVENT_SLEEP:
            sleep_mode_enter();
            break;

        case BSP_EVENT_DISCONNECT:
            err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break;

        case BSP_EVENT_WHITELIST_OFF:
            if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
            {
                err_code = ble_advertising_restart_without_whitelist();
                if (err_code != NRF_ERROR_INVALID_STATE)
                {
                    APP_ERROR_CHECK(err_code);
                }
            }
            break;

        default:
            break;
    }
}
*/




//-----------------------------------------------------------------------------
//      PUBLIC FUNCTIONS
//------------------------------------------------------------------------------
/*! \brief Function for initializing the BLE stack.
 *
 * \details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
  uint32_t err_code;

  nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC;

  // Initialize the SoftDevice handler module.
  SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);

  ble_enable_params_t ble_enable_params;
  err_code = softdevice_enable_get_default_config(CENTRAL_LINK_COUNT,
                                                  PERIPHERAL_LINK_COUNT,
                                                  &ble_enable_params);
  ASSERT(err_code == NRF_SUCCESS);

  //Check the ram settings against the used number of links
  CHECK_RAM_START_ADDR(CENTRAL_LINK_COUNT, PERIPHERAL_LINK_COUNT);

  // Enable BLE stack.
  #if (NRF_SD_BLE_API_VERSION == 3)
  ble_enable_params.gatt_enable_params.att_mtu = NRF_BLE_MAX_MTU_SIZE;
  #endif

  err_code = softdevice_enable(&ble_enable_params);
  ASSERT(err_code == NRF_SUCCESS);

  // Register with the SoftDevice handler module for BLE events.
  err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
  ASSERT(err_code == NRF_SUCCESS);

  // Register with the SoftDevice handler module for BLE events.
  err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
  ASSERT(err_code == NRF_SUCCESS);
}


//------------------------------------------------------------------------------
void BLE_Init(void)
{
  dbclichHandler_Init();

#if !defined(DISABLE_SOFTDEVICE) || (DISABLE_SOFTDEVICE == 0)
  ble_stack_init();
  
  bool erase_bonds = false;
  peer_manager_init(erase_bonds);
  if (erase_bonds == true)
  {
    NRF_LOG_INFO("Bonds erased!\r\n");
  }

  gap_params_init();

#if defined(USE_STATIC_PASSKEY) && USE_STATIC_PASSKEY
    static_passkey_def();
#endif

  advertising_init();
  BMS_init();
  conn_params_init();
  advertising_mode_set(BLE_ADV_MODE_FAST, true);
  advertising_stop();
  advertising_start();
#endif
}


//------------------------------------------------------------------------------
void BLE_Process(void)
{
  dbclick_Process();
}
