#include "sdk_common.h"
#include "app_error.h"
#include "nrf_log_ctrl.h"
#include "ble.h"
#include "ble_srv_common.h"
#include "softdevice_handler.h"
#include "ble_conn_state.h"
#include "ble_conn_params.h"
#include "fstorage.h"

#include "app_time_lib.h"
#include "adv.h"
#include "adv_ctrl.h"
#include "conn.h"
#include "bond_mgmt_srv.h"
#include "pm.h"
#include "pm.h"
#include "batMea.h"
#include "ble_ios.h"

#include "ble_main.h"
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
//        PRIVATE FUNCTIONS PROTOTYPES
//------------------------------------------------------------------------------
static void ios_set_sys_time(uint16_t conn_handle, uint16_t datalen, uint8_t *p_data);
static void ios_sys_time_request(uint16_t conn_handle);
static void ios_dev_stat_request(uint16_t conn_handle);

//------------------------------------------------------------------------------
//        PRIVATE VARIABLES
//------------------------------------------------------------------------------
// 5d51fdfX-06c2-11ed-aa05-0800200c9a66 x=0...A
static uuid_128_t base_uuid=
{
  .field.low= 0x0800200c9a66,
  .field.Mid = 0xaa05,
  .field.mId = 0x11ed,
  .field.miD = 0x06c2,
  .field.var = 0x0000,
  .field.high = 0x5d51,
};

// ble_ios characteristics
static const char_desc_t ios_chars[] =
{
  {
    .uuid = IOS_SYSTIME_CHAR,
    .len = {.init = 4, .max = 8, .var = true},
    .prop = {.read = 1, .write = 1},
    .rd_access = SEC_JUST_WORKS,
    .wr_access = SEC_JUST_WORKS,
    .cccd_wr_access = SEC_JUST_WORKS,
    .wrCb = ios_set_sys_time,
    .rdCb = ios_sys_time_request,
  },
  {
    .uuid = IOS_PULSE_CHAR,
    .len =  {.init = 4, .max = 8, .var = true},
    .prop = {.read = 1, .notify = 1},
    .rd_access = SEC_JUST_WORKS,
    .wr_access = SEC_NO_ACCESS,
    .cccd_wr_access = SEC_JUST_WORKS,
    .wrCb = NULL,
    .rdCb = NULL,//ios_get_pulse,
  },
  {
    .uuid = IOS_DEVSTAT_CHAR,
    .len =  {.init = 1, .max = 3, .var = true},
    .prop = {.read = 1, .write = 1},
    .rd_access = SEC_JUST_WORKS,
    .wr_access = SEC_JUST_WORKS,
    .cccd_wr_access = SEC_JUST_WORKS,
    .wrCb = NULL,
    .rdCb = ios_dev_stat_request,
  },
  {
    .uuid = IOS_HW_PARAM_CHAR,
    .len =  {.init = 8, .max = 8, .var = false},
    .prop = {.read = 1, .write = 1},
    .rd_access = SEC_JUST_WORKS,
    .wr_access = SEC_JUST_WORKS,
    .cccd_wr_access = SEC_JUST_WORKS,
    .wrCb = NULL,
    .rdCb = NULL,
  },
};

BLE_IOS_DEF(main_ios, &base_uuid, INPUT_OUTPUT_SERV, ios_chars, sizeof(ios_chars)/sizeof(char_desc_t));
APP_TIMER_DEF(sec_tmr);
static ble_ctx_t ble_ctx;

//------------------------------------------------------------------------------
//        PRIVATE FUNCTIONS
//------------------------------------------------------------------------------
static void ios_set_sys_time(uint16_t conn_handle, uint16_t datalen, uint8_t *p_data)
{
  if (datalen == sizeof (uint64_t))
  {
    uint64_t utc;
    memcpy(&utc, p_data, 8);
    app_time_Set_UTC(utc);
  }
}

// ---------------------------------------------------------------------------
static void ios_sys_time_request(uint16_t conn_handle)
{
  uint64_t now = app_time_Get_UTC();
  ret_code_t ret_code = ble_ios_rd_reply(conn_handle, &now, sizeof(now));
  APP_ERROR_CHECK(ret_code);
}

// ---------------------------------------------------------------------------
static void bat_acqure(uint16_t mv, void *ctx)
{
  ble_ctx_t *context = (ble_ctx_t*)ctx;

  if ((context->conn_handle == BLE_CONN_HANDLE_INVALID) || (context->auth_rd_conn_handle != context->conn_handle))
  {
    return; //don't send response if connection was lost or new connection established
  }

  uint8_t unit = (mv - 1500) >> 3;  //1 unit = 8mV after 1500mV
  NRF_LOG_INFO("Battery = %d mv, %d unit\n", mv, unit);

  ret_code_t ret_code = ble_ios_rd_reply(context->conn_handle, &unit, sizeof(unit));
  APP_ERROR_CHECK(ret_code);
}

// ---------------------------------------------------------------------------
static void ios_dev_stat_request(uint16_t conn_handle)
{
  ble_ctx.auth_rd_conn_handle = conn_handle;

  if (batMea_Start(bat_acqure, (void*)&ble_ctx) == false)
  {
    bat_acqure(1500, (void*)&ble_ctx);
  }
}

/*! ---------------------------------------------------------------------------
 * \brief Function for secure procedure start by server initiative.
 */
static void OnTimerEvent(void * p_context)
{
  NRF_LOG_INFO("SECURE timer\n");
  if (ble_ctx.conn_handle !=BLE_CONN_HANDLE_INVALID)
  {
    pm_secure_initiate(ble_ctx.conn_handle);
  }
}


/*! ---------------------------------------------------------------------------
 * \brief Function for handling the Application's BLE Stack events.
 *
 * \param[in]   p_ble_evt   Bluetooth stack event.
 */
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
  ret_code_t ret_code = NRF_SUCCESS;

  switch (p_ble_evt->header.evt_id)
  {
    case BLE_GAP_EVT_CONNECTED:
    {
      NRF_LOG_INFO("%s: Connected\n", (uint32_t)__func__);
      uint16_t conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
      ble_ctx.conn_handle = conn_handle;
      ret_code_t error = app_timer_start(sec_tmr, MS_TO_TICK(1000), NULL);
      ASSERT(error == NRF_SUCCESS);
      
      break;
    }

    case BLE_GAP_EVT_DISCONNECTED:
      NRF_LOG_INFO("%s: Disconnected\n", (uint32_t)__func__);
      ble_ctx.conn_handle = BLE_CONN_HANDLE_INVALID;
      break;

    case BLE_GATTC_EVT_TIMEOUT:
      NRF_LOG_DEBUG("%s: GATT Client Timeout\n", (uint32_t)__func__);
      ret_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                       BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
      APP_ERROR_CHECK(ret_code);
      break;

    case BLE_GATTS_EVT_TIMEOUT:
      NRF_LOG_DEBUG("%s: GATT Server Timeout\n", (uint32_t)__func__);
      ret_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                       BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
      APP_ERROR_CHECK(ret_code);
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

// ----------------------------------------------------------------------------
void ble_advertising_on_ble_evt(ble_evt_t const * p_ble_evt)
{
  switch (p_ble_evt->header.evt_id)
  {
    case BLE_GAP_EVT_CONNECTED:
      NRF_LOG_DEBUG("Connect\n");
      break;

    // Upon disconnection, whitelist will be activated and direct advertising is started.
    case BLE_GAP_EVT_DISCONNECTED:
      NRF_LOG_DEBUG("Disconnect\n");
      break;

    // Upon time-out, the next advertising mode is started.
    case BLE_GAP_EVT_TIMEOUT:
      NRF_LOG_DEBUG("Timeout\n");
      break;

    default:
      break;
  }
}

/*! ------------------------------------------------------------------------------
 * \brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
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
    ble_conn_params_on_ble_evt(p_ble_evt);
    on_ble_evt(p_ble_evt);
    ble_ios_on_ble_evt(p_ble_evt, (void*)&main_ios);
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
    uint32_t ret_code;
    switch (event)
    {
        case BSP_EVENT_SLEEP:
            sleep_mode_enter();
            break;

        case BSP_EVENT_DISCONNECT:
            ret_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (ret_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(ret_code);
            }
            break;

        case BSP_EVENT_WHITELIST_OFF:
            if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
            {
                ret_code = ble_advertising_restart_without_whitelist();
                if (ret_code != NRF_ERROR_INVALID_STATE)
                {
                    APP_ERROR_CHECK(ret_code);
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
  uint32_t ret_code;

  nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC;

  // Initialize the SoftDevice handler module.
  SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);

  ble_enable_params_t ble_enable_params;
  ret_code = softdevice_enable_get_default_config(CENTRAL_LINK_COUNT,
                                                  PERIPHERAL_LINK_COUNT,
                                                  &ble_enable_params);
  ASSERT(ret_code == NRF_SUCCESS);

  //Check the ram settings against the used number of links
  CHECK_RAM_START_ADDR(CENTRAL_LINK_COUNT, PERIPHERAL_LINK_COUNT);

  // Enable BLE stack.
  #if (NRF_SD_BLE_API_VERSION == 3)
  ble_enable_params.gatt_enable_params.att_mtu = NRF_BLE_MAX_MTU_SIZE;
  #endif

  ret_code = softdevice_enable(&ble_enable_params);
  ASSERT(ret_code == NRF_SUCCESS);

  // Register with the SoftDevice handler module for BLE events.
  ret_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
  ASSERT(ret_code == NRF_SUCCESS);

  // Register with the SoftDevice handler module for BLE events.
  ret_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
  ASSERT(ret_code == NRF_SUCCESS);
}


//------------------------------------------------------------------------------
void BLE_Init(void)
{
  ble_ctx.conn_handle = BLE_CONN_HANDLE_INVALID;
  adv_ctrl_Init(&ble_ctx);

  ret_code_t ret = app_timer_create(&sec_tmr, APP_TIMER_MODE_SINGLE_SHOT, OnTimerEvent);
  ASSERT(ret == NRF_SUCCESS);

#if !defined(DISABLE_SOFTDEVICE) || (DISABLE_SOFTDEVICE == 0)
  ble_stack_init();

  bool erase_bonds = false;
  peer_manager_init(erase_bonds, &ble_ctx);
  if (erase_bonds == true)
  {
    NRF_LOG_INFO("Bonds erased!\r\n");
  }

  gap_params_init(&ble_ctx);

#if defined(USE_STATIC_PASSKEY) && USE_STATIC_PASSKEY
    static_passkey_def();
#endif

  advertising_init();
  ble_ios_init(&main_ios);
  conn_params_init();
#endif
}


//------------------------------------------------------------------------------
void BLE_Process(void)
{
 adv_ctrl_Process();
}
