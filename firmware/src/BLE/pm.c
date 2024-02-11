#include <sdk_common.h>
#include "app_error.h"
#include "app_time_lib.h"
#include "ble_conn_state.h"
#include "peer_manager.h"
#include "fds.h"
#include "ble_main.h"
#include "adv.h"

#define NRF_LOG_MODULE_NAME "PEER_MG"
#define NRF_LOG_LEVEL         4
#include "nrf_log.h"


#define SEC_PARAM_BOND                  1     //Perform bonding.
#define SEC_PARAM_MITM                  1     // Man In The Middle protection not required.
#define SEC_PARAM_LESC                  0     // LE Secure Connections not enabled.
#define SEC_PARAM_KEYPRESS              0     // Keypress notifications not enabled.

// I/O capabilities.
#if defined(USE_STATIC_PASSKEY) && USE_STATIC_PASSKEY
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_DISPLAY_ONLY
#else
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE
#endif

#define SEC_PARAM_OOB                   0     // Out Of Band data not available
#define SEC_PARAM_MIN_KEY_SIZE          7     // Minimum encryption key size.
#define SEC_PARAM_MAX_KEY_SIZE          16    // Maximum encryption key size.

// ----------------------------------------------------------------------------
//   PRIVATE VARIABLE
// ----------------------------------------------------------------------------
static pm_peer_id_t                     m_peer_id;  // Device reference handle to the current bonded central.
static ble_ctx_t *ble_context;


// ----------------------------------------------------------------------------
//    PRIVATE FUNCTION
// ----------------------------------------------------------------------------
/*! \brief Function for handling Peer Manager events.
 *
 * \param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const * p_evt)
{
  ret_code_t err_code;

  switch (p_evt->evt_id)
  {
    case PM_EVT_BONDED_PEER_CONNECTED:
      NRF_LOG_INFO("PM_EVT_BONDED_PEER_CONNECTED\n");
      break;

    case PM_EVT_CONN_SEC_SUCCEEDED:
      NRF_LOG_INFO("PM_EVT_CONN_SEC_SUCCEEDED. Role: %d. conn_handle: %d, Procedure: %d\n",
                   ble_conn_state_role(p_evt->conn_handle),
                   p_evt->conn_handle,
                   p_evt->params.conn_sec_succeeded.procedure);
      break;

    case PM_EVT_CONN_SEC_FAILED:
      NRF_LOG_INFO("PM_EVT_CONN_SEC_FAILED\n");
      err_code = sd_ble_gap_disconnect(ble_context->conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
      ble_context->conn_handle = BLE_CONN_HANDLE_INVALID;
      APP_ERROR_CHECK(err_code);
      break;

    case PM_EVT_CONN_SEC_CONFIG_REQ:
      // Reject pairing request from an already bonded peer.
      NRF_LOG_INFO("PM_EVT_CONN_SEC_CONFIG_REQ\n");
      pm_conn_sec_config_t conn_sec_config = {.allow_repairing = false};
      pm_conn_sec_config_reply(p_evt->conn_handle, &conn_sec_config);
      break;

    case PM_EVT_STORAGE_FULL:
    {
      NRF_LOG_INFO("PM_EVT_STORAGE_FULL\n");
      // Run garbage collection on the flash.
      err_code = fds_gc();
      if (err_code == FDS_ERR_BUSY || err_code == FDS_ERR_NO_SPACE_IN_QUEUES)
      {
          // Retry.
      }
      else
      {
        APP_ERROR_CHECK(err_code);
      }
      break;
    }

    case PM_EVT_PEERS_DELETE_SUCCEEDED:
      NRF_LOG_INFO("PM_EVT_PEERS_DELETE_SUCCEEDED\n");
      break;

    case PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED:
      NRF_LOG_INFO("PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED\n");
      // The local database has likely changed, send service changed indications.
      pm_local_database_has_changed();
      break;

    case PM_EVT_PEER_DATA_UPDATE_FAILED:
      NRF_LOG_INFO("PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED\n");
      // Assert.
      APP_ERROR_CHECK(p_evt->params.peer_data_update_failed.error);
      break;

    case PM_EVT_PEER_DELETE_FAILED:
      // Assert.
      APP_ERROR_CHECK(p_evt->params.peer_delete_failed.error);
      break;

    case PM_EVT_PEERS_DELETE_FAILED:
      // Assert.
      APP_ERROR_CHECK(p_evt->params.peers_delete_failed_evt.error);
      break;

    case PM_EVT_ERROR_UNEXPECTED:
      // Assert.
      APP_ERROR_CHECK(p_evt->params.error_unexpected.error);
      break;

    default:
      NRF_LOG_INFO("%s: event %d\n", (uint32_t)__func__, p_evt->evt_id);
      break;
  }
}


void pmd(void)
{
  pm_peer_id_t current_peer_id = pm_next_peer_id_get(PM_PEER_ID_INVALID);
  while (current_peer_id != PM_PEER_ID_INVALID)
  {
    pm_peer_data_bonding_t b_data;
    ret_code_t rr = pm_peer_data_bonding_load(current_peer_id, &b_data);
    current_peer_id = pm_next_peer_id_get(current_peer_id);
  }
}

// ----------------------------------------------------------------------------
//    PUBLIC FUNCTION
// ----------------------------------------------------------------------------
/*! ---------------------------------------------------------------------------
 * \brief Function for the Peer Manager initialization.
 *
 * \param[in] erase_bonds  Indicates whether bonding information should be cleared from
 *                         persistent storage during initialization of the Peer Manager.
 */
void peer_manager_init(bool erase_bonds, ble_ctx_t *ctx)
{
  ble_gap_sec_params_t sec_param;
  ret_code_t err_code;

  ble_context = ctx;
  err_code = pm_init();
  APP_ERROR_CHECK(err_code);
//pmd();
  if (erase_bonds)
  {
    err_code = pm_peers_delete();
    APP_ERROR_CHECK(err_code);
  }

  memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

  // Security parameters to be used for all security procedures.
  sec_param.bond              = SEC_PARAM_BOND;
  sec_param.mitm              = SEC_PARAM_MITM;
  sec_param.lesc              = SEC_PARAM_LESC;
  sec_param.keypress          = SEC_PARAM_KEYPRESS;
  sec_param.io_caps           = SEC_PARAM_IO_CAPABILITIES;
  sec_param.oob               = SEC_PARAM_OOB;
  sec_param.min_key_size      = SEC_PARAM_MIN_KEY_SIZE;
  sec_param.max_key_size      = SEC_PARAM_MAX_KEY_SIZE;
  sec_param.kdist_own.enc     = 1;
  sec_param.kdist_own.id      = 1;
  sec_param.kdist_peer.enc    = 1;
  sec_param.kdist_peer.id     = 1;

  err_code = pm_sec_params_set(&sec_param);
  APP_ERROR_CHECK(err_code);

  err_code = pm_register(pm_evt_handler);
  APP_ERROR_CHECK(err_code);
}


/*! ---------------------------------------------------------------------------
 * \brief Function for start secure procedure by peripherial request.
 *
 * \param[in] conn_handle - connection handle for secure procedure
 */
void pm_secure_initiate(uint16_t conn_handle)
{
  pm_conn_sec_status_t    status;
  ret_code_t err_code = pm_conn_sec_status_get(conn_handle, &status);
  APP_ERROR_CHECK(err_code);

  NRF_LOG_INFO("%s: conn=%d enc= %d bond=%d\n", (uint32_t)__func__, status.connected, status.encrypted, status.bonded);
  // If the link is still not secured by the peer, initiate security procedure.
  if (!status.encrypted)
  {
      err_code = pm_conn_secure(conn_handle, false);
      APP_ERROR_CHECK(err_code);
  }
}