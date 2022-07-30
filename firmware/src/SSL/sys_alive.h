#ifndef SYS_ALIVE_H
#define SYS_ALIVE_H
#include <stdbool.h>
/*!
 * \brief This API needs to prevent suddenly sleep until all events not handled
 * When some event occurs at the end of main loop application needs to one more main cycle to habdle this event
 */
 
/*!
 * \brief Postpone enter to light-sleep mode for one main loop pass.
 * A good way to invoke this function always when any FSM changed own state
 * or one or more events occurs. Because application must handled it before light sleep enter
 */
void sleepLock(void);

/*!
 * \brief Check light-sleep lock status
 * Light sleep enter should only if return false.
 */
bool sleepLock_check(void);

#endif // SYS_ALIVE_H
