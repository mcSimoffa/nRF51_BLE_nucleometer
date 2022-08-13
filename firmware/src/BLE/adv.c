#include "sdk_common.h"
#include "nrf_assert.h"
#include "nrf_error.h"
#include "ble_advertising.h"
#include "peer_manager.h"
#include "fstorage.h"

#include "adv.h"

#if NRF_SD_BLE_API_VERSION != 2
#error "This API only for version 2"
#endif

#define APP_ADV_FAST_INTERVAL           80    //Fast advertising interval (in units of 0.625 ms. This value corresponds to 50 ms.)
#define APP_ADV_SLOW_INTERVAL           (1600 * 2)  //Slow advertising interval (in units of 0.625 ms. This value corrsponds to 2 seconds)
#define APP_ADV_FAST_TIMEOUT            20    //The duration of the fast advertising period (in seconds).
#define APP_ADV_SLOW_TIMEOUT            30    //The duration of the slow advertising period (in seconds).

#define NRF_LOG_MODULE_NAME "ADV"
#define NRF_LOG_LEVEL        4
#include "nrf_log.h"

#define SERVICE_UUID 0xfdf0




//------------------------------------------------------------------------------
//        PRIVATE VARIABLE
//------------------------------------------------------------------------------
static ble_uuid_t     m_adv_uuids[] = {{SERVICE_UUID,  BLE_UUID_TYPE_BLE},};    // Universally unique service identifiers.
static pm_peer_id_t   m_whitelist_peers[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];  // List of peers currently in the whitelist.
static uint32_t       m_whitelist_peer_cnt;                                 // Number of peers currently in the whitelist.

static ble_adv_mode_t m_adv_mode_current;
static bool m_advertising_start_pending;
static bool                   m_whitelist_en;
static ble_advdata_t          advdata;
static ble_adv_modes_config_t options;

// For SoftDevices v 2.x, this module caches a whitelist which is retrieved from the
// application using an event, and which is passed as a parameter when calling
// sd_ble_gap_adv_start().

static ble_gap_addr_t      *m_p_whitelist_addrs[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
static ble_gap_irk_t       *m_p_whitelist_irks[BLE_GAP_WHITELIST_IRK_MAX_COUNT];
static ble_gap_addr_t      m_whitelist_addrs[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
static ble_gap_irk_t       m_whitelist_irks[BLE_GAP_WHITELIST_IRK_MAX_COUNT];

static ble_gap_whitelist_t   m_whitelist =
{
  .pp_addrs = m_p_whitelist_addrs,
  .pp_irks  = m_p_whitelist_irks
};


//------------------------------------------------------------------------------
//        PRIVATE FUNCTIONS
//------------------------------------------------------------------------------

/*! ----------------------------------------------------------------------------
 * \brief Function to determine if a flash write operation in in progress.
 *
 * \return true if a flash operation is in progress, false if not.
 */
static bool flash_access_in_progress()
{
    uint32_t count;

    (void)fs_queued_op_count_get(&count);

    return (count != 0);
}


/*! ----------------------------------------------------------------------------
 * \brief Fetch the list of peer manager peer IDs.
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
static bool whitelist_has_entries()
{
    return ((m_whitelist.addr_count != 0) || (m_whitelist.irk_count != 0));
}




//-----------------------------------------------------------------------------
//      PUBLIC FUNCTIONS
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
void advertising_init(void)
{
  // Build and set advertising data
  memset(&advdata, 0, sizeof(advdata));
  advdata.name_type               = BLE_ADVDATA_FULL_NAME;
  advdata.include_appearance      = true;
  advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
  advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
  advdata.uuids_complete.p_uuids  = m_adv_uuids;

  ret_code_t ret = ble_advdata_set(&advdata, NULL);
  APP_ERROR_CHECK(ret);

  // Build and set advertising options
  memset(&options, 0, sizeof(options));
  options.ble_adv_whitelist_enabled      = true;
  options.ble_adv_directed_enabled       = false;
  options.ble_adv_directed_slow_enabled  = false;
  options.ble_adv_directed_slow_interval = 0;
  options.ble_adv_directed_slow_timeout  = 0;
  options.ble_adv_fast_enabled           = true;
  options.ble_adv_fast_interval          = APP_ADV_FAST_INTERVAL;
  options.ble_adv_fast_timeout           = APP_ADV_FAST_TIMEOUT;
  options.ble_adv_slow_enabled           = true;
  options.ble_adv_slow_interval          = APP_ADV_SLOW_INTERVAL;
  options.ble_adv_slow_timeout           = APP_ADV_SLOW_TIMEOUT;
  NRF_LOG_INFO("Adv Option Init\n");
  // prepare whitelist array and elements binding
  for (int i = 0; i <BLE_GAP_WHITELIST_ADDR_MAX_COUNT ; i++)
  {
      m_whitelist.pp_addrs[i] = &m_whitelist_addrs[i];
  }

  for (int i = 0; i <BLE_GAP_WHITELIST_IRK_MAX_COUNT ; i++)
  {
      m_whitelist.pp_irks[i] = &m_whitelist_irks[i];
  }
}


//-----------------------------------------------------------------------------
ret_code_t advertising_mode_set(ble_adv_mode_t advertising_mode, bool whitelist_en)
{
   // Only BLE_ADV_MODE_FAST and BLE_ADV_MODE_SLOW modes are supporded
  if ((advertising_mode != BLE_ADV_MODE_FAST && advertising_mode != BLE_ADV_MODE_SLOW))
  {
    return NRF_ERROR_INVALID_PARAM;
  }
  m_adv_mode_current = advertising_mode;
  m_whitelist_en = whitelist_en;
  return NRF_SUCCESS;
}


//-----------------------------------------------------------------------------
ret_code_t advertising_start(void)
{
  ble_gap_adv_params_t adv_params;
  ret_code_t ret;

  // Delay starting advertising until the flash operations are complete.
  if (flash_access_in_progress())
  {
     NRF_LOG_INFO("%s: BUSY\n", (uint32_t)__func__);
    return NRF_ERROR_BUSY;

  }

  // Fetch the whitelist.
  if (options.ble_adv_whitelist_enabled && m_whitelist_en)
  {
    uint32_t addr_cnt = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;
    uint32_t irk_cnt  = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;
    m_whitelist_peer_cnt = (sizeof(m_whitelist_peers) / sizeof(pm_peer_id_t));

    // Fetch a list of peer IDs and whitelist them.
    peer_list_get(m_whitelist_peers, &m_whitelist_peer_cnt);
    NRF_LOG_DEBUG("peers %d \r\n", m_whitelist_peer_cnt);

    ret = pm_whitelist_set(m_whitelist_peers, m_whitelist_peer_cnt);
    APP_ERROR_CHECK(ret);

    ret = pm_whitelist_get(m_whitelist_addrs, &addr_cnt, m_whitelist_irks,  &irk_cnt);
    APP_ERROR_CHECK(ret);

    NRF_LOG_DEBUG("pm_whitelist_get returns %d addrs, %d irks\n",
                   addr_cnt,
                   irk_cnt);

    m_whitelist.addr_count = (uint8_t)addr_cnt;
    m_whitelist.irk_count  = (uint8_t)irk_cnt;
  }

  // Initialize advertising parameters with default values.
  memset(&adv_params, 0, sizeof(adv_params));

  adv_params.type = BLE_GAP_ADV_TYPE_ADV_IND;
  adv_params.fp   = BLE_GAP_ADV_FP_ANY;
  if (m_adv_mode_current == BLE_ADV_MODE_FAST)
  {
    adv_params.interval = options.ble_adv_fast_interval;
    adv_params.timeout  = options.ble_adv_fast_timeout;
  }
  else
  {
    adv_params.interval = options.ble_adv_slow_interval;
    adv_params.timeout  = options.ble_adv_slow_timeout;
  }

  if (m_whitelist_en && whitelist_has_entries())
  {
    adv_params.p_whitelist = &m_whitelist;
    adv_params.fp = BLE_GAP_ADV_FP_FILTER_CONNREQ;
    advdata.flags  = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;
    ret = ble_advdata_set(&advdata, NULL);
    APP_ERROR_CHECK(ret);

    NRF_LOG_DEBUG("Adv mode %d WHITELIST\n", m_adv_mode_current);
  }
  else
  {
    NRF_LOG_DEBUG("Adv mode %d\n", m_adv_mode_current);
  }

  ret = sd_ble_gap_adv_start(&adv_params);
  APP_ERROR_CHECK(ret);
  return NRF_SUCCESS;
}


//-----------------------------------------------------------------------------
void advertising_stop(void)
{
  m_advertising_start_pending = false;
  ret_code_t ret = sd_ble_gap_adv_stop();
  NRF_LOG_DEBUG("Stop Code %d\n", ret);
}
