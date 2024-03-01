#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <stdint.h>
#include <stddef.h>

void EVQ_Init(void);

void EVQ_Startup(void);

uint16_t EVQ_GetEventsAmount(void);

uint64_t EVQ_GetCurrentEventTimestamp(void);

uint32_t EVQ_GetEvt(void);

#endif	// EVENT_QUEUE_H