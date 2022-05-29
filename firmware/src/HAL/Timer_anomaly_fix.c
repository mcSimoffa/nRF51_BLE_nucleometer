/*! \link https://infocenter.nordicsemi.com/pdf/nRF51822_pan_v3.3.pdf?cp=5_4_1_0
anomaly 73
*/
#include "nrf_drv_timer.h"

void timer_anomaly_fix(NRF_TIMER_Type *base,  uint8_t op)
{
  uint32_t fixAddr = (uint32_t)base + 0x0C0C;
  *(uint32_t*)fixAddr = op;
}
