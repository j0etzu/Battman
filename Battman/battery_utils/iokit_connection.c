//
//  iokit_connection.c
//  Battman
//
//  Created by Torrekie on 2025/2/9.
//

#include "iokit_connection.h"

#include <stdbool.h>
#include <stdint.h>

hvc_menu_t *convert_hvc(CFDictionaryRef dict, size_t *size, int8_t *index) {
    if (!dict || !size || !index) return NULL;

    CFArrayRef usbHvcMenu = CFDictionaryGetValue(dict, CFSTR("UsbHvcMenu"));
    if (!usbHvcMenu || CFGetTypeID(usbHvcMenu) != CFArrayGetTypeID()) {
        *size = 0;
    }
    CFNumberRef usbHvcHvcIndex = CFDictionaryGetValue(dict, CFSTR("UsbHvcHvcIndex"));
    if (!usbHvcHvcIndex || CFGetTypeID(usbHvcHvcIndex) != CFNumberGetTypeID()) {
        *index = -1;
    } else {
        CFNumberGetValue(usbHvcHvcIndex, kCFNumberSInt8Type, index);
    }

    if (*size == 0 || !usbHvcMenu) return NULL;

    CFIndex count = CFArrayGetCount(usbHvcMenu);
    *size = (size_t)count;

    if (count == 0) {
        return NULL;
    }
    
    // consider use static hvc_menu_t[7] ?
    hvc_menu_t *menu = malloc(count * sizeof(hvc_menu_t));
    if (!menu) {
        *size = 0;
        return NULL;
    }
    
    for (CFIndex i = 0; i < count; i++) {
        CFDictionaryRef entry = CFArrayGetValueAtIndex(usbHvcMenu, i);
        if (!entry || CFGetTypeID(entry) != CFDictionaryGetTypeID()) {
            free(menu);
            *size = 0;
            return NULL;
        }
        
        CFNumberRef currentNum = CFDictionaryGetValue(entry, CFSTR("MaxCurrent"));
        CFNumberRef voltageNum = CFDictionaryGetValue(entry, CFSTR("MaxVoltage"));
        if (!currentNum || CFGetTypeID(currentNum) != CFNumberGetTypeID() ||
            !voltageNum || CFGetTypeID(voltageNum) != CFNumberGetTypeID()) {
            free(menu);
            *size = 0;
            return NULL;
        }
        
        int32_t currentVal = 0, voltageVal = 0;
        if (!CFNumberGetValue(currentNum, kCFNumberSInt32Type, &currentVal) ||
            !CFNumberGetValue(voltageNum, kCFNumberSInt32Type, &voltageVal)) {
            free(menu);
            *size = 0;
            return NULL;
        }
        
        menu[i].current = (uint16_t)currentVal;
        menu[i].voltage = (uint16_t)voltageVal;
    }

    return menu;
}

bool first_vendor_at_usagepagepairs(uint32_t *vid, uint32_t *pid, uint32_t usagePage, uint32_t usage) {
	kern_return_t       kr;
	io_iterator_t       iter;
	io_object_t         device;
	CFMutableDictionaryRef matchingDict;
	CFNumberRef         pageNum, usageNum;

	if (vid == NULL || pid == NULL) {
		return false;
	}

	matchingDict = IOServiceMatching("IOHIDDevice"); // kIOHIDDeviceKey
	if (matchingDict == NULL) {
		return false;
	}

	pageNum = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usagePage);
	usageNum = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage);
	if (!pageNum || !usageNum) {
		if (pageNum)  CFRelease(pageNum);
		if (usageNum) CFRelease(usageNum);
		return false;
	}
	// kIOHIDPrimaryUsagePageKey
	CFDictionarySetValue(matchingDict, CFSTR("PrimaryUsagePage"), pageNum);
	// kIOHIDPrimaryUsageKey
	CFDictionarySetValue(matchingDict, CFSTR("PrimaryUsage"), usageNum);
	
	CFRelease(pageNum);
	CFRelease(usageNum);
	// Note: Do NOT CFRelease(matchingDict) here, because IOServiceGetMatchingServices
	//       will take ownership of it (per Apple’s documentation).

	kr = IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDict, &iter);
	if (kr != KERN_SUCCESS) {
		return false;
	}

	bool got = false;
	while ((device = IOIteratorNext(iter))) {
		// kIOHIDVendorIDKey
		CFTypeRef vendorRef  = IORegistryEntryCreateCFProperty(device, CFSTR("VendorID"), kCFAllocatorDefault, 0);
		// kIOHIDProductIDKey
		CFTypeRef productRef = IORegistryEntryCreateCFProperty(device, CFSTR("ProductID"), kCFAllocatorDefault, 0);

		int32_t signedVid  = 0;
		int32_t signedPid  = 0;

		if (vendorRef && CFGetTypeID(vendorRef) == CFNumberGetTypeID()) {
			CFNumberGetValue((CFNumberRef)vendorRef, kCFNumberSInt32Type, &signedVid);
			*vid = signedVid;
		}
		if (productRef && CFGetTypeID(productRef) == CFNumberGetTypeID()) {
			CFNumberGetValue((CFNumberRef)productRef, kCFNumberSInt32Type, &signedPid);
			*pid = signedPid;
		}
		
		// DBGLOG("Found HID device → VendorID: 0x%04x, ProductID: 0x%04x\n", (uint32_t)vid, (uint32_t)pid);

		if (vendorRef) {
			CFRelease(vendorRef);
			got = true;
		}
		if (productRef) {
			CFRelease(productRef);
			got = true;
		}
		IOObjectRelease(device);

		if (got) break;
	}
	
	IOObjectRelease(iter);
	return got;
}
