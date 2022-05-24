/*!
 * \file
 * \brief Real Time Clock Driver
 * \copyright Copyright (c) 2021 Benchmark Electronics
 *
 * \addtogroup HAL
 * \{
 * \addtogroup HAL_RTC RTC
 * \brief Real Time Clock Driver
 * \{
 */

#ifndef RTC_H
#define RTC_H

/* INCLUDES *********************************************************************************************************************/
#include <stdint.h>
#include <app_timer.h>

/* DEFINITIONS ******************************************************************************************************************/

/*!
 * \brief Converts milliseconds to ticks.
 */
#define MSECS_TO_TICKS(ms)      ((uint32_t)((((uint64_t)APP_TIMER_CLOCK_FREQ * (ms)) + 500) / 1000))
/*!
 * \brief Converts ticks to milliseconds.
 */
#define TICKS_TO_MSECS(ticks)   ((uint32_t)(((uint64_t)(ticks) * 1000) / APP_TIMER_CLOCK_FREQ))


/* GLOBAL VARIABLES *************************************************************************************************************/

/* FUNCTIONS ********************************************************************************************************************/

/*!
 * \brief Initializes the real time clock driver
 */
void RTC_Init(void);

/*!
 * \brief Startup the real time clock driver
 */
void RTC_Startup(void);

/*!
 * \brief Backup current time
 *
 * \param addTicks
 *    Additional ticks
 *
 * \note
 *    Backup time is used to initialize time after unexpected reset (e.g. assert)
 */
void RTC_Backup(uint32_t addTicks);

/*!
 * \brief Check is user time is enabled
 *
 * \return
 *    True if enabled; otherwise false
 */
bool RTC_IsUsrTimeEnabled(void);

/*!
 * \brief Set user time.

 * \param time
 *    Time to set
 *
 *  \note
 *    If parameter time is zero, the timer will be disabled and time will be set to zero
 */
void RTC_SetUsrTimeInSeconds(uint32_t time);

/*!
 * \brief Get user time
 *
 * \return
 *    User time in seconds
 */
uint32_t RTC_GetUsrTimeInSeconds(void);

/*!
 * \brief Get user time
 *
 * \return
 *    User time in milliseconds
 */
uint32_t RTC_GetUsrTimeInMilliSeconds(void);

/*!
 * \brief Get user time
 *
 * \return
 *    User time in ticks
 */
uint64_t RTC_GetUsrTimeInTicks(void);

/*!
 * \brief Get system time
 *
 * \return
 *    System time in seconds
 */
uint32_t RTC_GetSysTimeInSeconds(void);

/*!
 * \brief Get system time
 *
 * \return
 *    System time in milliseconds
 */
uint32_t RTC_GetSysTimeInMilliSeconds(void);

/*!
 * \brief Get system time
 *
 * \return
 *    System time in ticks
 */
uint64_t RTC_GetSysTimeInTicks(void);

/*!
 * \brief Get system time
 *
 * \return
 *    System time in ticks
 */
uint32_t RTC_GetTick(void);

/*!
 * \brief Get seconds part of timestamp
 *
 * \param ticks
 *    Timestamp in ticks
 *
 * \return
 *    Seconds part in seconds.
 */
uint32_t RTC_GetSeconds(uint64_t ticks);

/*!
 * \brief Get fraction part of timestamp
 *
 * \param ticks
 *    Timestamp in ticks
 *
 * \return
 *    Fraction part in 1/256 s
 */
uint8_t RTC_GetFraction(uint64_t ticks);

/*!
 * \brief Convert time from milliseconds to ticks
 *
 * \param ms
 *    Time in milliseconds
 *
 * \return
 *    Time in ticks
 */
uint32_t RTC_GetTickms(uint32_t ms);

/*!
 * \brief Convert time from ticks to milliseconds
 *
 * \param tick
 *    Time in ticks
 *
 * \return
 *    Time in milliseconds
 */
uint32_t RTC_GetmsTick(uint32_t tick);

/*!
 * \brief Get difference between reference and current time
 *
 * \param refTick
 *    Reference time in ticks
 *
 * \return
 *    Difference time in ticks
 */
uint32_t RTC_GetDiffTick(uint32_t refTick);

/*!
 * \brief Calculate time difference
 *
 * \param t1
 *    First time in ticks
 *
 * \param t2
 *    Second time in tick
 *
 * \return
 *    Difference between t1 and t2 in ticks
 */
uint32_t RTC_CalcTickDiff(uint32_t t1, uint32_t t2);

#endif // RTC_H

/*!
 * \}
 * \}
 */
