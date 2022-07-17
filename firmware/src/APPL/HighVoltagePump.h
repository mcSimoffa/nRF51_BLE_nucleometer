#ifndef HIGH_VOLTAGE_PUMP_H
#define HIGH_VOLTAGE_PUMP_H

/*! ---------------------------------------------------------------------------
  \brief High voltage module initialize 
 ----------------------------------------------------------------------------*/
void HV_pump_Init();

/*! ---------------------------------------------------------------------------
  \brief High voltage module start
  \details module generates ~360V to geiger counter tube supply
           two output GPIO use as a driver
           one output GPIO uses for overcharge preventing
           one intput GPIO uses as a voltage feedback
           one timer uses for DC-DC controlling
           one app_times instance uses for new HV pump initiate
 ----------------------------------------------------------------------------*/
void HV_pump_Startup(void);

#endif	// HIGH_VOLTAGE_PUMP_H