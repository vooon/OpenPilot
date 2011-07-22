
#ifndef PIOS_SIM_H
#define PIOS_SIM_H

int PIOS_SIM_Init();
int PIOS_SIM_Step(int dT);
void PIOS_SIM_SetActuator(int * actuator_int, int nchannels);
void PIOS_SIM_GetAccels(float *);
void PIOS_SIM_GetGyro(float *);
void PIOS_SIM_GetAttitude(float *);
void PIOS_SIM_GetPosition(float *);

#endif /* PIOS_SIM_H */
