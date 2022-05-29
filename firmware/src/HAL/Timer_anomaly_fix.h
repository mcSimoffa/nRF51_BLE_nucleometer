#ifndef TIMER_ANOMALY_FIX_H__
#define TIMER_ANOMALY_FIX_H__

#include "nrf_drv_timer.h"

/*! ---------------------------------------------------------------------------
  \brief Fix anomaly 73
  \details this function should be invoked before timer start (eith op=1)
            and after timer stop (with op=0)

  \param base[in-out] - base address
  \param op[in] - operation tyoe: 1- before enabled, 0 -after disabled
*/
void timer_anomaly_fix(NRF_TIMER_Type *base,  uint8_t op);

#endif // TIMER_ANOMALY_FIX_H__
