
#include "libquad.h"
#include "pios_sim_priv.h"
#include "sim_model.h"

static mxArray * sim_state;

const mwSize state_ndim = 1;
const mwSize state_dims[1] = {1};
const char *state_fieldnames[] = {"Accels", "Gyros", "Mag", "Baro", "q", "Position", "Actuator"};

/**
 * Initialize the aircraft state in a deterministic state
 */
int sim_model_init()
{
	const char * matlab_options[] = {"-nojvm", "-nodisplay"};

	if( !mclInitializeApplication(matlab_options, sizeof(matlab_options) / sizeof(*matlab_options)) )
		return -1;

	if( !libquadInitialize() )
		return -2;

	if ( !(sim_state = mxCreateStructArray(state_ndim, state_dims,
			sizeof(state_fieldnames)/sizeof(*state_fieldnames), state_fieldnames)) )
		return -3;

	// Allocate memory for the fields
	mxSetField(sim_state, 0, "Accels", mxCreateDoubleMatrix(3,1,mxREAL));
	mxSetField(sim_state, 0, "Gyros", mxCreateDoubleMatrix(3,1,mxREAL));
	mxSetField(sim_state, 0, "Mag", mxCreateDoubleMatrix(3,1,mxREAL));
	mxSetField(sim_state, 0, "Baro", mxCreateDoubleMatrix(1,1,mxREAL));
	mxSetField(sim_state, 0, "q", mxCreateDoubleMatrix(4,1,mxREAL));
	mxSetField(sim_state, 0, "Position", mxCreateDoubleMatrix(3,1,mxREAL));
	mxSetField(sim_state, 0, "Actuator", mxCreateDoubleMatrix(8,1,mxREAL));

	return 0;
}

/**
 * Shut down matlab engine
 */
int sim_model_terminate()
{
	// Free memory from state
	mxDestroyArray(sim_state);

	// Shut down matlab systems
	libquadTerminate();
	mclTerminateApplication();

	return 0;
}

/**
 * Safely copy the contents from a float array to a matlab
 * double array
 * @param[out] dest mxArray to copy to
 * @param[in] source float array to copy from
 * @param[in] array_len length of source array
 */
int copy_field_to_mx(mxArray * dest, float * source, int array_len)
{
	double * d;
	float * f;
	int i;
	int len;

	len = mxGetM(dest) * mxGetN(dest);
	if (len > array_len)
		len = array_len;

	d = mxGetPr(dest);
	f = source;
	for (i = 0; i < len; i++)
		d[i] = f[i];
}

/**
 * Safely copy the contents from a float array to a matlab
 * double array
 * @param[out] dest float array to copy to
 * @param[in] array_len size of float array (source)
 * @param[in] source mxArray to copy to
 */
int copy_mx_to_field(float * dest, int array_len, mxArray * source)
{
	double * d;
	float * f;
	int i;
	int len;

	len = mxGetM(source) * mxGetN(source);
	if (len > array_len)
		len = array_len;

	d = mxGetPr(source);
	f = dest;
	for (i = 0; i < len; i++)
		f[i] = d[i];
}

/**
 * Advance the simulation one time step
 * @param[in] dT the amount of time to advance simulation (seconds)
 * @param[in] state the set of inputs an doutputs
 */
int sim_model_step(float dT, struct pios_sim_state * state)
{
	// Format that state into mxStructure
	mxArray * prhs[2] = {mxCreateDoubleScalar(dT), sim_state};
	mxArray * plhs;

	// Populate the matlab structure from the C structure
	copy_field_to_mx(mxGetField(sim_state, 0, "Accels"), state->accels, sizeof(state->accels) / sizeof(*state->accels));
	copy_field_to_mx(mxGetField(sim_state, 0, "Gyros"), state->gyros, sizeof(state->gyros) / sizeof(*state->gyros));
	copy_field_to_mx(mxGetField(sim_state, 0, "Mag"), state->mag, sizeof(state->mag) / sizeof(*state->mag));
	copy_field_to_mx(mxGetField(sim_state, 0, "Baro"), state->baro, sizeof(state->baro) / sizeof(*state->baro));
	copy_field_to_mx(mxGetField(sim_state, 0, "q"), state->q, sizeof(state->q) / sizeof(*state->q));
	copy_field_to_mx(mxGetField(sim_state, 0, "Position"), state->position, sizeof(state->position) / sizeof(*state->position));
	copy_field_to_mx(mxGetField(sim_state, 0, "Actuator"), state->actuator, sizeof(state->actuator) / sizeof(*state->actuator));

	// Advance matlab simulation
	mlxQuad ( 1, &plhs, 2, prhs );

	// Populate the C structure from the matlab structure
	copy_mx_to_field(state->accels, sizeof(state->accels) / sizeof(*state->accels), mxGetField(plhs, 0, "Accels"));
	copy_mx_to_field(state->gyros, sizeof(state->gyros) / sizeof(*state->gyros), mxGetField(plhs, 0, "Gyros"));
	copy_mx_to_field(state->mag, sizeof(state->mag) / sizeof(*state->mag), mxGetField(plhs, 0, "Mag"));
	copy_mx_to_field(state->baro, sizeof(state->baro) / sizeof(*state->baro), mxGetField(plhs, 0, "Baro"));
	copy_mx_to_field(state->q, sizeof(state->q) / sizeof(*state->q), mxGetField(plhs, 0, "q"));
	copy_mx_to_field(state->position, sizeof(state->position) / sizeof(*state->position), mxGetField(plhs, 0, "Position"));
	copy_mx_to_field(state->actuator, sizeof(state->actuator) / sizeof(*state->actuator), mxGetField(plhs, 0, "Actuator"));

	// Format value from mxStructure
	mxDestroyArray(prhs[0]);
	mxDestroyArray(plhs);

	return 0;
}
