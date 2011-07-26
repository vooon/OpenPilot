#include "stdio.h"
#include "pios_sim_priv.h"
#include "sim_model.h"

int main() 
{
	struct pios_sim_state state = {
		.accels = {0, 0, 0},
		.gyros = {0, 0, 0},
		.mag = {0, 0, 0},
		.baro = {0},
		.q = {1, 0, 0, 0},
		.position = {0, 0, 0},
		.actuator = {0, 0, 0, 0, 0, 0, 0, 0}
	};

	int retval;
	
	// Initialize
	retval= sim_model_init();
	printf( "Returned %u\n", retval );

	state.actuator[0] = 1;
	state.actuator[1] = 2;
	state.actuator[2] = 3;
	state.actuator[3] = 4;

	// Step in time
	printf( "Before: %f %f %f %f\n", state.q[0], state.q[1], state.q[2], state.q[3] );
	retval = sim_model_step(0.001, &state);
	printf( "Step returned %u\n", retval);
	printf( "After: %f %f %f %f\n", state.q[0], state.q[1], state.q[2], state.q[3] );

	// Shut down system
	retval = sim_model_terminate();
	printf( "Shutdown returned %u\n", retval);

	return 0;
}
