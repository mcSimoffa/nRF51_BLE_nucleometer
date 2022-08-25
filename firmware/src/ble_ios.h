#ifndef BLE_IOS_H__
#define BLE_IOS_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"

#ifdef __cplusplus
extern "C" {
#endif

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

typedef void (*ble_ios_wr_handler_t) (uint16_t conn_handle, uint16_t datalen, uint8_t *p_data);

typedef void (*ble_ios_rd_handler_t) (uint16_t conn_handle);

/*!
 * \brief Characteristic length description
 */
typedef struct
{
  uint16_t init;      ///<  Initial length of the characteristic value.
  uint16_t max;       ///<  Maximum length of the characteristic value.
  bool     var;       ///<  Indicates if the characteristic value has variable length.
} ble_char_len_t;

/*!
 * \brief Characteristic description
 */
typedef struct
{
  uint16_t              uuid;                 // The UUID of the characteristic
  ble_char_len_t        len;                  // The length for the value of this characteristic
  ble_gatt_char_props_t prop;                 // The properties for this characteristic (CHAR_PROP_WRITE, CHAR_PROP_READ, CHAR_PROP_NOTIFY).
  security_req_t        rd_access;            // Security requirement for reading the characteristic value.
  security_req_t        wr_access;            // Security requirement for writing the characteristic value.
  security_req_t        cccd_wr_access;       // Security requirement for writing the characteristic's CCCD.
  ble_ios_rd_handler_t  rdCb;                 // Callback on authorize read action
  ble_ios_wr_handler_t  wrCb;                 // Callback on write action
  bool                  is_defered_read;      // The defered read properties cause to BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST event when char is read
} char_desc_t;


typedef struct
{
  uint16_t                    *service_handle;       // Handle of Input Output Service (as provided by the BLE stack).
  const uuid_128_t            *p_base_uuid;
  uint16_t                    service_uuid;
  const char_desc_t           *char_list;
  uint8_t                     chars_total;
  ble_gatts_char_handles_t    *char_handles;
} ble_ios_t;


/*! --------------------------------------------------------------------------
 * \brief   Macro for defining a ble_lbs instance.
 *
 * \param   _name   Name of the instance.
 */
#define BLE_IOS_DEF(_name, _base_uuid, _service_uuid, _char_list, _chars_total)             \
static const ble_ios_t _name =                                                              \
{                                                                                           \
  .service_handle = (uint16_t*)&(uint16_t){0},                                              \
  .p_base_uuid = _base_uuid,                                                                \
  .service_uuid = _service_uuid,                                                            \
  .char_list = _char_list,                                                                  \
  .chars_total = _chars_total,                                                              \
  .char_handles = (ble_gatts_char_handles_t*)&(ble_gatts_char_handles_t[_chars_total]){0},  \
};


/*! ---------------------------------------------------------------------------
 * \brief Function for initializing the Input Output Service.
 *
 * \param[out] p_ios      Input Output Service structure.

 * \retval NRF_SUCCESS If the service was initialized successfully. Otherwise, an error code is returned.
 */
void ble_ios_init(const ble_ios_t * p_ios);


/*! ---------------------------------------------------------------------------
 * \brief Function for handling the application's BLE stack events.
 *
 * \details This function handles all events from the BLE stack that are of interest to the Input Output Service.
 *
 * \param[in] p_ble_evt  Event received from the BLE stack.
 * \param[in] p_context  Input Output Service structure.
 */
void ble_ios_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);


/*! ---------------------------------------------------------------------------
 * \brief Function for sending a output data to Central with setting characteristic value.
 *
 ' \param[in] conn_handle   Handle of the peripheral connection to which the output data will be sent.
 * \param[in] p_data        pointer to new output data for send.
 * \param[in] len           length of data.
 *
 * \retval NRF_SUCCESS If the char value was set successfully.
 * \retval NRF_ERROR_BUSY if the previous operation isn't completed
 */
ret_code_t ble_ios_rd_reply(uint16_t conn_handle, void *p_data, uint16_t len);


/*! ---------------------------------------------------------------------------
 * \brief Function for sending a output data to Central with notification.
 *
 ' \param[in] conn_handle   Handle of the peripheral connection to which the output data will be notification.
 * \param[in] p_ios         Input Output Service structure.
 * \param[in] uuid          UUID for charachteristic to data notification
 * \param[in] p_data        pointer to new output data for send.
 * \param[in] len           length of data.
 *
 * \retval NRF_SUCCESS If the notification was sent successfully. Otherwise, an error code is returned.
 */
ret_code_t ble_ios_on_output_change(uint16_t conn_handle, const ble_ios_t *p_ios, uint16_t uuid, void *p_data, uint8_t len);


ret_code_t ble_ios_output_set(uint16_t conn_handle, const ble_ios_t *p_ios, uint16_t uuid, void *p_data, uint8_t len);


#ifdef __cplusplus
}
#endif

#endif // BLE_IOS_H__

