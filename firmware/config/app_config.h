#ifndef SYS_CONFIG_H
#define SYS_CONFIG_H
#include "pinmap.h"
// <<< Use Configuration Wizard in Context Menu >>>\n

//==========================================================
// <q> DISABLE_SOFTDEVICE  - Disable SoftDevice (uses for a step by step debuging)
#ifndef DISABLE_SOFTDEVICE
#define DISABLE_SOFTDEVICE 0
#endif

//==========================================================

// <e> CPU_USAGE_MONITOR - Enables CPU usage & time profiler module.
#ifndef CPU_USAGE_MONITOR
#define CPU_USAGE_MONITOR 0
#endif

#if CPU_USAGE_MONITOR

// <o> CPU_USAGE_REPORT_PERIOD_MS - Reporet period <0-5000>
#ifndef CPU_USAGE_REPORT_PERIOD_MS
#define CPU_USAGE_REPORT_PERIOD_MS 1000
#endif

#endif // CPU_USAGE_MONITOR
// </e>


//==========================================================
// <e> USE_STATIC_PASSKEY  - Use 6-Digit static passkey
#ifndef USE_STATIC_PASSKEY
#define USE_STATIC_PASSKEY 1
#endif

#if USE_STATIC_PASSKEY

// <s> STATIC_PASSKEY - Static passkey to device access
#ifndef STATIC_PASSKEY
#define STATIC_PASSKEY "123456"
#endif

#endif // USE_STATIC_PASSKEY
// </e>


// <<< end of configuration section >>>
#endif  // SYS_CONFIG_H