//
//  wrapper.c
//  Battman
//
//  Created by Torrekie on 2025/7/18.
//

#include <stdbool.h>
#include "scprefs.h"
#include "../common.h"

static SCPreferencesRef getThermalPrefs(void) {
	SCPreferencesRef prefs = SCPreferencesCreate(kCFAllocatorDefault, CFSTR("OSThermalStatus"), CFSTR("OSThermalStatus.plist"));
	if (!prefs) {
		os_log_error(gLog, "unable to open OSThermalStatus.plist");
		return NULL;
	}
	return prefs;
}

// ret: kSCStatus
static int savePrefs(SCPreferencesRef prefs) {
	Boolean ret;

	ret = SCPreferencesCommitChanges(prefs);
	if (ret) {
		(void)SCPreferencesApplyChanges(prefs);
	} else {
		// Should use SCLog
		os_log_error(gLog, "SCPreferencesCommitChanges failed with error: %s", SCErrorString(SCError()));
		return SCError();
	}
	
	return kSCStatusOK;
}

// ret: -1 Failed, 0 None, 3 CLTMv2
int getCLTMStatus(void) {
	SCPreferencesRef prefs = getThermalPrefs();
	if (!prefs)
		return -1;

	CFPropertyListRef status = SCPreferencesGetValue(prefs, CFSTR("CLTMStatus"));
	if (!status) {
		CFRelease(prefs);
		return 0;
	}

	int ret = 0;
	if (CFGetTypeID(status) == CFNumberGetTypeID()) {
		CFNumberGetValue(status, kCFNumberIntType, &ret);
	}

	CFRelease(prefs);
	return ret;
}

// ret: kSCStatus
int setThermPrefs(CFStringRef persistKey, CFStringRef key, CFTypeRef value, bool persist) {
	SCPreferencesRef prefs = getThermalPrefs();
	if (!prefs)
		return kSCStatusFailed;

	SCPreferencesSetValue(prefs, key, value);
	if (persistKey) SCPreferencesSetValue(prefs, persistKey, persist ? kCFBooleanTrue : kCFBooleanFalse);

	return savePrefs(prefs);
}

#define THERM_GETTER_SETTER(name, persistKey, key)			\
int set ## name ## Enabled(bool enable, bool persist) {		\
	return setThermPrefs(persistKey, key, enable ? kCFBooleanTrue : kCFBooleanFalse, persist); \
}															\
bool get ## name ## Enabled(bool *enable, bool *persist) {	\
	SCPreferencesRef prefs = getThermalPrefs();				\
	if (!prefs)												\
		return false;										\
	CFTypeRef ret = SCPreferencesGetValue(prefs, key);		\
	*enable = (ret == kCFBooleanTrue);						\
	if (persistKey) *persist = SCPreferencesGetValue(prefs, persistKey) == kCFBooleanTrue;	\
	CFRelease(prefs);										\
	return (ret != NULL);									\
}															\
int del ## name ## Entry(void) {							\
	SCPreferencesRef prefs = getThermalPrefs();				\
	if (!prefs)												\
		return kSCStatusFailed;								\
	SCPreferencesRemoveValue(prefs, key);					\
	int ret = savePrefs(prefs);								\
	os_log_info(gLog, "Deleting %s: %d", CFStringGetCStringPtr(key, kCFStringEncodingUTF8), ret);	\
	CFRelease(prefs);										\
	return ret;												\
}

THERM_GETTER_SETTER(OSNotif, CFSTR("OSThermalNotificationPersistentlyEnabled"), CFSTR("OSThermalNotificationEnabled"))
THERM_GETTER_SETTER(HIP, CFSTR("hipPersistentlyEnabled"), CFSTR("hipOverride"))
THERM_GETTER_SETTER(SimulateHIP, NULL, CFSTR("simulateHip"))
THERM_GETTER_SETTER(CLTM, CFSTR("engageBehaviorPersistentlyEnabled"), CFSTR("engageBehavior"))
THERM_GETTER_SETTER(Sunlight, CFSTR("sunlightOverridePersistentlyEnabled"), CFSTR("sunlightOverride"))
