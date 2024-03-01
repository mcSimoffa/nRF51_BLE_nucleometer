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
#include "nrf_soc.h"
#include "pm.h"
#include "batMea.h"
#include "ble_ios.h"
#include "event_queue.h"
#include "realtime_particle_watcher.h"


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
#define BATTERY_LOW_INDICATED_REF       1500    // A minimal voltage reference to indicate
//------------------------------------------------------------------------------
//        PRIVATE FUNCTIONS PROTOTYPES
//------------------------------------------------------------------------------
static void ios_set_sys_time(uint16_t conn_handle, uint16_t datalen, uint8_t *p_data);
static void ios_sys_time_request(uint16_t conn_handle);
static void ios_instant_value_request(uint16_t conn_handle);
static void ios_evq_request(uint16_t conn_handle);
static void ios_evq_status_request(uint16_t conn_handle);
static void ios_temperature_request(uint16_t conn_handle);
static void ios_battery_request(uint16_t conn_handle);

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
    .len = {.init = 8, .max = 8, .var = false},
    .prop = {.read = 1, .write = 1},
    .rd_access = SEC_JUST_WORKS,
    .wr_access = SEC_JUST_WORKS,
    .cccd_wr_access = SEC_JUST_WORKS,
    .wrCb = ios_set_sys_time,
    .rdCb = ios_sys_time_request,
    .is_defered_read = true,
  },
  {
    .uuid = IOS_PULSE_CHAR,
    .len =  {.init = 4, .max = 4, .var = false},
    .prop = {.notify = 1},
    .cccd_wr_access = SEC_JUST_WORKS,
  },
  {
    .uuid = IOS_INSTANT_VALUE_CHAR,
    .len =  {.init = 2, .max = 2, .var = false},
    .prop = {.read = 1},
    .rd_access = SEC_JUST_WORKS,
    .cccd_wr_access = SEC_JUST_WORKS,
    .rdCb = ios_instant_value_request,
    .is_defered_read = true,
  },
  {
    .uuid = IOS_EVQ_CHAR,
    .len =  {.init = 4, .max = 4, .var = false},
    .prop = {.read = 1},
    .rd_access = SEC_JUST_WORKS,
    .rdCb = ios_evq_request,
    .is_defered_read = true,
  },
  {
    .uuid = IOS_EVQ_STATUS_CHAR,
    .len =  {.init = 10, .max = 10, .var = false},
    .prop = {.read = 1},
    .rd_access = SEC_JUST_WORKS,
    .rdCb = ios_evq_status_request,
    .is_defered_read = true,
  },
  {
    .uuid = IOS_TEMPERATURE_CHAR,
    .len =  {.init = 2, .max = 2, .var = false},
    .prop = {.read = 1},
    .rd_access = SEC_JUST_WORKS,
    .rdCb = ios_temperature_request,
    .is_defered_read = true,
  },
  {
    .uuid = IOS_BATTERY_CHAR,
    .len =  {.init = 1, .max = 1, .var = false},
    .prop = {.read = 1},
    .rd_access = SEC_JUST_WORKS,
    .rdCb = ios_battery_request,
    .is_defered_read = true,
  },
};

BLE_IOS_DEF(main_ios, &base_uuid, INPUT_OUTPUT_SERV, ios_chars, sizeof(ios_chars)/sizeof(char_desc_t));
APP_TIMER_DEF(sec_tmr);

static ble_ctx_t ble_ctx =
{
  .conn_handle = BLE_CONN_HANDLE_INVALID,
  .auth_rd_conn_handle = BLE_CONN_HANDLE_INVALID,
};

//------------------------------------------------------------------------------
//        PRIVATE FUNCTIONS
//------------------------------------------------------------------------------
static void ios_set_sys_time(uint16_t conn_handle, uint16_t datalen, uint8_t *p_data)
{
  if (datalen == sizeof (uint64_t))
  {
    uint64_t utc;
    memcpy(&utc, p_data, datalen);
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
static void ios_evq_request(uint16_t conn_handle)
{
  uint32_t cnt = EVQ_GetEvt();
  ret_code_t ret_code = ble_ios_rd_reply(conn_handle, &cnt, sizeof(cnt));
  APP_ERROR_CHECK(ret_code);
}

// ---------------------------------------------------------------------------
static void ios_evq_status_request(uint16_t conn_handle)
{
  struct
  {
    uint16_t events;
    uint64_t last_timestamp;
  } __PACKED answer;

  answer.events= EVQ_GetEventsAmount();
  answer.last_timestamp = EVQ_GetCurrentEventTimestamp();

  ret_code_t ret_code = ble_ios_rd_reply(conn_handle, &answer, sizeof(answer));
  APP_ERROR_CHECK(ret_code);
}

// ---------------------------------------------------------------------------
static void bat_mea_cb(uint16_t mv, void *ctx)
{
  ble_ctx_t *context = (ble_ctx_t*)ctx;

  if ((context->conn_handle == BLE_CONN_HANDLE_INVALID) || (context->auth_rd_conn_handle != context->conn_handle))
  {
    return; //don't send response if connection was lost or new connection established
  }

  uint8_t unit = (mv - BATTERY_LOW_INDICATED_REF) >> 3;  //1 unit = 8mV after BATTERY_LOW_INDICATED_REF mV
  NRF_LOG_INFO("Battery = %d mv, %d unit\n", mv, unit);

  ret_code_t ret_code = ble_ios_rd_reply(context->conn_handle, &unit, sizeof(unit));
  APP_ERROR_CHECK(ret_code);
}

// ---------------------------------------------------------------------------
static void ios_battery_request(uint16_t conn_handle)
{
  ble_ctx.auth_rd_conn_handle = conn_handle;

  if (batMea_Start(bat_mea_cb, (void*)&ble_ctx) == false)
  {
    // if batMea module is already in measure state and busy
    bat_mea_cb(BATTERY_LOW_INDICATED_REF, (void*)&ble_ctx);
  }
}

// ---------------------------------------------------------------------------
static void ios_temperature_request(uint16_t conn_handle)
{
  int32_t temp=0;
  sd_temp_get(&temp);
  ASSERT((temp > INT16_MIN) && (temp < INT16_MAX));
  int16_t short_temp = (int16_t)temp;
  ret_code_t ret_code = ble_ios_rd_reply(conn_handle, &short_temp, sizeof(short_temp));
  APP_ERROR_CHECK(ret_code);
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

// ---------------------------------------------------------------------------
static void ios_instant_value_request(uint16_t conn_handle)
{
  uint16_t val = RPW_GetInstant();
  ret_code_t ret_code = ble_ios_rd_reply(conn_handle, &val, sizeof(val));
  APP_ERROR_CHECK(ret_code);
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
      ret_code = sd_ble_gatts_sys_attr_set(p_ble_evt->evt.gap_evt.conn_handle, NULL, 0, 0);
      APP_ERROR_CHECK(ret_code);

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



/*! ---------------------------------------------------------------------------
 * \brief Function for initializing the BLE stack.
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
  APP_ERROR_CHECK(ret_code);

  //Check the ram settings against the used number of links
  CHECK_RAM_START_ADDR(CENTRAL_LINK_COUNT, PERIPHERAL_LINK_COUNT);

  // Enable BLE stack.
  #if (NRF_SD_BLE_API_VERSION == 3)
  ble_enable_params.gatt_enable_params.att_mtu = NRF_BLE_MAX_MTU_SIZE;
  #endif

  ret_code = softdevice_enable(&ble_enable_params);
  APP_ERROR_CHECK(ret_code);

  // Register with the SoftDevice handler module for BLE events.
  ret_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
  APP_ERROR_CHECK(ret_code);

  // Register with the SoftDevice handler module for BLE events.
  ret_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
  APP_ERROR_CHECK(ret_code);
}

//------------------------------------------------------------------------------
static void retCodeCheck(ret_code_t ret_code)
{
  if (ret_code != NRF_SUCCESS &&
      ret_code != BLE_ERROR_INVALID_CONN_HANDLE &&
      ret_code != NRF_ERROR_INVALID_STATE)
  {
      APP_ERROR_CHECK(ret_code);
  }

}


//------------------------------------------------------------------------------
//      PUBLIC FUNCTIONS
//------------------------------------------------------------------------------
void BLE_Init(bool erase_bonds)
{
  adv_ctrl_Init(&ble_ctx);

  ret_code_t ret = app_timer_create(&sec_tmr, APP_TIMER_MODE_SINGLE_SHOT, OnTimerEvent);
  APP_ERROR_CHECK(ret);

#if !defined(DISABLE_SOFTDEVICE) || (DISABLE_SOFTDEVICE == 0)
  ble_stack_init();

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

// ----------------------------------------------------------------------------
void ble_ios_pulse_transfer(uint32_t pulse)
{
  ret_code_t ret_code;

  if (ble_ctx.conn_handle == BLE_CONN_HANDLE_INVALID)
  {
    return;
  }

  ret_code = ble_ios_on_output_change(ble_ctx.conn_handle, &main_ios, IOS_PULSE_CHAR, &pulse, sizeof(pulse));
  retCodeCheck(ret_code);

  //ret_code = ble_ios_output_set(ble_ctx.conn_handle, &main_ios, IOS_PULSE_CHAR, &pulse, sizeof(pulse));
  //retCodeCheck(ret_code);
}