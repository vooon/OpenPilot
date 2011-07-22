
#ifndef PIOS_SIM_PRIV_H
#define PIOS_SIM_PRIV_H

/**
 * State of inputs and outputs to the simulation model
 */
struct pios_sim_state {
	float accels[3];
	float gyros[3];
	float mag[3];
	float baro;
	float q[4];
	float position[3];
	int actuator[8];
};

#endif /* PIOS_SIM_PRIV */
