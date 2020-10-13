/**
 * Copyright (c) 2015 - 2017, Nordic Semiconductor ASA
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 * 
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 * 
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

/** @file
 *
 * @defgroup modif_ble_ios Input Output Server
 * @{
 * @ingroup ble_sdk_srv
 *
 * @brief Input Output Service Server module. It created on base LED button Service
 *
 * @details This module implements a custom IO Service with an input and output Characteristics.
 *          During initialization, the module adds the IO Service and Characteristics
 *          to the BLE stack database.
 *
 *          The application must supply an event handler for receiving IO Service
 *          events. Using this handler, the service notifies the application when the
 *          Input value changes.
 *
 *          The service also provides a function for letting the application notify
 *          the state of the Output Characteristic to connected peers.
 *
 * @note The application must propagate BLE stack events to the IO Service
 *       module by calling ble_ios_on_ble_evt() from the @ref softdevice_handler callback.
*/

#ifndef BLE_IOS_H__
#define BLE_IOS_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LBS_UUID_BASE        {0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, \
                              0xDE, 0xEF, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00}
#define LBS_UUID_SERVICE     0x1523
#define LBS_UUID_BUTTON_CHAR 0x1524
#define LBS_UUID_LED_CHAR    0x1525
// The length of created characteristics
#define INPUT_CHAR_LEN        2
#define INPUT_CHAR_LEN_MAX    2
#define OUTPUT_CHAR_LEN       6
#define OUTPUT_CHAR_LEN_MAX   6

// Forward declaration of the ble_ios_t type.
typedef struct ble_ios_s ble_ios_t;

typedef void (*ble_ios_input_handler_t) (ble_ios_t * p_ios, void * p_input_incoming);

/**@brief Input Output Service structure. This structure contains various status information for the service. */
struct ble_ios_s
{
    uint16_t                    service_handle;      /**< Handle of LED Button Service (as provided by the BLE stack). */
    ble_gatts_char_handles_t    led_char_handles;    /**< Handles related to the LED Characteristic. */
    ble_gatts_char_handles_t    button_char_handles; /**< Handles related to the Button Characteristic. */
    uint8_t                     uuid_type;           /**< UUID type for the LED Button Service. */
    uint16_t                    conn_handle;         /**< Handle of the current connection (as provided by the BLE stack). BLE_CONN_HANDLE_INVALID if not in a connection. */
    ble_ios_input_handler_t     ios_input_handler;   /**< Event handler to be called when the LED Characteristic is written. */
};


/**@brief Function for initializing the Input Output Service.
 *
 * @param[out] p_ios          IO Service structure. This structure must be supplied by
 *                            the application. It is initialized by this function and will later
 *                            be used to identify this particular service instance.
 * @param[out] input_handler  Poionter to handler Input Event.
 *
 * @retval NRF_SUCCESS If the service was initialized successfully. Otherwise, an error code is returned.
 */
uint32_t ble_ios_init(ble_ios_t * p_ios, ble_ios_input_handler_t input_handler);


/**@brief Function for handling the application's BLE stack events.
 *
 * @details This function handles all events from the BLE stack that are of interest to the LED Button Service.
 *
 * @param[in] p_ios      IO Service structure.
 * @param[in] p_ble_evt  Event received from the BLE stack.
 */
void ble_ios_on_ble_evt(ble_ios_t * p_ios, ble_evt_t * p_ble_evt);


/**@brief Function for sending a Output notification.
 *
 * @param[in] p_ios      IO Service structure.
 * @param[in] button_state  New button state.
 *
 * @retval NRF_SUCCESS If the notification was sent successfully. Otherwise, an error code is returned.
 */
uint32_t ble_ios_on_output_change(ble_ios_t * p_ios, void * p_value_to_output, uint16_t outputSize);


#ifdef __cplusplus
}
#endif

#endif // BLE_IOS_H__

/** @} */
