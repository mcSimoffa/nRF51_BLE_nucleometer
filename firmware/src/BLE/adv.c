#include <sdk_common.h>
#include <nrf_assert.h>
#include "ble_advertising.h"
#include "peer_manager.h"
#include <app_error.h>
#include "adv.h"

#define APP_ADV_FAST_INTERVAL           80    //Fast advertising interval (in units of 0.625 ms. This value corresponds to 50 ms.)
#define APP_ADV_SLOW_INTERVAL           (1600 * 2)  //Slow advertising interval (in units of 0.625 ms. This value corrsponds to 2 seconds)
#define APP_ADV_FAST_TIMEOUT            10    //The duration of the fast advertising period (in seconds).
#define APP_ADV_SLOW_TIMEOUT            30    //The duration of the slow advertising period (in seconds).

#define NRF_LOG_MODULE_NAME "ADV"
#include "nrf_log.h"

#define SERVICE_UUID 0xfdf0
typedef union
{
  struct
  {
    uint64_t  low:48;
    uint64_t  Mid:16;
    uint64_t  mId:16;
    uint64_t  miD:16;
    uint64_t  var:16;
    uint64_t  high:16; 
  } field;
  ble_uuid128_t uuid128;
} uuid_128_t;



// 5d51fdfX-06c2-11ed-aa05-0800200c9a66 x=0...A
static volatile uuid_128_t base_uuid=
{
  .field.low= 0x0800200c9a66,
  .field.Mid = 0xaa05,
  .field.mId = 0x11ed,
  .field.miD = 0x06c2,
  .field.var = 0x0000,
  .field.high = 0x5d51,
};

static ble_uuid_t m_adv_uuids[] = {{SERVICE_UUID,  BLE_UUID_TYPE_BLE},};    // Universally unique service identifiers.
static pm_peer_id_t   m_whitelist_peers[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];  // List of peers currently in the whitelist.
static uint32_t       m_whitelist_peer_cnt;                                 // Number of peers currently in the whitelist.

//------------------------------------------------------------------------------
//        PRIVATE FUNCTIONS
//------------------------------------------------------------------------------
/*! \brief Function for handling advertising events.
 *
 * \details This function will be called for advertising events which are passed to the application.
 *
 * \param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    uint32_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_DIRECTED:
            NRF_LOG_INFO("BLE_ADV_EVT_DIRECTED\r\n");
            break;

        case BLE_ADV_EVT_FAST:
            NRF_LOG_INFO("BLE_ADV_EVT_FAST\r\n");
            break;

        case BLE_ADV_EVT_SLOW:
            NRF_LOG_INFO("BLE_ADV_EVT_SLOW\r\n");
            break;

        case BLE_ADV_EVT_FAST_WHITELIST:
            NRF_LOG_INFO("BLE_ADV_EVT_FAST_WHITELIST\r\n");
            break;

        case BLE_ADV_EVT_SLOW_WHITELIST:
            NRF_LOG_INFO("BLE_ADV_EVT_SLOW_WHITELIST\r\n");
            break;

        case BLE_ADV_EVT_IDLE:
          NRF_LOG_INFO("BLE_ADV_EVT_IDLE\n");
            break;

        case BLE_ADV_EVT_WHITELIST_REQUEST:
        {
          NRF_LOG_INFO("BLE_ADV_EVT_WHITELIST_REQUEST\n");
         /*   NRF_LOG_DEBUG("BLE_ADV_EVT_WHITELIST_REQUEST\r\n");
            ble_gap_addr_t whitelist_addrs[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
            ble_gap_irk_t  whitelist_irks[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
            uint32_t addr_cnt = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;
            uint32_t irk_cnt  = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;

            m_whitelist_peer_cnt = (sizeof(m_whitelist_peers) / sizeof(pm_peer_id_t));

            // Fetch a list of peer IDs and whitelist them.
            peer_list_get(m_whitelist_peers, &m_whitelist_peer_cnt);
            NRF_LOG_DEBUG("peers %d \r\n", m_whitelist_peer_cnt);
            err_code = pm_whitelist_set(m_whitelist_peers, m_whitelist_peer_cnt);
            APP_ERROR_CHECK(err_code);
            err_code = pm_whitelist_get(whitelist_addrs, &addr_cnt,
                                        whitelist_irks,  &irk_cnt);
            APP_ERROR_CHECK(err_code);

            NRF_LOG_DEBUG("pm_whitelist_get returns %d addr in whitelist and %d irk whitelist\r\n",
                           addr_cnt,
                           irk_cnt);

            // Apply the whitelist.
            err_code = ble_advertising_whitelist_reply(whitelist_addrs, addr_cnt,
                                                       whitelist_irks,  irk_cnt);
            APP_ERROR_CHECK(err_code);*/
            break;
        }

        case BLE_ADV_EVT_PEER_ADDR_REQUEST:
        {
          NRF_LOG_INFO("BLE_ADV_EVT_PEER_ADDR_REQUEST\n");
        /*
            pm_peer_data_bonding_t peer_bonding_data;

            // Only Give peer address if we have a handle to the bonded peer.
            if (m_peer_id != PM_PEER_ID_INVALID)
            {
                err_code = pm_peer_data_bonding_load(m_peer_id, &peer_bonding_data);
                if (err_code != NRF_ERROR_NOT_FOUND)
                {
                    APP_ERROR_CHECK(err_code);

                    ble_gap_addr_t * p_peer_addr = &(peer_bonding_data.peer_ble_id.id_addr_info);
                    err_code = ble_advertising_peer_addr_reply(p_peer_addr);
                    APP_ERROR_CHECK(err_code);
                }
            }
            */
            break; //BLE_ADV_EVT_PEER_ADDR_REQUEST
        }

        default:
          NRF_LOG_INFO("%s: event %d\n", (uint32_t)__func__, ble_adv_evt);
          break;
    }
}

// ----------------------------------------------------------------------------
/*! \@brief Function for handling advertising errors.
 *
 * \param[in] nrf_error  Error code containing information about what went wrong.
 */
static void ble_advertising_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

// ----------------------------------------------------------------------------
/*! \brief Fetch the list of peer manager peer IDs.
 *
 * \param[inout] p_peers   The buffer where to store the list of peer IDs.
 * \param[inout] p_size    In: The size of the @p p_peers buffer.
 *                         Out: The number of peers copied in the buffer.
 */
static void peer_list_get(pm_peer_id_t * p_peers, uint32_t * p_size)
{
    pm_peer_id_t peer_id;
    uint32_t     peers_to_copy;

    peers_to_copy = (*p_size < BLE_GAP_WHITELIST_ADDR_MAX_COUNT) ?
                     *p_size : BLE_GAP_WHITELIST_ADDR_MAX_COUNT;

    peer_id = pm_next_peer_id_get(PM_PEER_ID_INVALID);
    *p_size = 0;

    while ((peer_id != PM_PEER_ID_INVALID) && (peers_to_copy--))
    {
        p_peers[(*p_size)++] = peer_id;
        peer_id = pm_next_peer_id_get(peer_id);
    }
}


//-----------------------------------------------------------------------------
//      PUBLIC FUNCTIONS
//-----------------------------------------------------------------------------
/**@brief Function for starting advertising.
 */
void advertising_start(void)
{
  ret_code_t ret;

  memset(m_whitelist_peers, PM_PEER_ID_INVALID, sizeof(m_whitelist_peers));
  m_whitelist_peer_cnt = (sizeof(m_whitelist_peers) / sizeof(pm_peer_id_t));

  peer_list_get(m_whitelist_peers, &m_whitelist_peer_cnt);

  ret = pm_whitelist_set(m_whitelist_peers, m_whitelist_peer_cnt);
  ASSERT(ret == NRF_SUCCESS);

  // Setup the device identies list.
  // Some SoftDevices do not support this feature.
  ret = pm_device_identities_list_set(m_whitelist_peers, m_whitelist_peer_cnt);
  if (ret != NRF_ERROR_NOT_SUPPORTED)
  {
      ASSERT(ret == NRF_SUCCESS);
  }

  ret = ble_advertising_start(BLE_ADV_MODE_FAST);
  ASSERT(ret == NRF_SUCCESS);
}


/*! ---------------------------------------------------------------------------
 \brief Function for initializing the Advertising functionality.
 */
void advertising_init(void)
{
    uint32_t               err_code;
    uint8_t                adv_flags;
    ble_advdata_t          advdata;
    ble_adv_modes_config_t options;

    // Build and set advertising data
    memset(&advdata, 0, sizeof(advdata));

    adv_flags                       = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance      = true;
    advdata.flags                   = adv_flags;
    advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    advdata.uuids_complete.p_uuids  = m_adv_uuids;

    memset(&options, 0, sizeof(options));
    options.ble_adv_whitelist_enabled      = true;
    options.ble_adv_directed_enabled       = true;
    options.ble_adv_directed_slow_enabled  = false;
    options.ble_adv_directed_slow_interval = 0;
    options.ble_adv_directed_slow_timeout  = 0;
    options.ble_adv_fast_enabled           = true;
    options.ble_adv_fast_interval          = APP_ADV_FAST_INTERVAL;
    options.ble_adv_fast_timeout           = APP_ADV_FAST_TIMEOUT;
    options.ble_adv_slow_enabled           = true;
    options.ble_adv_slow_interval          = APP_ADV_SLOW_INTERVAL;
    options.ble_adv_slow_timeout           = APP_ADV_SLOW_TIMEOUT;

    err_code = ble_advertising_init(&advdata, NULL, &options, on_adv_evt, ble_advertising_error_handler);
    ASSERT(err_code == NRF_SUCCESS);
}

