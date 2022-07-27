#ifndef BUTTON_H
#define BUTTON_H

#include "section_vars.h"

typedef struct
{
  union
  {
    struct
    {
      uint8_t   pressed:1;
      uint8_t   button_num:7;
    } field;
    uint8_t     num;
  };
} button_event_t;

/*!\brief button event callback.
 * \param[in] event   Type of button event.
 */
typedef void (*button_handler_t)(button_event_t event);


/*!\brief   Macro for registering a button module callback.
 *          Applications which use button API must register with the module using this macro.
 
 * \details This macro places the callback variable in a section named "button_handlers"
 *
 * \param[in]   hndl     callback pointer.
 */
#define BUTTON_REGISTER_HANDLER(hndl) NRF_SECTION_VARS_REGISTER_VAR(button_handlers, button_handler_t const hndl)


void button_Init(void);
void button_Startup(void);
void button_Process(void);
uint8_t button_IsPressed(uint8_t n_btn);

#endif