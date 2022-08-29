#ifndef PARTICLE_WATCHER_H
#define PARTICLE_WATCHER_H

void PWT_Init(void);

void PWT_Startup(void);

void PWT_Process(void);

uint32_t  PWT_GetBarVol();

void PWT_Set_active_tf(uint8_t tf);

#endif	// PARTICLE_WATCHER_H