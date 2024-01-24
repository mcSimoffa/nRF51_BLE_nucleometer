#ifndef PIN_MAP_H
#define PIN_MAP_H

#ifdef SBM20

#define PUMP_HV_PIN                   5
#define PUMP_HV_PIN_DRV               11
#define PUMP_HV_PIN_INACTIVE_STATE    NRF_GPIOTE_INITIAL_VALUE_LOW

#define DCRG                          4
#define GREEN_LED_PIN                 19
#define ORANGE_LED_PIN                15

#define PULSE_PIN                     2

#define BUTTON_PIN                    17
#define BUTTONS                       {BUTTON_PIN}
#define BUTTONS_ACTIVE_LEVEL          {0}
#define BUTTONS_PULL                  {NRF_GPIO_PIN_PULLUP}

#define BUZZER_PIN_A                  1
#define BUZZER_PIN_B                  6

#define LPCOMP_INPUT_CHANNEL          NRF_LPCOMP_INPUT_4
#define LPCOMP_REFERENCE              NRF_LPCOMP_REF_EXT_REF0
#elif defined(J305)

#define PUMP_HV_PIN                   22
#define PUMP_HV_PIN_DRV               24
#define PUMP_HV_PIN_INACTIVE_STATE    NRF_GPIOTE_INITIAL_VALUE_LOW

#define DCRG                          4
#define GREEN_LED_PIN                 12
#define ORANGE_LED_PIN                14

#define PULSE_PIN                     5

#define BUTTON_PIN                    8
#define BUTTONS                       {BUTTON_PIN}
#define BUTTONS_ACTIVE_LEVEL          {0}
#define BUTTONS_PULL                  {NRF_GPIO_PIN_PULLUP}

#define BUZZER_PIN_A                  16
#define BUZZER_PIN_B                  6

#define LPCOMP_INPUT_CHANNEL          NRF_LPCOMP_INPUT_4
#define LPCOMP_REFERENCE              NRF_LPCOMP_REF_EXT_REF0

#endif

#endif // PIN_MAP_H