
#include "libquad.h"
#include "pios_sim_priv.h"
#include "sim_model.h"

struct pios_sim_state pios_sim_state;

/**
 * Initialize the aircraft state in a deterministic state
 */
int sim_model_init()
{
    int i;
    
    for (i = 0; i < 4; i++)
        pios_sim_state.q[i] = 0;
    
    for (i = 0; i < 3; i++) {
        pios_sim_state.accels[i] = 0;
        pios_sim_state.gyros[i] = 0;
        pios_sim_state.mag[i] = 0;
        pios_sim_state.position[i] = 0;
    }
    
    pios_sim_state.baro = 0;
    return 0;
}

/**
 * Advance the simulation one time step
 * @param[in] dT the amount of time to advance simulation (seconds)
 * @param[in] state the set of inputs an doutputs
 */
int sim_model_step(float dT, struct pios_sim_state * state)
{
    // Format that state into mxStructure
    
    // Advance matlab simulation
    //mlfQuad ( ... );
    
    // Format value from mxStructure
}
