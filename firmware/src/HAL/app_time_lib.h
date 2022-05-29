#ifndef APP_TIME_LIB_H
#define APP_TIME_LIB_H

#include <stdint.h>
#include <app_timer.h>

#define APP_TIMER_PRESCALER             0   // Value of the RTC1 PRESCALER register.
#define MS_TO_TICK(ms)                  APP_TIMER_TICKS(ms, APP_TIMER_PRESCALER)
typedef struct
{
  uint64_t  sec;  // number of seconds
  uint32_t  ms;   // number of milliseconds (1/1000 s)
  uint32_t  frac; // or fractions (1/32768 s)
} timestr_t;


/*! ---------------------------------------------------------------------------
  \brief Initializes 64-bit time library based on app_timer
*/
void app_time_Init(void);


/*! ---------------------------------------------------------------------------
  \brief Start 64-bit time library based on app_timer
*/
void app_time_Startup(void);


/*! ---------------------------------------------------------------------------
  \brief Check is UTC time was enabled

  \return true - UTC time is enabled
  \return false - UTC time is disabled (invoke app_time_Set_UTC() to set UTC and enable)
*/
bool app_time_Is_UTC_en(void);


/*! ---------------------------------------------------------------------------
  \brief Setting UTC time
  \param now_ticks[in] - 64 bit tick value (in 1/32768) of user time (UTC) for now

  \return true - UTC time was set successful
  \return false - UTC time set fail (zero now_ticks)
*/
bool app_time_Set_UTC(uint64_t now_ticks);


/*! ---------------------------------------------------------------------------
  \brief Getting system time in 1/32768 ticks

  \return system time from system start
*/
uint64_t app_time_Get_sys_time(void);


/*! ---------------------------------------------------------------------------
  \brief Getting user time (UTC) in 1/32768 ticks

  \return user time from system start
*/
uint64_t app_time_Get_UTC(void);


void app_time_Ticks_to_struct(uint64_t ticks, timestr_t *timestr);

#endif // APP_TIME_LIB_H
