#ifndef NRF_GPIO_ADDS_H__
#define NRF_GPIO_ADDS_H__

#include <nrf_gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function for configuring the given GPIO pin number as output, hiding inner details.
 *        This function can be used to configure a pin as simple output with gate driving GPIO_PIN_CNF_DRIVE_H0H1 (strong cases).
 *
 * @param pin_number Specifies the pin number.
 *
 * @note  Sense capability on the pin is disabled and input is disconnected from the buffer as the pins are configured as output.
 */
__STATIC_INLINE void nrf_gpio_cfg_strong_output(uint32_t pin_number);


__STATIC_INLINE void nrf_gpio_cfg_strong_output(uint32_t pin_number)
{
    nrf_gpio_cfg(
        pin_number,
        NRF_GPIO_PIN_DIR_OUTPUT,
        NRF_GPIO_PIN_INPUT_DISCONNECT,
        NRF_GPIO_PIN_NOPULL,
        NRF_GPIO_PIN_H0H1,
        NRF_GPIO_PIN_NOSENSE);
}



#ifdef __cplusplus
}
#endif

#endif // NRF_GPIO_ADDS_H__
