#ifndef SOUND_H
#define SOUND_H

typedef struct
{
  uint32_t duration;
  uint16_t  freq;
} sound_note_t;

void sound_Init(void);

void sound_Startup(void);

void sound_Start(sound_note_t *sequence, uint16_t seq_long);

#endif //SOUND_H