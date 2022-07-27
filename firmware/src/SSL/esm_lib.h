#ifndef ESM_LIB_H__
#define ESM_LIB_H__
#include <stdint.h>
#include <stdbool.h>
#include "sdk_errors.h"
#include "nordic_common.h"

/*!
  \brief Macro to create one state with jump logic to another state
         of static allocate end state mashine
  \param [in] _state - a state wjhich is describes
  \param [in] _exe_func - pointer to f_proc_t funcrion. This funcrion executes untin this state is sctive
  \param [in] _signals - total amount of signals. Each signal mean one possible jumo to another state
  Next all signals should be described as an array:
  of element like {SIGNAL, NEW_STATE, jump function}
  If f_p;roc function return some SIGNAL then ESM move to NEW_STATE through jump function
  */
#define ESM_STATE_DEF(_state, _str_state, _exe_func, _signals)  \
  .state = _state,                                              \
  .exe_func = _exe_func,                                        \
  .nameState = _str_state,                                      \
  .total_signals = _signals,                                    \
  .p_signals =                                                  \
  (const ESM_signal_t*)&(const ESM_signal_t[_signals])


/*!
  \brief Macro to fill a common parameters of static allocate end state mashine
  \param [in] _states - total amount of states. All explicit states should be described
                        like array of ESM_STATE_DEF
  */
#define ESM_DEF(_states, _name)                     \
  .esm_name = _name,                                \
  .total_states = _states,                          \
  .p_states =                                       \
  (const ESM_state_t*)&(const ESM_state_t[_states])


// context for ESM. One ESM (unified logic) can have many contexts
typedef struct
{
  void      *user_ctx;  //User context
  uint16_t  state;
  bool      isInit;
  uint8_t   logLevel;   //0=None, 1=Error, 2=Warn, 3=Info, 4=Debug
} ESM_ctx_t;

typedef void      (* f_jump_t)(void *user_ctx);   //type of jump function from state to new state
typedef uint16_t  (* f_proc_t)(void *user_ctx);   //type of process function (execute when certain state is active)

typedef struct
{
  uint16_t  signal;
  uint16_t  toState;
  f_jump_t  f_jump;
} ESM_signal_t;

typedef struct
{
  uint16_t            state;
  f_proc_t            exe_func;
  char                *nameState;
  uint16_t            total_signals;
  const ESM_signal_t  *p_signals; 
} ESM_state_t;


typedef struct
{
  char              *esm_name;
  uint16_t          total_states; 
  const ESM_state_t *p_states;
} ESM_t;


/*!
  \brief End state machine process function
  It's a driver of end state machine. You need execute this function in main context
  \param [in] inst end state machine instance
  \return true - if this esm changed own state
          false - if the state keeps previous
  */
bool esmProcess(const ESM_t *inst, ESM_ctx_t *ctx);


/*!
  \brief End state machine enable
         End state mashine is blocked until this function unblock ESM. It also can block ESM. 
  \param [in] inst  - end state machine instance
  \param [in] en    - true - enable. false- disable ESM
  \param [in] ctx   - context. This pointer will be passed to f_jump and f_proc functions

  \return NRF_SUCCESS - if OK
          NRF_ERROR_INVALID_DATA occurs if ESM_STATES_DEF(wrong), ESM_SIGNALS_DEF(0), exe function is NULL
          NRF_ERROR_NOT_FOUND occurs if ESM_SIGNALS_DEF(wrong), jump function is NULL  
  */
ret_code_t esmEnable(const ESM_t *inst, bool en, ESM_ctx_t *ctx);


/*!
  \brief Get state of End state machine
  \param [in] ctx end state machine context
  \return current state
  */
uint16_t esmGetState(ESM_ctx_t *ctx);

#endif	// ESM_LIB_H__