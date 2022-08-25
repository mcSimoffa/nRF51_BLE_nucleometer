#ifndef PIN_MAP_H
#define PIN_MAP_H

#define PUMP_HV_PIN                   05 
#define PUMP_HV_PIN_DRV               11
#define PUMP_HV_PIN_INACTIVE_STATE    NRF_GPIOTE_INITIAL_VALUE_LOW

#define DCRG                          04
#define GREEN_LED_PIN                 19
#define ORANGE_LED_PIN                15

#define PULSE_PIN                     02

#define BUTTON_PIN                    17
#define BUTTONS                       {BUTTON_PIN}
#define BUTTONS_ACTIVE_LEVEL          {0}
#define BUTTONS_PULL                  {NRF_GPIO_PIN_PULLUP}

#define BUZZER_PIN                    06

#endif // PIN_MAP_H