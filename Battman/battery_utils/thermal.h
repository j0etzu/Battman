//
//  thermal.h
//  Battman
//
//  Created by Torrekie on 2025/7/11.
//

#ifndef thermal_h
#define thermal_h

#include <os/base.h>
#include <stdbool.h>
#include <TargetConditionals.h>

typedef enum {
	kBattmanThermalPressureLevelError = -1,
	kBattmanThermalPressureLevelNominal,
	kBattmanThermalPressureLevelLight,
	kBattmanThermalPressureLevelModerate,
	kBattmanThermalPressureLevelHeavy,
	kBattmanThermalPressureLevelTrapping,
	kBattmanThermalPressureLevelSleeping,

	kBattmanThermalPressureLevelUnknown
} thermal_pressure_t;

typedef enum {
	kBattmanThermalNotificationLevelAny = -1,
	kBattmanThermalNotificationLevelNormal,
	kBattmanThermalNotificationLevel70PercentTorch,
	kBattmanThermalNotificationLevel70PercentBacklight,
	kBattmanThermalNotificationLevel50PercentTorch,
	kBattmanThermalNotificationLevel50PercentBacklight,
	kBattmanThermalNotificationLevelDisableTorch,
	kBattmanThermalNotificationLevel25PercentBacklight,
	kBattmanThermalNotificationLevelDisableMapsHalo,
	kBattmanThermalNotificationLevelAppTerminate,
	kBattmanThermalNotificationLevelDeviceRestart,
	kBattmanThermalNotificationLevelThermalTableReady,

	kBattmanThermalNotificationLevelUnknown
} thermal_notif_level_t;

__BEGIN_DECLS

const char *get_thermal_pressure_string(thermal_pressure_t pressure);
const char *get_thermal_notif_level_string(thermal_notif_level_t level);

thermal_pressure_t thermal_pressure(void);
float thermal_max_trigger_temperature(void);
int thermal_solar_state(void);

#if !(TARGET_OS_OSX || TARGET_OS_MACCATALYST)
thermal_notif_level_t thermal_notif_level(void);
#endif

__END_DECLS

#endif /* thermal_h */
