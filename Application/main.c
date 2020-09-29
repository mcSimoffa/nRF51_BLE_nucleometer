/*********************************************************************
*                    SEGGER Microcontroller GmbH                     *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*            (c) 2014 - 2020 SEGGER Microcontroller GmbH             *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
* All rights reserved.                                               *
*                                                                    *
* Redistribution and use in source and binary forms, with or         *
* without modification, are permitted provided that the following    *
* condition is met:                                                  *
*                                                                    *
* - Redistributions of source code must retain the above copyright   *
*   notice, this condition and the following disclaimer.             *
*                                                                    *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             *
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,        *
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           *
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
* DISCLAIMED. IN NO EVENT SHALL SEGGER Microcontroller BE LIABLE FOR *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  *
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;    *
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF      *
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT          *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE  *
* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
* DAMAGE.                                                            *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------
*/
#include <stdio.h>
#include <stdlib.h>
#include "boards.h"
#include "bsp.h"
#include "custom_board.h"
#include "nrf.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "softdevice_handler.h"
#include "app_timer.h"
#include "ble.h"
#include "softdevice_handler.h"
#include "ble_advdata.h"

//#include "nrf_delay.h"
//#include "nrf_gpio.h"
//#include "nrf_drv_gpiote.h"

/* *******************************************************************
                    Function prototypes
******************************************************************** */
static void ble_stack_init(void);
static void advertising_init(void);

/* *******************************************************************
                    DEFINITIONs
******************************************************************** */
//#define DEAD_BEEF 0xDEADBEEF      // Value used as error code on stack dump, can be used to identify stack location on stack unwind.
#define APP_TIMER_PRESCALER     0   // Value of the RTC1 PRESCALER register.
#define APP_TIMER_OP_QUEUE_SIZE 4   // Size of timer operation queues
#define CENTRAL_LINK_COUNT      0   // Number of central links used by the application. When changing this number remember to adjust the RAM settings
#define PERIPHERAL_LINK_COUNT   0   // Number of peripheral links used by the application. When changing this number remember to adjust the RAM settings
//in global defines needs BLE_STACK_SUPPORT_REQD https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk5.v12.3.0%2Flib_softdevice_handler.html
#define APP_COMPANY_IDENTIFIER          0x0059   // Company identifier for Nordic Semiconductor ASA. as per www.bluetooth.org.

#define APP_BEACON_INFO_LENGTH          23      // Total length of information advertised by the Beacon.
#define APP_DEVICE_TYPE                 0x02    // 0x02 refers to Beacon.
#define APP_ADV_DATA_LENGTH             21      //Length of manufacturer specific data in the advertisement.
#define APP_BEACON_UUID                 0x01, 0x12, 0x23, 0x34, \
                                        0x45, 0x56, 0x67, 0x78, \
                                        0x89, 0x9a, 0xab, 0xbc, \
                                        0xcd, 0xde, 0xef, 0xf0    // Proprietary UUID for Beacon.

#define APP_MAJOR_VALUE                 0x01, 0x02    // Major value used to identify Beacons.
#define APP_MINOR_VALUE                 0x03, 0x04    // Minor value used to identify Beacons.
#define APP_MEASURED_RSSI               0xC3          // The Beacon's measured RSSI at 1 meter distance in dBm.
#define NON_CONNECTABLE_ADV_INTERVAL    MSEC_TO_UNITS(100, 625) // The advertising interval for non-connectable advertisement (100 ms). This value can vary between 100ms to 10.24s). Step 625 uS
#define APP_CFG_NON_CONN_ADV_TIMEOUT    0                                 /**< Time for which the device must be advertising in non-connectable mode (in seconds). 0 disables timeout. */


/* *******************************************************************
                    Global Variable
******************************************************************** */
static uint8_t m_beacon_info[APP_BEACON_INFO_LENGTH] =   //Information advertised by the Beacon.
{
    APP_DEVICE_TYPE,     // Manufacturer specific information. Specifies the device type in this
                         // implementation.
    APP_ADV_DATA_LENGTH, // Manufacturer specific information. Specifies the length of the
                         // manufacturer specific data in this implementation.
    APP_BEACON_UUID,     // 128 bit UUID value.
    APP_MAJOR_VALUE,     // Major arbitrary value that can be used to distinguish between Beacons.
    APP_MINOR_VALUE,     // Minor arbitrary value that can be used to distinguish between Beacons.
    APP_MEASURED_RSSI    // Manufacturer specific information. The Beacon's measured TX power in
                         // this implementation.
};

static ble_gap_adv_params_t m_adv_params;   // Parameters to be passed to the stack when starting advertising.

/* *******************************************************************
                    MAIN program cycle
******************************************************************** */
int main(void) 
{ //it here from C:\Program Files\SEGGER\SEGGER Embedded Studio for ARM 5.10a\source\thumb_crt0.s
  uint32_t err_code;
  // Initialize.
  err_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(err_code);


  APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false); //why this timer needs ?
  err_code = bsp_init(BSP_INIT_LED, APP_TIMER_TICKS(100, APP_TIMER_PRESCALER), NULL); //custom_board.h
  APP_ERROR_CHECK(err_code);
  ble_stack_init();
  advertising_init();
  for (;; )
    {
        if (NRF_LOG_PROCESS() == false)
        {
            //power_manage();
        }
    }
}




/* ******************************************************************
Initializes the SoftDevice and the BLE event interrupt.
https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk51.v10.0.0%2Fgroup__peer__manager.html
SoftDevice use a resource:
https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.s130.sds%2Fdita%2Fsoftdevices%2Fs130%2Fsd_resource_reqs%2Fhw_block_interrupt_vector.html&anchor=hw_block_interrupt_vector
interrupt
https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.s130.sds%2Fdita%2Fsoftdevices%2Fs130%2Fprocessor_avail_interrupt_latency%2Finterrupt_forwarding_to_application.html&anchor=interrupt_forwarding_to_application

******************************************************************** */
static void ble_stack_init(void) 
{
  uint32_t err_code;
  nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC; //this struct define clock source for Softdevice

  // Initialize the SoftDevice handler module.
  
  SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);

  ble_enable_params_t ble_enable_params; // ble.h
  err_code = softdevice_enable_get_default_config(CENTRAL_LINK_COUNT,
      PERIPHERAL_LINK_COUNT,
      &ble_enable_params);  //default param (it possible manual tune also)
  APP_ERROR_CHECK(err_code);

  //Check the ram settings against the used number of links
  CHECK_RAM_START_ADDR(CENTRAL_LINK_COUNT, PERIPHERAL_LINK_COUNT);

  // Enable BLE stack.
  err_code = softdevice_enable(&ble_enable_params);
  APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the Advertising functionality.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
static void advertising_init(void)
{
    uint32_t      err_code;
    ble_advdata_t advdata;
    uint8_t       flags = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;

    ble_advdata_manuf_data_t manuf_specific_data;

    manuf_specific_data.company_identifier = APP_COMPANY_IDENTIFIER;

#if defined(USE_UICR_FOR_MAJ_MIN_VALUES)
    // If USE_UICR_FOR_MAJ_MIN_VALUES is defined, the major and minor values will be read from the
    // UICR instead of using the default values. The major and minor values obtained from the UICR
    // are encoded into advertising data in big endian order (MSB First).
    // To set the UICR used by this example to a desired value, write to the address 0x10001080
    // using the nrfjprog tool. The command to be used is as follows.
    // nrfjprog --snr <Segger-chip-Serial-Number> --memwr 0x10001080 --val <your major/minor value>
    // For example, for a major value and minor value of 0xabcd and 0x0102 respectively, the
    // the following command should be used.
    // nrfjprog --snr <Segger-chip-Serial-Number> --memwr 0x10001080 --val 0xabcd0102
    uint16_t major_value = ((*(uint32_t *)UICR_ADDRESS) & 0xFFFF0000) >> 16;
    uint16_t minor_value = ((*(uint32_t *)UICR_ADDRESS) & 0x0000FFFF);

    uint8_t index = MAJ_VAL_OFFSET_IN_BEACON_INFO;

    m_beacon_info[index++] = MSB_16(major_value);
    m_beacon_info[index++] = LSB_16(major_value);

    m_beacon_info[index++] = MSB_16(minor_value);
    m_beacon_info[index++] = LSB_16(minor_value);
#endif

    manuf_specific_data.data.p_data = (uint8_t *) m_beacon_info;
    manuf_specific_data.data.size   = APP_BEACON_INFO_LENGTH;

    // Build and set advertising data.
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type             = BLE_ADVDATA_NO_NAME;
    advdata.flags                 = flags;
    advdata.p_manuf_specific_data = &manuf_specific_data;

    err_code = ble_advdata_set(&advdata, NULL);
    APP_ERROR_CHECK(err_code);

    // Initialize advertising parameters (used when starting advertising).
    memset(&m_adv_params, 0, sizeof(m_adv_params));

    m_adv_params.type        = BLE_GAP_ADV_TYPE_ADV_NONCONN_IND;
    m_adv_params.p_peer_addr = NULL;                             // Undirected advertisement.
    m_adv_params.fp          = BLE_GAP_ADV_FP_ANY;
    m_adv_params.interval    = NON_CONNECTABLE_ADV_INTERVAL;
    m_adv_params.timeout     = APP_CFG_NON_CONN_ADV_TIMEOUT;
}