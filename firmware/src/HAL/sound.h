#ifndef SOUND_H
#define SOUND_H

typedef struct
{
  uint32_t duration;
  uint16_t  freq;
} sound_note_t;

void sound_Init(void);

void sound_Startup(void);

void sound_hello(void);
void sound_alarm(void);
void sound_danger(void);

#endif //SOUND_H