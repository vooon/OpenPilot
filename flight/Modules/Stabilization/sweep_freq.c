/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup StabilizationModule Sweep frequency Module
 * @brief System identification controller
 * @{
 *
 * @file       sweep_freq.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      System identification functionality
 *
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "openpilot.h"

#include "frequencysweep.h"

#include "math.h"
#include "sin_lookup.h"

 #define F_PI ((float) M_PI)

/**
 * Apply a series of periodic perturbations to the output and measure the properties of the
 * response in order to characterize the system.  Note that the closed loop controller is
 * still providing the input to this so it measues the rejection and response properties as
 * well.
 */
int sweep_freq(float control_in, float gyro, float * control_out, int channel, bool reinit)
{
	static portTickType freqStartTime;

	static int freq_index = 0;

	static float gyro_accum_sin, gyro_accum_cos;
	static float output_accum_sin, output_accum_cos;
	static uint32_t accumulated = 0;

	const float AMPLITUDE = 0.1f;
	const float FREQUENCIES[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
	const int NUM_FREQUENCIES = NELEMENTS(FREQUENCIES);
	const float TIME_PER_FREQ = 3000; // in ms

	portTickType thisTime = xTaskGetTickCount();

	static float last_phase = 0;

	// On first run initialize estimates to something reasonable
	if(reinit) {
		gyro_accum_sin = 0;
		gyro_accum_cos = 0;
		output_accum_sin = 0;
		output_accum_cos = 0;
		accumulated = 0;

		// TODO: Move this to the end of a frequency
		freq_index = 0;
		freqStartTime = thisTime;
		last_phase = 0;
	}

	// Compute the current output perturbation
	const float current_frequency = FREQUENCIES[freq_index];
	int freq_time = thisTime - freqStartTime;
	float phase = (float) (freq_time) * current_frequency * 0.360f;  // time is in ms
	phase = fmod(phase, 360.0f);

	// Compute output, simple threshold on error
	*control_out = control_in + sin_lookup_deg(phase) * AMPLITUDE;

	/**** The code below here is to estimate the properties of the oscillation ****/

	// Accumulate the complex gyro response
	gyro_accum_sin += sin_lookup_deg(phase) * gyro;
	gyro_accum_cos += sin_lookup_deg(phase + 90) * gyro;
	output_accum_sin += 100.0f * sin_lookup_deg(phase) * (*control_out);
	output_accum_cos += 100.0f * sin_lookup_deg(phase + 90) * (*control_out);
	accumulated ++;

	// Stop when phase wraps around and enough time elapsed
	bool done = (freq_time > TIME_PER_FREQ) && (phase < last_phase);
	last_phase = phase;

	if (done) {

		float this_amplitude = 2 * sqrtf(gyro_accum_sin*gyro_accum_sin + gyro_accum_cos*gyro_accum_cos) / accumulated;
		float calculated_gains = this_amplitude / AMPLITUDE;
		float calculated_phases = 180.0f * atan2f(gyro_accum_cos, gyro_accum_sin) / F_PI;

		// Store result from this frequency
		FrequencySweepData result;
		FrequencySweepGet(&result);
		if (freq_index < FREQUENCYSWEEP_GYROGAIN_NUMELEM)
			result.GyroGain[freq_index] = calculated_gains;
		if (freq_index < FREQUENCYSWEEP_GYROPHASE_NUMELEM)
			result.GyroPhase[freq_index] = calculated_phases;

		this_amplitude = 2 * sqrtf(output_accum_sin*output_accum_sin + output_accum_cos*output_accum_cos) / accumulated;
		calculated_gains = this_amplitude / AMPLITUDE;
		calculated_phases = 180.0f * atan2f(output_accum_cos, output_accum_sin) / F_PI;
		if (freq_index < FREQUENCYSWEEP_OUTPUTGAIN_NUMELEM)
			result.OutputGain[freq_index] = calculated_gains;
		if (freq_index < FREQUENCYSWEEP_OUTPUTPHASE_NUMELEM)
			result.OutputPhase[freq_index] = calculated_phases;

		FrequencySweepSet(&result);

		accumulated = 0;
		gyro_accum_sin = 0;
		gyro_accum_cos = 0;
		output_accum_sin = 0;
		output_accum_cos = 0;

		freq_index = (freq_index + 1) % NUM_FREQUENCIES;		
		freqStartTime = thisTime;

	}

	return 0;
}


