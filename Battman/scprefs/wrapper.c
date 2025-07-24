//
//  wrapper.c
//  Battman
//
//  Created by Torrekie on 2025/7/18.
//

#include "scprefs.h"
#include "../common.h"

int getCLTMStatus(void) {
	SCPreferencesRef prefs = SCPreferencesCreate(kCFAllocatorDefault, CFSTR("OSThermalStatus"), CFSTR("OSThermalStatus.plist"));
	if (!prefs) {
		os_log_error(gLog, "unable to open OSThermalStatus.plist\n");
		return -1;
	}

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
