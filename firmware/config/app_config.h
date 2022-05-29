#ifndef SYS_CONFIG_H
#define SYS_CONFIG_H
#include "pinmap.h"
// <<< Use Configuration Wizard in Context Menu >>>\n

//==========================================================
// <q> DISABLE_SOFTDEVICE  - Disable SoftDevice (uses for a step by step debuging)
#ifndef DISABLE_SOFTDEVICE
#define DISABLE_SOFTDEVICE
#endif

//==========================================================

// <h> CPU_USAGE_MONITOR - Enables CPU usage & time profiler module.
#ifndef CPU_USAGE_MONITOR
#define CPU_USAGE_MONITOR 1
#endif

#if CPU_USAGE_MONITOR

// <o> CPU_USAGE_REPORT_PERIOD_MS - Reporet period <0-5000>
#ifndef CPU_USAGE_REPORT_PERIOD_MS
#define CPU_USAGE_REPORT_PERIOD_MS 1000
#endif

#endif // CPU_USAGE_MONITOR
// </h>
// <<< end of configuration section >>>
#endif  // SYS_CONFIG_H