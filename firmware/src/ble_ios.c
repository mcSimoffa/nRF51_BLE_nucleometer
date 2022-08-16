#include "sdk_common.h"
#include "app_error.h"
#include "ble_srv_common.h"
#include "nrf_assert.h"

#include "ble_ios.h"

#define NRF_LOG_MODULE_NAME "BLEIOS"
#define NRF_LOG_LEVEL        3
#include "nrf_log.h"
//------------------------------------------------------------------------------
//        PRIVATE TYPES
//------------------------------------------------------------------------------



/*! ------------------------------------------------------------------------------
 * \brief Function for handling the Write event.
 *
 * \param[in] cd          Char descriptor pointer.
 * \param[in] p_ble_evt   Event received from the BLE stack.
 */
static void on_write(const char_desc_t *cd, ble_evt_t const * p_ble_evt)
{
  ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
  NRF_LOG_HEXDUMP_INFO(&p_evt_write->data, p_evt_write->len);
  if ((p_evt_write->len > 0) && (cd->wrCb != NULL))
  {
    cd->wrCb(p_ble_evt->evt.gatts_evt.conn_handle, p_evt_write->len, (uint8_t*)&p_evt_write->data);
  }
}


static void on_read(const char_desc_t *cd, ble_evt_t const * p_ble_evt)
{
  if (cd->rdCb != NULL)
  {
    cd->rdCb(p_ble_evt->evt.gatts_evt.conn_handle);
  }
}

//-----------------------------------------------------------------------------
//      PUBLIC FUNCTIONS
//-----------------------------------------------------------------------------
void ble_ios_init(const ble_ios_t *p_ios)
{
    uint32_t              err_code;
    ble_uuid_t            ble_uuid;
    ble_add_char_params_t add_char_params;


    // Add service.
    uint8_t     uuid_type;
    err_code = sd_ble_uuid_vs_add(&p_ios->p_base_uuid->uuid128, &uuid_type);
    APP_ERROR_CHECK(err_code);

    ble_uuid.type = uuid_type;
    ble_uuid.uuid = p_ios->service_uuid;

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, p_ios->service_handle);
    APP_ERROR_CHECK(err_code);
    
    // Add Output characteristics.
    for (uint8_t i=0; i<p_ios->chars_total; i++)
    {
      memset(&add_char_params, 0, sizeof(add_char_params));
      add_char_params.uuid              = p_ios->char_list[i].uuid;
      add_char_params.uuid_type         = uuid_type;
      add_char_params.init_len          = p_ios->char_list[i].len.init;
      add_char_params.max_len           = p_ios->char_list[i].len.max;
      add_char_params.is_var_len        = p_ios->char_list[i].len.var;
      add_char_params.char_props        = p_ios->char_list[i].prop;
      add_char_params.is_defered_read   = true;

      add_char_params.read_access       = p_ios->char_list[i].rd_access;
      add_char_params.write_access      = p_ios->char_list[i].wr_access;
      add_char_params.cccd_write_access = p_ios->char_list[i].cccd_wr_access;

      err_code = characteristic_add(*p_ios->service_handle, &add_char_params, &p_ios->char_handles[i]);
      APP_ERROR_CHECK(err_code);
    }
}


// -----------------------------------------------------------------------------
void ble_ios_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
  ble_ios_t *p_ios = (ble_ios_t*)p_context;
  
  switch (p_ble_evt->header.evt_id)
  {
    case BLE_GATTS_EVT_WRITE:
      for(uint8_t i=0; i<p_ios->chars_total; i++)
      {
        if (p_ble_evt->evt.gatts_evt.params.write.handle == p_ios->char_handles[i].value_handle)
        {
          NRF_LOG_INFO("got data for UUID 0x%4X\n", p_ios->char_list[i].uuid);
          on_write(&p_ios->char_list[i], p_ble_evt);
          break;
        }
      }
      break;

    case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
      if (p_ble_evt->evt.gatts_evt.params.authorize_request.type == BLE_GATTS_AUTHORIZE_TYPE_READ)
      {
        for(uint8_t i=0; i<p_ios->chars_total; i++)
        {
          if (p_ble_evt->evt.gatts_evt.params.authorize_request.request.read.handle == p_ios->char_handles[i].value_handle)
          {
            NRF_LOG_INFO("auth request for UUID 0x%4X\n", p_ios->char_list[i].uuid);
            on_read(&p_ios->char_list[i], p_ble_evt);
            break;
          }
        }
      }
      break;

    default:
      // No implementation needed.
      break;
  }
}

// -----------------------------------------------------------------------------
ret_code_t ble_ios_rd_reply(uint16_t conn_handle, void *p_data, uint16_t len)
{
  ret_code_t ret_code;
  ble_gatts_rw_authorize_reply_params_t reply_params = {0};

  reply_params.type = BLE_GATTS_AUTHORIZE_TYPE_READ;
  reply_params.params.read.p_data    = p_data;
  reply_params.params.read.len       = len;
  reply_params.params.read.offset    = 0;
  reply_params.params.read.update    = 1;
  reply_params.params.read.gatt_status = NRF_SUCCESS;

  ret_code = sd_ble_gatts_rw_authorize_reply(conn_handle, &reply_params);
  if (ret_code == NRF_ERROR_BUSY)
  {
    NRF_LOG_WARNING("%s: retcode BUSY", (uint32_t)__func__, ret_code);
  }
  else
  {
    APP_ERROR_CHECK(ret_code);
  }
  return ret_code;
}

// -----------------------------------------------------------------------------
ret_code_t ble_ios_on_output_change(uint16_t conn_handle, const ble_ios_t *p_ios, uint16_t uuid, void *p_data, uint8_t len)
{
  ble_gatts_hvx_params_t params = {0};
  uint16_t length = len;
  uint16_t value_handle;

  for(uint8_t i=0; i<p_ios->chars_total; i++)
  {
    if (p_ios->char_list[i].uuid == uuid)
    {
      value_handle = p_ios->char_handles[i].value_handle;
      NRF_LOG_INFO("Set data for UUID 0x%4X handle %d\n", uuid, value_handle);

      params.type   = BLE_GATT_HVX_NOTIFICATION;
      params.handle = value_handle;
      params.p_data = (uint8_t*)p_data;
      params.p_len  = &length;

      return sd_ble_gatts_hvx(conn_handle, &params);
    }
  }
  ASSERT(false);  //unknown UUID
}

