#include <sdk_common.h>
#include <nrf_assert.h>
#include "app_error.h"
#include "esm_lib.h"

#define NRF_LOG_MODULE_NAME     "esm_lib"
#define NRF_LOG_LEVEL           4
#include "nrf_log.h"


//----------------------------------------------------------------------
//      PRIVATE FUNCTIONS
//----------------------------------------------------------------------
static const char *getNameFromState(const ESM_t *inst, uint16_t _state)
{
  ASSERT(_state < inst->total_states);
  return (inst->p_states[_state].nameState);
}


//----------------------------------------------------------------------
//      PUBLIC FUNCTIONS
//----------------------------------------------------------------------
uint16_t esmGetState(ESM_ctx_t *ctx)
{
  return (ctx->state);
}

//-----------------------------------------------------------------------------   
ret_code_t esmEnable(const ESM_t *inst, bool en, ESM_ctx_t *ctx)
{
  if (inst == NULL || ctx == NULL)
    return NRF_ERROR_NULL;

  if (en)
  {
  // check order of states describe. The order must be 0,1,2,3.. without jumps and back.
  uint16_t  rightOrderState = 0;
  for (uint16_t i=0; i<inst->total_states; i++)
    if (inst->p_states[i].state != rightOrderState)
    {
      if (ctx->logLevel >= NRF_LOG_LEVEL_ERROR)
        NRF_LOG_ERROR("%s: ESM[%s] Element[%d] has jump. Reorder state table\n", (uint32_t)__func__, (uint32_t)inst->esm_name, i);

      ASSERT(false);
    }
    else
      ++rightOrderState;

    for (uint16_t i=0; i<inst->total_states; i++)
    {
      if (inst->p_states[i].total_signals == 0 || inst->p_states[i].exe_func == NULL)
        // check parameter for macro ESM_STATES_DEF, or/and count of p_signals
        return NRF_ERROR_INVALID_DATA;
  
      for (uint16_t j=0; j<inst->p_states[i].total_signals; j++)
        if (inst->p_states[i].p_signals[j].signal == 0)
          //check parameter for macro ESM_SIGNALS_DEF. Signal must be more than 0 (NO_ACTION) 
          return NRF_ERROR_NOT_FOUND;
    }
  }
  else
    ctx->state = 0;

  ctx->isInit = en;
  if (ctx->logLevel >= NRF_LOG_LEVEL_DEBUG)
    NRF_LOG_DEBUG("%s: ESM[%s] in %s\n", (uint32_t)__func__,
                    (uint32_t)inst->esm_name, (uint32_t)(en ? "Enabled" : "Disabled"));

  return NRF_SUCCESS;
}


//-----------------------------------------------------------------------------
bool esmProcess(const ESM_t *inst, ESM_ctx_t *ctx)
{
  bool retval = false;
  if (ctx->isInit)
  {
    uint16_t active_state = ctx->state;
    uint16_t i, signal;
       
    signal = inst->p_states[active_state].exe_func(ctx->user_ctx);

    if (signal != 0)
    {
      for (i=0; i<inst->p_states[active_state].total_signals; i++)
      {
        if (inst->p_states[active_state].p_signals[i].signal == signal)
        {
          uint16_t new_state = inst->p_states[active_state].p_signals[i].toState;

          if (ctx->logLevel >= NRF_LOG_LEVEL_DEBUG)
            NRF_LOG_DEBUG("%s: ESM[%s] sign %d, %s -> %s\n", (uint32_t)__func__, (uint32_t)inst->esm_name, signal,
                (uint32_t)getNameFromState(inst, active_state),
                (uint32_t)getNameFromState(inst, new_state));

          if (inst->p_states[active_state].p_signals[i].f_jump)
            inst->p_states[active_state].p_signals[i].f_jump(ctx->user_ctx);

          ctx->state = new_state;
          retval = true;

          break;
        }
      }
      // check: action for a new state should be described 
      if (i == inst->p_states[active_state].total_signals)
      {
        if (ctx->logLevel >= NRF_LOG_LEVEL_ERROR)
          NRF_LOG_ERROR("%s: ESM[%s] For state %d signal %d not described\n",
                          (uint32_t)__func__, (uint32_t)inst->esm_name, active_state, signal);
        ASSERT(false);
      }
    }
  }
  return retval;
}
