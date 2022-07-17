#ifndef PARTICLE_CNT_H
#define PARTICLE_CNT_HH

/*! ---------------------------------------------------------------------------
  \brief Particle counter module init 
 ----------------------------------------------------------------------------*/
void particle_cnt_Init();

/*! ---------------------------------------------------------------------------
  \brief Particle counter module start
  \details module uses one GPIO and one timer to count pulse from geiger tube
           One app_timer instancr uses to polling counter timer and getting a current pulse value
  
 ----------------------------------------------------------------------------*/
void particle_cnt_Startup();

#endif	// PARTICLE_CNT_H