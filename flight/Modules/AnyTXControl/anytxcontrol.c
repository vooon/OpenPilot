/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup AnyTXControlModule AnyTX Control Module
 * @brief Provide manual control.
 * @{
 *
 * @file       anytxcontrol.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      ManualControl module. Handles safety R/C link and flight mode.
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

#include <openpilot.h>
#include "anytxcontrol.h"
#include "anytxcontrolsettings.h"
#include "manualcontrolcommand.h"
#include "flighttelemetrystats.h"
#include "receiveractivity.h"
#include "accessorydesired.h"
#include "taskinfo.h"

#if defined(PIOS_INCLUDE_USB_RCTX)
#include "pios_usb_rctx.h"
#endif /* PIOS_INCLUDE_USB_RCTX */

// Private constants
#if defined(PIOS_MANUAL_STACK_SIZE)
#define STACK_SIZE_BYTES  PIOS_MANUAL_STACK_SIZE
#else
#define STACK_SIZE_BYTES  1024
#endif

#define TASK_PRIORITY     (tskIDLE_PRIORITY + 4)
#define UPDATE_PERIOD_MS  20
#define THROTTLE_FAILSAFE -0.1f
#define ARMED_TIME_MS     1000
#define ARMED_THRESHOLD   0.50f
// safe band to allow a bit of calibration error or trim offset (in microseconds)
#define CONNECTION_OFFSET 150

// Private types
typedef enum {
    ARM_STATE_DISARMED,
    ARM_STATE_ARMING_MANUAL,
    ARM_STATE_ARMED,
    ARM_STATE_DISARMING_MANUAL,
    ARM_STATE_DISARMING_TIMEOUT
} ArmState_t;

// Private variables
static xTaskHandle taskHandle;
static ArmState_t armState;
static portTickType lastSysTime;

// Private functions

static void anytxControlTask(void *parameters);
static float scaleChannel(int16_t value, int16_t max, int16_t min, int16_t neutral);
static uint32_t timeDifferenceMs(portTickType start_time, portTickType end_time);
// static bool okToArm(void);
static bool validInputRange(int16_t min, int16_t max, uint16_t value);
static void applyDeadband(float *value, float deadband);

#define RCVR_ACTIVITY_MONITOR_CHANNELS_PER_GROUP 12
#define RCVR_ACTIVITY_MONITOR_MIN_RANGE          10
struct rcvr_activity_fsm {
    AnyTXControlSettingsChannelGroupsOptions group;
    uint16_t prev[RCVR_ACTIVITY_MONITOR_CHANNELS_PER_GROUP];
    uint8_t sample_count;
};
static struct rcvr_activity_fsm activity_fsm;

static void resetRcvrActivity(struct rcvr_activity_fsm *fsm);
static bool updateRcvrActivity(struct rcvr_activity_fsm *fsm);

#define assumptions (assumptions1 && assumptions3 && assumptions5 && assumptions7 && assumptions8 && assumptions_flightmode && assumptions_channelcount)

/**
 * Module starting
 */
int32_t AnyTXControlStart()
{
    // Start main task
    xTaskCreate(anytxControlTask, (signed char *)"ManualControl", STACK_SIZE_BYTES / 4, NULL, TASK_PRIORITY, &taskHandle);
    PIOS_TASK_MONITOR_RegisterTask(TASKINFO_RUNNING_MANUALCONTROL, taskHandle);
    PIOS_WDG_RegisterFlag(PIOS_WDG_MANUAL);

    return 0;
}

/**
 * Module initialization
 */
int32_t AnyTXControlInitialize()
{
    /* Check the assumptions about uavobject enum's are correct */
    AccessoryDesiredInitialize();
    ManualControlCommandInitialize();
    ReceiverActivityInitialize();
    AnyTXControlSettingsInitialize();

    return 0;
}
MODULE_INITCALL(AnyTXControlInitialize, AnyTXControlStart)

/**
 * Module task
 */
static void anytxControlTask(__attribute__((unused)) void *parameters)
{
    AnyTXControlSettingsData settings;
    ManualControlCommandData cmd;
    // float flightMode = 0;

    uint8_t disconnected_count = 0;
    uint8_t connected_count    = 0;

    // For now manual instantiate extra instances of Accessory Desired.  In future should be done dynamically
    // this includes not even registering it if not used
    AccessoryDesiredCreateInstance();
    AccessoryDesiredCreateInstance();

    // Make sure unarmed on power up
    ManualControlCommandGet(&cmd);
    armState = ARM_STATE_DISARMED;

    /* Initialize the RcvrActivty FSM */
    portTickType lastActivityTime = xTaskGetTickCount();
    resetRcvrActivity(&activity_fsm);

    // Main task loop
    lastSysTime = xTaskGetTickCount();
    while (1) {
        float scaledChannel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_NUMELEM];

        // Wait until next update
        vTaskDelayUntil(&lastSysTime, UPDATE_PERIOD_MS / portTICK_RATE_MS);
        PIOS_WDG_UpdateFlag(PIOS_WDG_MANUAL);

        // Read settings
        AnyTXControlSettingsGet(&settings);

        /* Update channel activity monitor */
        if (updateRcvrActivity(&activity_fsm)) {
            /* Reset the aging timer because activity was detected */
            lastActivityTime = lastSysTime;
        }
        if (timeDifferenceMs(lastActivityTime, lastSysTime) > 5000) {
            resetRcvrActivity(&activity_fsm);
            lastActivityTime = lastSysTime;
        }

        if (ManualControlCommandReadOnly()) {
            FlightTelemetryStatsData flightTelemStats;
            FlightTelemetryStatsGet(&flightTelemStats);
            if (flightTelemStats.Status != FLIGHTTELEMETRYSTATS_STATUS_CONNECTED) {
                /* trying to fly via GCS and lost connection.  fall back to transmitter */
                UAVObjMetadata metadata;
                ManualControlCommandGetMetadata(&metadata);
                UAVObjSetAccess(&metadata, ACCESS_READWRITE);
                ManualControlCommandSetMetadata(&metadata);
            }
        }

        if (!ManualControlCommandReadOnly()) {
            bool valid_input_detected = true;

            // Read channel values in us
            for (uint8_t n = 0;
                 n < ANYTXCONTROLSETTINGS_CHANNELGROUPS_NUMELEM && n < MANUALCONTROLCOMMAND_CHANNEL_NUMELEM;
                 ++n) {
                extern uint32_t pios_rcvr_group_map[];

                if (settings.ChannelGroups[n] >= ANYTXCONTROLSETTINGS_CHANNELGROUPS_NONE) {
                    cmd.Channel[n] = PIOS_RCVR_INVALID;
                } else {
                    cmd.Channel[n] = PIOS_RCVR_Read(pios_rcvr_group_map[settings.ChannelGroups[n]],
                                                    settings.ChannelNumber[n]);
                }

                // If a channel has timed out this is not valid data and we shouldn't update anything
                // until we decide to go to failsafe
                if (cmd.Channel[n] == (uint16_t)PIOS_RCVR_TIMEOUT) {
                    valid_input_detected = false;
                } else {
                    scaledChannel[n] = scaleChannel(cmd.Channel[n], settings.ChannelMax[n], settings.ChannelMin[n], settings.ChannelNeutral[n]);
                }
            }

            // Check settings, if error raise alarm
            if (settings.ChannelGroups[ANYTXCONTROLSETTINGS_CHANNELGROUPS_ROLL] >= ANYTXCONTROLSETTINGS_CHANNELGROUPS_NONE ||
                settings.ChannelGroups[ANYTXCONTROLSETTINGS_CHANNELGROUPS_PITCH] >= ANYTXCONTROLSETTINGS_CHANNELGROUPS_NONE ||
                settings.ChannelGroups[ANYTXCONTROLSETTINGS_CHANNELGROUPS_YAW] >= ANYTXCONTROLSETTINGS_CHANNELGROUPS_NONE ||
                settings.ChannelGroups[ANYTXCONTROLSETTINGS_CHANNELGROUPS_THROTTLE] >= ANYTXCONTROLSETTINGS_CHANNELGROUPS_NONE ||
                // Check all channel mappings are valid
                cmd.Channel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_ROLL] == (uint16_t)PIOS_RCVR_INVALID ||
                cmd.Channel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_PITCH] == (uint16_t)PIOS_RCVR_INVALID ||
                cmd.Channel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_YAW] == (uint16_t)PIOS_RCVR_INVALID ||
                cmd.Channel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_THROTTLE] == (uint16_t)PIOS_RCVR_INVALID ||
                // Check the driver exists
                cmd.Channel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_ROLL] == (uint16_t)PIOS_RCVR_NODRIVER ||
                cmd.Channel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_PITCH] == (uint16_t)PIOS_RCVR_NODRIVER ||
                cmd.Channel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_YAW] == (uint16_t)PIOS_RCVR_NODRIVER ||
                cmd.Channel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_THROTTLE] == (uint16_t)PIOS_RCVR_NODRIVER
                ) {
                AlarmsSet(SYSTEMALARMS_ALARM_MANUALCONTROL, SYSTEMALARMS_ALARM_CRITICAL);
                cmd.Connected = MANUALCONTROLCOMMAND_CONNECTED_FALSE;
                ManualControlCommandSet(&cmd);

                continue;
            }

            // decide if we have valid manual input or not
            valid_input_detected &= validInputRange(settings.ChannelMin[ANYTXCONTROLSETTINGS_CHANNELGROUPS_THROTTLE], settings.ChannelMax[ANYTXCONTROLSETTINGS_CHANNELGROUPS_THROTTLE], cmd.Channel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_THROTTLE]) &&
                                    validInputRange(settings.ChannelMin[ANYTXCONTROLSETTINGS_CHANNELGROUPS_ROLL], settings.ChannelMax[ANYTXCONTROLSETTINGS_CHANNELGROUPS_ROLL], cmd.Channel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_ROLL]) &&
                                    validInputRange(settings.ChannelMin[ANYTXCONTROLSETTINGS_CHANNELGROUPS_YAW], settings.ChannelMax[ANYTXCONTROLSETTINGS_CHANNELGROUPS_YAW], cmd.Channel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_YAW]) &&
                                    validInputRange(settings.ChannelMin[ANYTXCONTROLSETTINGS_CHANNELGROUPS_PITCH], settings.ChannelMax[ANYTXCONTROLSETTINGS_CHANNELGROUPS_PITCH], cmd.Channel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_PITCH]);

            // Implement hysteresis loop on connection status
            if (valid_input_detected && (++connected_count > 10)) {
                cmd.Connected      = MANUALCONTROLCOMMAND_CONNECTED_TRUE;
                connected_count    = 0;
                disconnected_count = 0;
            } else if (!valid_input_detected && (++disconnected_count > 10)) {
                cmd.Connected      = MANUALCONTROLCOMMAND_CONNECTED_FALSE;
                connected_count    = 0;
                disconnected_count = 0;
            }
            // cmd.Connected = MANUALCONTROLCOMMAND_CONNECTED_TRUE;

            if (cmd.Connected == MANUALCONTROLCOMMAND_CONNECTED_FALSE) {
                cmd.Throttle   = -1;      // Shut down engine with no control
                cmd.Roll       = 0;
                cmd.Yaw = 0;
                cmd.Pitch      = 0;
                cmd.Collective = 0;
                // cmd.FlightMode = MANUALCONTROLCOMMAND_FLIGHTMODE_AUTO; // don't do until AUTO implemented and functioning
                // Important: Throttle < 0 will reset Stabilization coefficients among other things. Either change this,
                // or leave throttle at IDLE speed or above when going into AUTO-failsafe.
                AlarmsSet(SYSTEMALARMS_ALARM_MANUALCONTROL, SYSTEMALARMS_ALARM_WARNING);

                AccessoryDesiredData accessory;
                // Set Accessory 0
                if (settings.ChannelGroups[ANYTXCONTROLSETTINGS_CHANNELGROUPS_ACCESSORY0] !=
                    ANYTXCONTROLSETTINGS_CHANNELGROUPS_NONE) {
                    accessory.AccessoryVal = 0;
                    if (AccessoryDesiredInstSet(0, &accessory) != 0) {
                        AlarmsSet(SYSTEMALARMS_ALARM_MANUALCONTROL, SYSTEMALARMS_ALARM_WARNING);
                    }
                }
                // Set Accessory 1
                if (settings.ChannelGroups[ANYTXCONTROLSETTINGS_CHANNELGROUPS_ACCESSORY1] !=
                    ANYTXCONTROLSETTINGS_CHANNELGROUPS_NONE) {
                    accessory.AccessoryVal = 0;
                    if (AccessoryDesiredInstSet(1, &accessory) != 0) {
                        AlarmsSet(SYSTEMALARMS_ALARM_MANUALCONTROL, SYSTEMALARMS_ALARM_WARNING);
                    }
                }
                // Set Accessory 2
                if (settings.ChannelGroups[ANYTXCONTROLSETTINGS_CHANNELGROUPS_ACCESSORY2] !=
                    ANYTXCONTROLSETTINGS_CHANNELGROUPS_NONE) {
                    accessory.AccessoryVal = 0;
                    if (AccessoryDesiredInstSet(2, &accessory) != 0) {
                        AlarmsSet(SYSTEMALARMS_ALARM_MANUALCONTROL, SYSTEMALARMS_ALARM_WARNING);
                    }
                }
            } else {
                AlarmsClear(SYSTEMALARMS_ALARM_MANUALCONTROL);

                // Scale channels to -1 -> +1 range
                cmd.Roll     = scaledChannel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_ROLL];
                cmd.Pitch    = scaledChannel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_PITCH];
                cmd.Yaw      = scaledChannel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_YAW];
                cmd.Throttle = scaledChannel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_THROTTLE];
                // flightMode         = scaledChannel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_FLIGHTMODE];

                // Apply deadband for Roll/Pitch/Yaw stick inputs
                if (settings.Deadband > 0.0f) {
                    applyDeadband(&cmd.Roll, settings.Deadband);
                    applyDeadband(&cmd.Pitch, settings.Deadband);
                    applyDeadband(&cmd.Yaw, settings.Deadband);
                }

                if (cmd.Channel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_COLLECTIVE] != (uint16_t)PIOS_RCVR_INVALID
                    && cmd.Channel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_COLLECTIVE] != (uint16_t)PIOS_RCVR_NODRIVER
                    && cmd.Channel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_COLLECTIVE] != (uint16_t)PIOS_RCVR_TIMEOUT) {
                    cmd.Collective = scaledChannel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_COLLECTIVE];
                }

                AccessoryDesiredData accessory;
                // Set Accessory 0
                if (settings.ChannelGroups[ANYTXCONTROLSETTINGS_CHANNELGROUPS_ACCESSORY0] !=
                    ANYTXCONTROLSETTINGS_CHANNELGROUPS_NONE) {
                    accessory.AccessoryVal = scaledChannel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_ACCESSORY0];
                    if (AccessoryDesiredInstSet(0, &accessory) != 0) {
                        AlarmsSet(SYSTEMALARMS_ALARM_MANUALCONTROL, SYSTEMALARMS_ALARM_WARNING);
                    }
                }
                // Set Accessory 1
                if (settings.ChannelGroups[ANYTXCONTROLSETTINGS_CHANNELGROUPS_ACCESSORY1] !=
                    ANYTXCONTROLSETTINGS_CHANNELGROUPS_NONE) {
                    accessory.AccessoryVal = scaledChannel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_ACCESSORY1];
                    if (AccessoryDesiredInstSet(1, &accessory) != 0) {
                        AlarmsSet(SYSTEMALARMS_ALARM_MANUALCONTROL, SYSTEMALARMS_ALARM_WARNING);
                    }
                }
                // Set Accessory 2
                if (settings.ChannelGroups[ANYTXCONTROLSETTINGS_CHANNELGROUPS_ACCESSORY2] !=
                    ANYTXCONTROLSETTINGS_CHANNELGROUPS_NONE) {
                    accessory.AccessoryVal = scaledChannel[ANYTXCONTROLSETTINGS_CHANNELGROUPS_ACCESSORY2];
                    if (AccessoryDesiredInstSet(2, &accessory) != 0) {
                        AlarmsSet(SYSTEMALARMS_ALARM_MANUALCONTROL, SYSTEMALARMS_ALARM_WARNING);
                    }
                }
            }


            // Update cmd object
            ManualControlCommandSet(&cmd);
#if defined(PIOS_INCLUDE_USB_RCTX)
            if (pios_usb_rctx_id) {
                PIOS_USB_RCTX_Update(pios_usb_rctx_id,
                                     cmd.Channel,
                                     settings.ChannelMin,
                                     settings.ChannelMax,
                                     NELEMENTS(cmd.Channel));
            }
#endif /* PIOS_INCLUDE_USB_RCTX */
        } else {
            ManualControlCommandGet(&cmd); /* Under GCS control */
        }
    }
}

static void resetRcvrActivity(struct rcvr_activity_fsm *fsm)
{
    ReceiverActivityData data;
    bool updated = false;

    /* Clear all channel activity flags */
    ReceiverActivityGet(&data);
    if (data.ActiveGroup != RECEIVERACTIVITY_ACTIVEGROUP_NONE &&
        data.ActiveChannel != 255) {
        data.ActiveGroup   = RECEIVERACTIVITY_ACTIVEGROUP_NONE;
        data.ActiveChannel = 255;
        updated = true;
    }
    if (updated) {
        ReceiverActivitySet(&data);
    }

    /* Reset the FSM state */
    fsm->group = 0;
    fsm->sample_count = 0;
}

static void updateRcvrActivitySample(uint32_t rcvr_id, uint16_t samples[], uint8_t max_channels)
{
    for (uint8_t channel = 1; channel <= max_channels; channel++) {
        // Subtract 1 because channels are 1 indexed
        samples[channel - 1] = PIOS_RCVR_Read(rcvr_id, channel);
    }
}

static bool updateRcvrActivityCompare(uint32_t rcvr_id, struct rcvr_activity_fsm *fsm)
{
    bool activity_updated = false;

    /* Compare the current value to the previous sampled value */
    for (uint8_t channel = 1;
         channel <= RCVR_ACTIVITY_MONITOR_CHANNELS_PER_GROUP;
         channel++) {
        uint16_t delta;
        uint16_t prev = fsm->prev[channel - 1]; // Subtract 1 because channels are 1 indexed
        uint16_t curr = PIOS_RCVR_Read(rcvr_id, channel);
        if (curr > prev) {
            delta = curr - prev;
        } else {
            delta = prev - curr;
        }

        if (delta > RCVR_ACTIVITY_MONITOR_MIN_RANGE) {
            /* Mark this channel as active */
            ReceiverActivityActiveGroupOptions group;

            /* Don't assume manualcontrolsettings and receiveractivity are in the same order. */
            switch (fsm->group) {
            case ANYTXCONTROLSETTINGS_CHANNELGROUPS_PWM:
                group = RECEIVERACTIVITY_ACTIVEGROUP_PWM;
                break;
            case ANYTXCONTROLSETTINGS_CHANNELGROUPS_PPM:
                group = RECEIVERACTIVITY_ACTIVEGROUP_PPM;
                break;
            case ANYTXCONTROLSETTINGS_CHANNELGROUPS_DSMMAINPORT:
                group = RECEIVERACTIVITY_ACTIVEGROUP_DSMMAINPORT;
                break;
            case ANYTXCONTROLSETTINGS_CHANNELGROUPS_DSMFLEXIPORT:
                group = RECEIVERACTIVITY_ACTIVEGROUP_DSMFLEXIPORT;
                break;
            case ANYTXCONTROLSETTINGS_CHANNELGROUPS_SBUS:
                group = RECEIVERACTIVITY_ACTIVEGROUP_SBUS;
                break;
            case ANYTXCONTROLSETTINGS_CHANNELGROUPS_GCS:
                group = RECEIVERACTIVITY_ACTIVEGROUP_GCS;
                break;
            default:
                PIOS_Assert(0);
                break;
            }

            ReceiverActivityActiveGroupSet((uint8_t *)&group);
            ReceiverActivityActiveChannelSet(&channel);
            activity_updated = true;
        }
    }
    return activity_updated;
}

static bool updateRcvrActivity(struct rcvr_activity_fsm *fsm)
{
    bool activity_updated = false;

    if (fsm->group >= ANYTXCONTROLSETTINGS_CHANNELGROUPS_NONE) {
        /* We're out of range, reset things */
        resetRcvrActivity(fsm);
    }

    extern uint32_t pios_rcvr_group_map[];
    if (!pios_rcvr_group_map[fsm->group]) {
        /* Unbound group, skip it */
        goto group_completed;
    }

    if (fsm->sample_count == 0) {
        /* Take a sample of each channel in this group */
        updateRcvrActivitySample(pios_rcvr_group_map[fsm->group],
                                 fsm->prev,
                                 NELEMENTS(fsm->prev));
        fsm->sample_count++;
        return false;
    }

    /* Compare with previous sample */
    activity_updated = updateRcvrActivityCompare(pios_rcvr_group_map[fsm->group], fsm);

group_completed:
    /* Reset the sample counter */
    fsm->sample_count = 0;

    /* Find the next active group, but limit search so we can't loop forever here */
    for (uint8_t i = 0; i < ANYTXCONTROLSETTINGS_CHANNELGROUPS_NONE; i++) {
        /* Move to the next group */
        fsm->group++;
        if (fsm->group >= ANYTXCONTROLSETTINGS_CHANNELGROUPS_NONE) {
            /* Wrap back to the first group */
            fsm->group = 0;
        }
        if (pios_rcvr_group_map[fsm->group]) {
            /*
             * Found an active group, take a sample here to avoid an
             * extra 20ms delay in the main thread so we can speed up
             * this algorithm.
             */
            updateRcvrActivitySample(pios_rcvr_group_map[fsm->group],
                                     fsm->prev,
                                     NELEMENTS(fsm->prev));
            fsm->sample_count++;
            break;
        }
    }

    return activity_updated;
}

/**
 * Convert channel from servo pulse duration (microseconds) to scaled -1/+1 range.
 */
static float scaleChannel(int16_t value, int16_t max, int16_t min, int16_t neutral)
{
    float valueScaled;

    // Scale
    if ((max > min && value >= neutral) || (min > max && value <= neutral)) {
        if (max != neutral) {
            valueScaled = (float)(value - neutral) / (float)(max - neutral);
        } else {
            valueScaled = 0;
        }
    } else {
        if (min != neutral) {
            valueScaled = (float)(value - neutral) / (float)(neutral - min);
        } else {
            valueScaled = 0;
        }
    }

    // Bound
    if (valueScaled > 1.0f) {
        valueScaled = 1.0f;
    } else if (valueScaled < -1.0f) {
        valueScaled = -1.0f;
    }

    return valueScaled;
}

static uint32_t timeDifferenceMs(portTickType start_time, portTickType end_time)
{
    if (end_time > start_time) {
        return (end_time - start_time) * portTICK_RATE_MS;
    }
    return ((((portTICK_RATE_MS) - 1) - start_time) + end_time) * portTICK_RATE_MS;
}


/**
 * @brief Determine if the manual input value is within acceptable limits
 * @returns return TRUE if so, otherwise return FALSE
 */
bool validInputRange(int16_t min, int16_t max, uint16_t value)
{
    if (min > max) {
        int16_t tmp = min;
        min = max;
        max = tmp;
    }
    return value >= min - CONNECTION_OFFSET && value <= max + CONNECTION_OFFSET;
}

/**
 * @brief Apply deadband to Roll/Pitch/Yaw channels
 */
static void applyDeadband(float *value, float deadband)
{
    if (fabsf(*value) < deadband) {
        *value = 0.0f;
    } else if (*value > 0.0f) {
        *value -= deadband;
    } else {
        *value += deadband;
    }
}

/**
 * @}
 * @}
 */
