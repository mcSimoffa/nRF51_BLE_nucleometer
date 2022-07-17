#include <sdk_common.h>
#include <nrf_assert.h>
#include "nrf_drv_ppi.h"
#include "nrf_drv_gpiote.h"

void common_drv_init(void)
{  
  ret_code_t err_code;
  if (!nrf_drv_gpiote_is_init())
  {
    err_code = nrf_drv_gpiote_init();
    ASSERT(err_code == NRF_SUCCESS);
  }

  err_code = nrf_drv_ppi_init();
  ASSERT(err_code == NRF_SUCCESS);
}