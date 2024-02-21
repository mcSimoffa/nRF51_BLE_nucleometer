#ifndef REALTIME_PARTICLE_WATCHER_H
#define REALTIME_PARTICLE_WATCHER_H

#include <stdint.h>
#include <stdbool.h>

void RPW_Init(void);
void RPW_Startup(void);
uint16_t RPW_GetInstant(void);

#endif	// REALTIME_PARTICLE_WATCHER_H