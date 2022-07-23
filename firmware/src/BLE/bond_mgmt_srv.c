#include "nrf_ble_qwr.h"
#include "nrf_ble_bms.h"
#include "ble_conn_state.h"
#include "nrf_assert.h"
#include "peer_manager.h"

#define NRF_LOG_MODULE_NAME "BMS"
#include "nrf_log.h"

#define MEM_BUFF_SIZE 512

// ----------------------------------------------------------------------------
//   PRIVATE TYPES
// ----------------------------------------------------------------------------
typedef enum
{
    NO_DELETE,
    DELETE_ALL_BONDS,
    DELETE_CUR_BOND,
    DELETE_ALL_BUT_CUR_BOND,
}delete_bonds_t;

static nrf_ble_bms_t                    m_bms;                                      /**< Structure used to identify the Bond Management service. */


// ----------------------------------------------------------------------------
//   PRIVATE VARIABLE
// ----------------------------------------------------------------------------
static delete_bonds_t delete_operation = NO_DELETE;
static ble_conn_state_user_flag_id_t    m_bms_bonds_to_delete;                      //!< Flags used to identify bonds that should be deleted. */
static nrf_ble_qwr_t        m_qwr;
static uint8_t   mem[MEM_BUFF_SIZE];



/**@brief Function for handling events from bond management service.
 */
static void bms_evt_handler(nrf_ble_bms_t * p_ess, nrf_ble_bms_evt_t * p_evt)
{
    ret_code_t err_code;

    switch (p_evt->evt_type)
    {
        case NRF_BLE_BMS_EVT_AUTH:
        {
            bool is_authorized = true;
#if USE_AUTHORIZATION_CODE
            if ((p_evt->auth_code.len != m_auth_code_len) ||
                (memcmp(m_auth_code, p_evt->auth_code.code, m_auth_code_len) != 0))
            {
                is_authorized = false;
            }
#endif
            err_code = nrf_ble_bms_auth_response(&m_bms, is_authorized);
            ASSERT(err_code == NRF_SUCCESS);
        } break; //NRF_BLE_BMS_EVT_AUTH

        default:
        break;
    }
}

// ----------------------------------------------------------------------------
uint16_t qwr_evt_handler(nrf_ble_qwr_t * p_qwr, nrf_ble_qwr_evt_t * p_evt)
{
    return nrf_ble_bms_on_qwr_evt(&m_bms, &m_qwr, p_evt);
}

// ----------------------------------------------------------------------------
/**@brief Function for deleting all bonds
*/
static void delete_all_bonds(nrf_ble_bms_t const * p_bms)
{
  uint32_t err_code;
  uint16_t conn_handle;

  NRF_LOG_INFO("Client requested that all bonds be deleted\r\n");

  pm_peer_id_t peer_id = pm_next_peer_id_get(PM_PEER_ID_INVALID);
  while (peer_id != PM_PEER_ID_INVALID)
  {
    err_code = pm_conn_handle_get(peer_id, &conn_handle);
    ASSERT(err_code == NRF_SUCCESS);

    if (conn_handle != BLE_CONN_HANDLE_INVALID)
    {
      //Defer the deletion since this connection is active.
      NRF_LOG_DEBUG("Defer the deletion\r\n");
      ble_conn_state_user_flag_set(conn_handle, m_bms_bonds_to_delete, true);
    }
    else
    {
      err_code = pm_peer_delete(peer_id);
      ASSERT(err_code == NRF_SUCCESS);
    }

    peer_id = pm_next_peer_id_get(peer_id);
  }
}


// ----------------------------------------------------------------------------
/**@brief Function for handling Service errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void service_error_handler(uint32_t nrf_error)
{
  APP_ERROR_HANDLER(nrf_error);
}

// ----------------------------------------------------------------------------
/**@brief Function for deleting the current bond
*/
static void delete_requesting_bond(nrf_ble_bms_t const * p_bms)
{
  ble_conn_state_user_flag_set(p_bms->conn_handle, m_bms_bonds_to_delete, true);
}


// ----------------------------------------------------------------------------
//    PUBLIC FUNCTION
// ----------------------------------------------------------------------------
/**@brief Function for initializing the services that will be used by the application.
 *
 * @details Initialize the Glucose, Battery and Device Information services.
 */
void BMS_init(void)
{
    uint32_t             err_code;
    nrf_ble_bms_init_t   bms_init;
    nrf_ble_qwr_init_t   qwr_init;

    // Initialize Queued Write Module
    memset(&qwr_init, 0, sizeof(qwr_init));
    qwr_init.mem_buffer.len   = MEM_BUFF_SIZE;
    qwr_init.mem_buffer.p_mem = mem;
    qwr_init.callback         = qwr_evt_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    ASSERT(err_code == NRF_SUCCESS);

    // Initialize Bond Management Service
    memset(&bms_init, 0, sizeof(bms_init));

    m_bms_bonds_to_delete        = ble_conn_state_user_flag_acquire();
    bms_init.evt_handler         = bms_evt_handler;
    bms_init.error_handler       = service_error_handler;
#if USE_AUTHORIZATION_CODE
    bms_init.feature.delete_requesting_auth         = true;
    bms_init.feature.delete_all_auth                = true;
    bms_init.feature.delete_all_but_requesting_auth = false;
#else
    bms_init.feature.delete_requesting              = true;
    bms_init.feature.delete_all                     = true;
    bms_init.feature.delete_all_but_requesting      = false;
#endif
    bms_init.bms_feature_sec_req = SEC_JUST_WORKS;
    bms_init.bms_ctrlpt_sec_req  = SEC_JUST_WORKS;

    bms_init.p_qwr                                       = &m_qwr;
    bms_init.bond_callbacks.delete_requesting            = delete_requesting_bond;
    bms_init.bond_callbacks.delete_all                   = delete_all_bonds;

    err_code = nrf_ble_bms_init(&m_bms, &bms_init);
    ASSERT(err_code == NRF_SUCCESS);
}

// ---------------------------------------------------------------------------
void BMS_set_conn_handle(uint16_t conn_handle)
{
  ret_code_t err_code = nrf_ble_bms_set_conn_handle(&m_bms, conn_handle);
  ASSERT(err_code == NRF_SUCCESS);
  err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, conn_handle);
  ASSERT(err_code == NRF_SUCCESS);
}

// ---------------------------------------------------------------------------
void BMS_on_ble_evt(ble_evt_t * p_ble_evt)
{
  nrf_ble_bms_on_ble_evt(&m_bms, p_ble_evt);
  nrf_ble_qwr_on_ble_evt(&m_qwr, p_ble_evt);
}

// ---------------------------------------------------------------------------
void delete_disconnected_bonds(void)
{
    uint32_t err_code;
    sdk_mapped_flags_key_list_t conn_handle_list = ble_conn_state_conn_handles();

    for (uint32_t i = 0; i < conn_handle_list.len; i++)
    {
        pm_peer_id_t peer_id;
        uint16_t conn_handle = conn_handle_list.flag_keys[i];
        bool pending         = ble_conn_state_user_flag_get(conn_handle, m_bms_bonds_to_delete);

        if (pending)
        {
            NRF_LOG_DEBUG("Attempting to delete bond.\r\n");
            err_code = pm_peer_id_get(conn_handle, &peer_id);
            if (err_code == NRF_SUCCESS)
            {
                err_code = pm_peer_delete(peer_id);
                APP_ERROR_CHECK(err_code);
            }
        }
    }
}