//
//  UPSMonitor-new.m
//  Battman
//
//  Created by Torrekie on 2025/9/6.
//

#import "common.h"
#import "UPSMonitor.h"

#include <pthread/pthread.h>
#include <syslog.h>
#import <UIKit/UIKit.h>

UPSDeviceSet *gAllUPSDevices = NULL;

// ---------------------------------------------------------------------------
// Globals (internal)
// ---------------------------------------------------------------------------

static bool UPSWatching = false;

static dispatch_queue_t gDevicesQueue;        // serial queue to protect gAllUPSDevices

static pthread_t gUPSWatchThread = 0;         // watch thread (joinable)
static CFRunLoopRef gBackgroundRunLoop = NULL;
static IONotificationPortRef gNotifyPort = NULL;
static io_iterator_t gAddedIter = MACH_PORT_NULL;

static bool gTerminationInProgress = false;

// queue used for teardown work (so we do not block main thread)
static dispatch_queue_t gTeardownQueue;

// (Full file content, with modifications and added helper functions)

// --- additions at top of file (new global lock & helper prototypes) ---
static pthread_mutex_t gAllUPSDevicesLock = PTHREAD_MUTEX_INITIALIZER;

static bool UPSDeviceSetContainsByID(UPSDeviceSet *set, uint64_t regID);
static void SetupPluginAndEventSourceForDevice(UPSDataRef upsDataRef, io_service_t upsDevice);
static void TeardownDeviceRuntime(UPSDataRef upsDataRef); // remove sources/timers/notification/plugin but keep CF properties
static void RecreateMonitoringForExistingDevices(void); // run on background runloop to reattach existing devices

void suspendPowerEventMonitoring(void);
void resumePowerEventMonitoring(void);
void cleanupPowerEventMonitoring(void);

// ---------------------------------------------------------------------
// Existing code continues, with edits below
// ---------------------------------------------------------------------
@implementation UPSMonitor

// Create an empty set
static UPSDeviceSet *UPSDeviceSetCreate(void) {
	UPSDeviceSet *set = calloc(1, sizeof(*set));
	if (!set) return NULL;
	set->capacity = 1;
	set->items    = calloc(set->capacity, sizeof(UPSDataRef));
	return set;
}

// Free the set itself (not the UPSDataRefs; you should free those separately)
static void UPSDeviceSetDestroy(UPSDeviceSet *set) {
	if (!set) return;
	free(set->items);
	free(set);
}

// Returns true if already present
static bool UPSDeviceSetContains(UPSDeviceSet *set, UPSDataRef ptr) {
	bool ret = false;
	if (!set) return false;
	pthread_mutex_lock(&gAllUPSDevicesLock);
	for (size_t i = 0; i < set->count; i++) {
		if (set->items[i]->regID == ptr->regID) { ret = true; break; }
	}
	pthread_mutex_unlock(&gAllUPSDevicesLock);
	return ret;
}

// New: check existence by regID
static bool UPSDeviceSetContainsByID(UPSDeviceSet *set, uint64_t regID) {
	bool ret = false;
	if (!set) return false;
	pthread_mutex_lock(&gAllUPSDevicesLock);
	for (size_t i = 0; i < set->count; i++) {
		if (set->items[i]->regID == regID) { ret = true; break; }
	}
	pthread_mutex_unlock(&gAllUPSDevicesLock);
	return ret;
}

// Add ptr if not already in the set
static bool UPSDeviceSetAdd(UPSDeviceSet *set, UPSDataRef ptr) {
	if (!set || !ptr) return false;
	pthread_mutex_lock(&gAllUPSDevicesLock);
	// check existence by regID
	for (size_t i = 0; i < set->count; i++) {
		if (set->items[i]->regID == ptr->regID) {
			pthread_mutex_unlock(&gAllUPSDevicesLock);
			return false;
		}
	}
	if (set->count == set->capacity) {
		size_t newCap = set->capacity * 2;
		UPSDataRef *newArr = realloc(set->items, newCap * sizeof(UPSDataRef));
		if (!newArr) {
			pthread_mutex_unlock(&gAllUPSDevicesLock);
			return false;
		}
		set->items    = newArr;
		set->capacity = newCap;
	}
	set->items[set->count++] = ptr;
	pthread_mutex_unlock(&gAllUPSDevicesLock);
	return true;
}

// Remove ptr if present; shifts tail elements down
static bool UPSDeviceSetRemove(UPSDeviceSet *set, UPSDataRef ptr) {
	if (!set || !ptr) return false;
	bool ret = false;
	pthread_mutex_lock(&gAllUPSDevicesLock);
	for (size_t i = 0; i < set->count; i++) {
		if (set->items[i]->regID == ptr->regID) {
			memmove(&set->items[i],
					&set->items[i+1],
					(set->count - i - 1) * sizeof(UPSDataRef));
			set->count--;
			ret = true;
			break;
		}
	}
	pthread_mutex_unlock(&gAllUPSDevicesLock);
	return ret;
}

// Number of items
static size_t UPSDeviceSetCount(UPSDeviceSet *set) {
	size_t c = 0;
	if (!set) return 0;
	pthread_mutex_lock(&gAllUPSDevicesLock);
	c = set->count;
	pthread_mutex_unlock(&gAllUPSDevicesLock);
	return c;
}

// Copy all items into user-supplied buffer (must be at least count() in size)
static void UPSDeviceSetGetAll(UPSDeviceSet *set, UPSDataRef *outBuffer) {
	if (!set || !outBuffer) return;
	pthread_mutex_lock(&gAllUPSDevicesLock);
	memcpy(outBuffer, set->items, set->count * sizeof(UPSDataRef));
	pthread_mutex_unlock(&gAllUPSDevicesLock);
}

UPSDataRef UPSDeviceMatchingVendorProduct(int vid, int pid) {
	UPSDataRef ret = NULL;
	if (!gAllUPSDevices || gAllUPSDevices->count == 0) {
		DBGLOG(@"[UPSMonitor] no UPS devices yet");
		return NULL;
	}
	if (vid == 0 || pid == 0) {
		return NULL;
	}
	
	pthread_mutex_lock(&gAllUPSDevicesLock);
	for (size_t i = 0; i < gAllUPSDevices->count; i++) {
		UPSDataRef d = gAllUPSDevices->items[i];
		
		SInt32 vendor, product;
		CFNumberRef number;
		if (d->upsProperties) {
			number = CFDictionaryGetValue(d->upsProperties, CFSTR("Vendor ID"));
			if (!number || !CFNumberGetValue(number, kCFNumberSInt32Type, &vendor)) {
				continue;
			}
			number = CFDictionaryGetValue(d->upsProperties, CFSTR("Product ID"));
			if (!number || !CFNumberGetValue(number, kCFNumberSInt32Type, &product)) {
				continue;
			}
			if ((vendor == vid) && (product == pid)) {
				ret = d;
				break;
			}
		}
	}
	pthread_mutex_unlock(&gAllUPSDevicesLock);
	return ret;
}

// Free per-device runtime objects and the structure. This function must be safe to call from any thread.
// It will schedule runloop removals on gBackgroundRunLoop if needed.
static void
FreeUPSData(UPSDataRef upsDataRef)
{
	if (upsDataRef == NULL) return;
	
	// Remove runloop source/timer on the background runloop (if that runloop exists)
	if (upsDataRef->upsEventSource || upsDataRef->upsEventTimer) {
		if (gBackgroundRunLoop) {
			// schedule removal on the background runloop to be safe
			CFRetain(upsDataRef); // keep until removal block runs
			CFRunLoopPerformBlock(gBackgroundRunLoop, kCFRunLoopDefaultMode, ^{
				if (upsDataRef->upsEventSource) {
					CFRunLoopRemoveSource(gBackgroundRunLoop, upsDataRef->upsEventSource, kCFRunLoopDefaultMode);
					CFRelease(upsDataRef->upsEventSource);
					upsDataRef->upsEventSource = NULL;
				}
				if (upsDataRef->upsEventTimer) {
					CFRunLoopRemoveTimer(gBackgroundRunLoop, upsDataRef->upsEventTimer, kCFRunLoopDefaultMode);
					CFRelease(upsDataRef->upsEventTimer);
					upsDataRef->upsEventTimer = NULL;
				}
				CFRelease(upsDataRef);
			});
			CFRunLoopWakeUp(gBackgroundRunLoop);
		} else {
			// no background runloop: try to remove from current runloop (best-effort)
			if (upsDataRef->upsEventSource) {
				CFRunLoopRemoveSource(CFRunLoopGetCurrent(), upsDataRef->upsEventSource, kCFRunLoopDefaultMode);
				CFRelease(upsDataRef->upsEventSource);
				upsDataRef->upsEventSource = NULL;
			}
			if (upsDataRef->upsEventTimer) {
				CFRunLoopRemoveTimer(CFRunLoopGetCurrent(), upsDataRef->upsEventTimer, kCFRunLoopDefaultMode);
				CFRelease(upsDataRef->upsEventTimer);
				upsDataRef->upsEventTimer = NULL;
			}
		}
	}
	
	if (upsDataRef->upsPlugInInterface) {
		// safe guard: only release if non-null
		(*upsDataRef->upsPlugInInterface)->Release(upsDataRef->upsPlugInInterface);
		upsDataRef->upsPlugInInterface = NULL;
	}
	
	if (upsDataRef->notification != MACH_PORT_NULL) {
		IOObjectRelease(upsDataRef->notification);
		upsDataRef->notification = MACH_PORT_NULL;
	}
	
	// release CF objects (properties/capabilities/event) - these were retained when created/assigned
	if (upsDataRef->upsProperties) {
		CFRelease(upsDataRef->upsProperties);
		upsDataRef->upsProperties = NULL;
	}
	if (upsDataRef->upsCapabilities) {
		CFRelease(upsDataRef->upsCapabilities);
		upsDataRef->upsCapabilities = NULL;
	}
	if (upsDataRef->upsEvent) {
		CFRelease(upsDataRef->upsEvent);
		upsDataRef->upsEvent = NULL;
	}
	
	free(upsDataRef);
}

// Device interest notifications come here (called by IOKit)
void DeviceNotification(void *refCon, io_service_t service, natural_t messageType, void *messageArgument ) {
	UPSDataRef upsData = (UPSDataRef)refCon;
	if (upsData == NULL) {
		return;
	}
	
	if (messageType == kIOMessageServiceIsTerminated) {
		// Remove from set and free
		if (gAllUPSDevices) {
			UPSDeviceSetRemove(gAllUPSDevices, upsData);
		}
		FreeUPSData(upsData);
	}
}

// Helper: classify typeRef and populate either timer or source pointer
static void
ProcessUPSEventSource(CFTypeRef typeRef, CFRunLoopTimerRef * pTimer, CFRunLoopSourceRef * pSource)
{
	if (!typeRef) return;
	if ( CFGetTypeID(typeRef) == CFRunLoopTimerGetTypeID() ) {
		*pTimer = (CFRunLoopTimerRef)typeRef;
		CFRetain(*pTimer);
	}
	else if ( CFGetTypeID(typeRef) == CFRunLoopSourceGetTypeID() ) {
		*pSource = (CFRunLoopSourceRef)typeRef;
		CFRetain(*pSource);
	}
}

// New helper: attempt to set up plugin and async event source for an existing upsDataRef and upsDevice
static void SetupPluginAndEventSourceForDevice(UPSDataRef upsDataRef, io_service_t upsDevice) {
	if (!upsDataRef || upsDevice == MACH_PORT_NULL) return;
	
	IOCFPlugInInterface **    plugInInterface = NULL;
	IOUPSPlugInInterface_v140 **   upsPlugInInterface  = NULL;
	SInt32                    score           = 0;
	IOReturn                  kr;
	HRESULT                   result;
	CFTypeRef typeRef = NULL;
	
	kr = IOCreatePlugInInterfaceForService(upsDevice, kIOUPSPlugInTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);
	if (kr != kIOReturnSuccess || !plugInInterface) {
		DBGLOG(@"[UPSMonitor] IOCreatePlugInInterfaceForService failed: 0x%08x", kr);
		if (plugInInterface) (*plugInInterface)->Release(plugInInterface);
		return;
	}
	
	// Try the v140 interface first
	result = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOUPSPlugInInterfaceID_v140), (LPVOID)&upsPlugInInterface);
	if ( ( result == S_OK ) && upsPlugInInterface ) {
		kr = (*upsPlugInInterface)->createAsyncEventSource(upsPlugInInterface, &typeRef);
		if ((kr != kIOReturnSuccess) || !typeRef) {
			// fallthrough to cleanup below (we may still try fallback below)
		}
	} else {
		// fallback to older interface
		result = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOUPSPlugInInterfaceID), (LPVOID)&upsPlugInInterface);
		if ( ( result == S_OK ) && upsPlugInInterface ) {
			kr = (*upsPlugInInterface)->createAsyncEventSource(upsPlugInInterface, &typeRef);
			if ((kr != kIOReturnSuccess) || !typeRef) {
				// fallthrough
			}
		}
	}
	
	// We have an async event source (or timer)
	if (typeRef) {
		// Process and attach to the background runloop
		ProcessUPSEventSource(typeRef, &upsDataRef->upsEventTimer, &upsDataRef->upsEventSource);
		
		// Attach to background runloop safely (we expect this function to be called on the background runloop,
		// but guard just in case)
		if (gBackgroundRunLoop && (upsDataRef->upsEventSource || upsDataRef->upsEventTimer)) {
			if (upsDataRef->upsEventSource) {
				CFRunLoopAddSource(gBackgroundRunLoop, upsDataRef->upsEventSource, kCFRunLoopDefaultMode);
			}
			if (upsDataRef->upsEventTimer) {
				CFRunLoopAddTimer(gBackgroundRunLoop, upsDataRef->upsEventTimer, kCFRunLoopDefaultMode);
			}
		} else {
			// best-effort: add to current runloop
			if (upsDataRef->upsEventSource) {
				CFRunLoopAddSource(CFRunLoopGetCurrent(), upsDataRef->upsEventSource, kCFRunLoopDefaultMode);
			}
			if (upsDataRef->upsEventTimer) {
				CFRunLoopAddTimer(CFRunLoopGetCurrent(), upsDataRef->upsEventTimer, kCFRunLoopDefaultMode);
			}
		}
		
		CFRelease(typeRef);
		typeRef = NULL;
	}
	
	// Now always try to get fresh properties/capabilities/event from the plugin and replace stored copies.
	// Use temporaries and swap under mutex to avoid races and leaks.
	if (( result == S_OK ) && upsPlugInInterface) {
		CFDictionaryRef newProps = NULL;
		CFSetRef newCaps  = NULL;
		CFDictionaryRef       newEvent = NULL;
		
		// attempt to get fresh values; failures are non-fatal (we keep existing ones)
		kr = (*upsPlugInInterface)->getProperties(upsPlugInInterface, &newProps);
		if ((kr != kIOReturnSuccess) || (!newProps)) {
			if (newProps) { CFRelease(newProps); newProps = NULL; }
		}
		
		kr = (*upsPlugInInterface)->getCapabilities(upsPlugInInterface, &newCaps);
		if ((kr != kIOReturnSuccess) || (!newCaps)) {
			if (newCaps) { CFRelease(newCaps); newCaps = NULL; }
		}
		
		kr = (*upsPlugInInterface)->getEvent(upsPlugInInterface, &newEvent);
		if ((kr != kIOReturnSuccess) || (!newEvent)) {
			if (newEvent) { CFRelease(newEvent); newEvent = NULL; }
		}
		
		// release the plugin interface immediately per your requirement
		(*upsPlugInInterface)->Release(upsPlugInInterface);
		upsPlugInInterface = NULL;
		
		// Atomically swap the CF objects while holding the global devices lock to avoid races
		pthread_mutex_lock(&gAllUPSDevicesLock);
		
		if (newProps) {
			if (upsDataRef->upsProperties) CFRelease(upsDataRef->upsProperties);
			upsDataRef->upsProperties = newProps; // ownership transferred
		}
		// else: keep existing upsProperties if fresh fetch failed
		
		if (newCaps) {
			if (upsDataRef->upsCapabilities) CFRelease(upsDataRef->upsCapabilities);
			upsDataRef->upsCapabilities = newCaps;
		}
		
		if (newEvent) {
			if (upsDataRef->upsEvent) CFRelease(upsDataRef->upsEvent);
			upsDataRef->upsEvent = newEvent;
		}
		
		pthread_mutex_unlock(&gAllUPSDevicesLock);
	}
	
	// Release the original plugInInterface if still held
	if (plugInInterface) {
		(*plugInInterface)->Release(plugInInterface);
		plugInInterface = NULL;
	}
	
	// Ensure interest notification is present (best-effort)
	if (upsDataRef->notification == MACH_PORT_NULL) {
		kr = IOServiceAddInterestNotification(gNotifyPort, upsDevice, "IOGeneralInterest", DeviceNotification, upsDataRef, &(upsDataRef->notification));
		if (kr != kIOReturnSuccess) {
			upsDataRef->notification = MACH_PORT_NULL;
		}
	}
}

// Helper: teardown the runtime pieces of the upsDataRef but keep upsProperties/upsCapabilities/upsEvent
static void TeardownDeviceRuntime(UPSDataRef upsDataRef) {
	if (!upsDataRef) return;
	
	// Remove sources/timers on background runloop
	if ((upsDataRef->upsEventSource || upsDataRef->upsEventTimer) && gBackgroundRunLoop) {
		CFRetain(upsDataRef);
		CFRunLoopPerformBlock(gBackgroundRunLoop, kCFRunLoopDefaultMode, ^{
			if (upsDataRef->upsEventSource) {
				CFRunLoopRemoveSource(gBackgroundRunLoop, upsDataRef->upsEventSource, kCFRunLoopDefaultMode);
				CFRelease(upsDataRef->upsEventSource);
				upsDataRef->upsEventSource = NULL;
			}
			if (upsDataRef->upsEventTimer) {
				CFRunLoopRemoveTimer(gBackgroundRunLoop, upsDataRef->upsEventTimer, kCFRunLoopDefaultMode);
				CFRelease(upsDataRef->upsEventTimer);
				upsDataRef->upsEventTimer = NULL;
			}
			CFRelease(upsDataRef);
		});
		CFRunLoopWakeUp(gBackgroundRunLoop);
	} else {
		// best-effort immediate removal if background runloop missing
		if (upsDataRef->upsEventSource) {
			CFRunLoopRemoveSource(CFRunLoopGetCurrent(), upsDataRef->upsEventSource, kCFRunLoopDefaultMode);
			CFRelease(upsDataRef->upsEventSource);
			upsDataRef->upsEventSource = NULL;
		}
		if (upsDataRef->upsEventTimer) {
			CFRunLoopRemoveTimer(CFRunLoopGetCurrent(), upsDataRef->upsEventTimer, kCFRunLoopDefaultMode);
			CFRelease(upsDataRef->upsEventTimer);
			upsDataRef->upsEventTimer = NULL;
		}
	}
	
	// Release plugin interface if still held
	if (upsDataRef->upsPlugInInterface) {
		(*upsDataRef->upsPlugInInterface)->Release(upsDataRef->upsPlugInInterface);
		upsDataRef->upsPlugInInterface = NULL;
	}
	
	// Release interest notification so we don't get callbacks while backgrounded
	if (upsDataRef->notification != MACH_PORT_NULL) {
		IOObjectRelease(upsDataRef->notification);
		upsDataRef->notification = MACH_PORT_NULL;
	}
	
	// Note: do NOT CFRelease upsProperties/upsCapabilities/upsEvent here — we want to keep these across backgrounding per requirement
}

// Recreate monitoring for devices that are currently present. Runs on background runloop.
static void RecreateMonitoringForExistingDevices(void) {
	if (!gAllUPSDevices) return;
	
	// Build a matching dictionary similar to threadMain
	CFMutableDictionaryRef matchingDict = IOServiceMatching(kIOHIDDeviceKey);
	if (!matchingDict) {
		DBGLOG(@"[UPSMonitor] ERROR: IOServiceMatching returned NULL in RecreateMonitoringForExistingDevices");
		return;
	}
	
	// Build usage-pairs array same as threadMain
	CFMutableArrayRef devicePairs = CFArrayCreateMutable(kCFAllocatorDefault, 4, &kCFTypeArrayCallBacks);
	if (!devicePairs) {
		CFRelease(matchingDict);
		return;
	}
	
	int usagePages[] = {
		kHIDPage_PowerDevice,
		kHIDPage_BatterySystem,
		kHIDPage_AppleVendor,
		kHIDPage_PowerDevice
	};
	int usages[] = {
		0,
		0,
		kHIDUsage_AppleVendor_AccessoryBattery,
		kHIDUsage_PD_PeripheralDevice
	};
	size_t count = sizeof(usagePages) / sizeof(usagePages[0]);
	for (size_t i = 0; i < count; i++) {
		CFMutableDictionaryRef pair = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
		if (!pair) {
			CFRelease(devicePairs);
			CFRelease(matchingDict);
			return;
		}
		CFNumberRef numPage = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usagePages[i]);
		CFDictionarySetValue(pair, CFSTR(kIOHIDDeviceUsagePageKey), numPage);
		CFRelease(numPage);
		if (usages[i] != 0) {
			CFNumberRef numUsage = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usages[i]);
			CFDictionarySetValue(pair, CFSTR(kIOHIDDeviceUsageKey), numUsage);
			CFRelease(numUsage);
		}
		CFArrayAppendValue(devicePairs, pair);
		CFRelease(pair);
	}
	CFDictionarySetValue(matchingDict, CFSTR(kIOHIDDeviceUsagePairsKey), devicePairs);
	CFRelease(devicePairs);
	
	// Get iterator for currently present devices
	io_iterator_t iter = MACH_PORT_NULL;
	kern_return_t kr = IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDict, &iter);
	// matchingDict is consumed by IOServiceGetMatchingServices
	matchingDict = NULL;
	if (kr != kIOReturnSuccess || iter == MACH_PORT_NULL) {
		DBGLOG(@"[UPSMonitor] IOServiceGetMatchingServices failed during recreate: 0x%08x", kr);
		if (iter != MACH_PORT_NULL) IOObjectRelease(iter);
		return;
	}
	
	// Iterate present devices; for each, try to match by registry entry ID to an existing element in gAllUPSDevices
	io_service_t service;
	while ((service = IOIteratorNext(iter))) {
		uint64_t entryID = 0;
		IORegistryEntryGetRegistryEntryID(service, &entryID);
		
		// find an upsDataRef with same regID
		pthread_mutex_lock(&gAllUPSDevicesLock);
		UPSDataRef matched = NULL;
		for (size_t i = 0; i < gAllUPSDevices->count; i++) {
			if (gAllUPSDevices->items[i]->regID == entryID) {
				matched = gAllUPSDevices->items[i];
				break;
			}
		}
		pthread_mutex_unlock(&gAllUPSDevicesLock);
		
		if (matched) {
			// set up plugin and event source for the existing upsDataRef
			SetupPluginAndEventSourceForDevice(matched, service);
		} else {
			// no existing device with that regID — normal: let UPSDeviceAdded handle it (it will be called via first-match notifications)
		}
		
		IOObjectRelease(service);
	}
	IOObjectRelease(iter);
}

// Called by the IOKit matching notification when devices are added
static void
UPSDeviceAdded(void *refCon, io_iterator_t iterator)
{
	io_object_t             upsDevice           = MACH_PORT_NULL;
	
	while ( (upsDevice = IOIteratorNext(iterator)) ) {
		DBGLOG(@"[UPSMonitor] UPSDevice Got");
		IOCFPlugInInterface **    plugInInterface = NULL;
		IOUPSPlugInInterface_v140 **   upsPlugInInterface  = NULL;
		SInt32                    score           = 0;
		IOReturn                  kr;
		HRESULT                   result;
		CFTypeRef typeRef = NULL;
		
		UPSDataRef upsDataRef = calloc(1, sizeof(UPSData));
		if (!upsDataRef) {
			IOObjectRelease(upsDevice);
			continue;
		}
		
		uint64_t entryID = 0;
		IORegistryEntryGetRegistryEntryID(upsDevice, &entryID);
		upsDataRef->regID              = entryID;
		upsDataRef->notification       = MACH_PORT_NULL;
		upsDataRef->upsPlugInInterface = NULL;
		upsDataRef->upsProperties      = NULL;
		upsDataRef->upsCapabilities    = NULL;
		upsDataRef->upsEvent           = NULL;
		upsDataRef->upsEventSource     = NULL;
		upsDataRef->upsEventTimer      = NULL;
		
		// If we already have a device with this registry ID, avoid creating a duplicate. Free the temp and continue.
		if (gAllUPSDevices && UPSDeviceSetContainsByID(gAllUPSDevices, entryID)) {
			DBGLOG(@"[UPSMonitor] device with regID %llu already present - skipping", entryID);
			free(upsDataRef);
			IOObjectRelease(upsDevice);
			continue;
		}
		
		// Create the CF plugin for this device (we only use it to create async event source & read properties)
		kr = IOCreatePlugInInterfaceForService(upsDevice, kIOUPSPlugInTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);
		
		if (kr != kIOReturnSuccess)
			goto CLEANUP_PARTIAL;
		
		// Grab the new v140 interface
		result = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOUPSPlugInInterfaceID_v140), (LPVOID)&upsPlugInInterface);
		
		if ( ( result == S_OK ) && upsPlugInInterface ) {
			kr = (*upsPlugInInterface)->createAsyncEventSource(upsPlugInInterface, &typeRef);
			
			if ((kr != kIOReturnSuccess) || !typeRef)
				goto CLEANUP_PARTIAL;
			
			if (CFGetTypeID(typeRef) == CFArrayGetTypeID()) {
				CFArrayRef arrayRef = (CFArrayRef)typeRef;
				CFIndex     count   = CFArrayGetCount(arrayRef);
				
				for (CFIndex i = 0; i < count; i++) {
					CFTypeRef element = CFArrayGetValueAtIndex(arrayRef, i);
					ProcessUPSEventSource(element, &upsDataRef->upsEventTimer, &upsDataRef->upsEventSource);
				}
			}
			else {
				ProcessUPSEventSource(typeRef, &upsDataRef->upsEventTimer, &upsDataRef->upsEventSource);
			}
			
			// Attach source/timer to the background runloop (we're in the background thread's runloop context)
			if (upsDataRef->upsEventSource) {
				CFRunLoopAddSource(CFRunLoopGetCurrent(), upsDataRef->upsEventSource, kCFRunLoopDefaultMode);
			}
			if (upsDataRef->upsEventTimer) {
				CFRunLoopAddTimer(CFRunLoopGetCurrent(), upsDataRef->upsEventTimer, kCFRunLoopDefaultMode);
			}
			
			if ( typeRef )
				CFRelease(typeRef);
		}
		// Couldn't grab the new interface.  Fallback on the old.
		else
		{
			result = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOUPSPlugInInterfaceID), (LPVOID)&upsPlugInInterface);
		}
		
		// Got the interface
		if ( ( result == S_OK ) && upsPlugInInterface )
		{
			// Fetch properties, capabilities and events and store them in the upsDataRef (kept across backgrounding)
			upsDataRef->upsPlugInInterface = (IOUPSPlugInInterface **)upsPlugInInterface; // temporarily store pointer
			kr = (*upsPlugInInterface)->getProperties(upsPlugInInterface, &upsDataRef->upsProperties);
			if ((kr != kIOReturnSuccess) || (!upsDataRef->upsProperties)) {
				// proceed, we might still have event source
			}
			
			kr = (*upsPlugInInterface)->getCapabilities(upsPlugInInterface, &upsDataRef->upsCapabilities);
			if ((kr != kIOReturnSuccess) || (!upsDataRef->upsCapabilities)) {
				// proceed
			}
			
			kr = (*upsPlugInInterface)->getEvent(upsPlugInInterface, &upsDataRef->upsEvent);
			if ((kr != kIOReturnSuccess) || (!upsDataRef->upsEvent)) {
				// proceed
			}
			
			// Per requirement: release the plugin interface immediately after we got what we needed.
			if (upsDataRef->upsPlugInInterface) {
				(*upsDataRef->upsPlugInInterface)->Release(upsDataRef->upsPlugInInterface);
				upsDataRef->upsPlugInInterface = NULL;
			}
			
			// Create interest notification and add to set
			kr = IOServiceAddInterestNotification(gNotifyPort, upsDevice, "IOGeneralInterest", DeviceNotification, upsDataRef, &(upsDataRef->notification));
			if (kr != kIOReturnSuccess) {
				upsDataRef->notification = MACH_PORT_NULL;
			}
			
			if (!gAllUPSDevices) {
				gAllUPSDevices = UPSDeviceSetCreate();
			}
			if (!UPSDeviceSetAdd(gAllUPSDevices, upsDataRef)) {
				// If we failed to add (race or duplicate), cleanup upsDataRef to avoid leak
				DBGLOG(@"[UPSMonitor] Failed to add upsDataRef to global set - cleaning up");
				if (upsDataRef->notification != MACH_PORT_NULL) {
					IOObjectRelease(upsDataRef->notification);
					upsDataRef->notification = MACH_PORT_NULL;
				}
				if (upsDataRef->upsEventSource) {
					CFRunLoopRemoveSource(CFRunLoopGetCurrent(), upsDataRef->upsEventSource, kCFRunLoopDefaultMode);
					CFRelease(upsDataRef->upsEventSource);
					upsDataRef->upsEventSource = NULL;
				}
				if (upsDataRef->upsEventTimer) {
					CFRunLoopRemoveTimer(CFRunLoopGetCurrent(), upsDataRef->upsEventTimer, kCFRunLoopDefaultMode);
					CFRelease(upsDataRef->upsEventTimer);
					upsDataRef->upsEventTimer = NULL;
				}
				if (upsDataRef->upsProperties) CFRelease(upsDataRef->upsProperties);
				if (upsDataRef->upsCapabilities) CFRelease(upsDataRef->upsCapabilities);
				if (upsDataRef->upsEvent) CFRelease(upsDataRef->upsEvent);
				free(upsDataRef);
				upsDataRef = NULL;
				IOObjectRelease(upsDevice);
				continue;
			}
			
#ifdef DEBUG
			PrintAllUPSDevices();
#endif
			IOObjectRelease(upsDevice);
			continue;
		}
		
	CLEANUP_PARTIAL:
		// (same cleanup logic as before), but be careful to only release what we have.
		if (upsDataRef) {
			DBGLOG(@"[UPSMonitor] cleanup");
			if (upsDataRef->notification != MACH_PORT_NULL) {
				IOObjectRelease(upsDataRef->notification);
				upsDataRef->notification = MACH_PORT_NULL;
			}
			// if plugin interface stored, release it
			if (upsDataRef->upsPlugInInterface) {
				(*upsDataRef->upsPlugInInterface)->Release(upsDataRef->upsPlugInInterface);
				upsDataRef->upsPlugInInterface = NULL;
			}
			if (upsDataRef->upsProperties)     CFRelease(upsDataRef->upsProperties);
			if (upsDataRef->upsCapabilities)   CFRelease(upsDataRef->upsCapabilities);
			if (upsDataRef->upsEvent)          CFRelease(upsDataRef->upsEvent);
			if (upsDataRef->upsEventSource) {
				CFRunLoopRemoveSource(CFRunLoopGetCurrent(), upsDataRef->upsEventSource, kCFRunLoopDefaultMode);
				CFRelease(upsDataRef->upsEventSource);
				upsDataRef->upsEventSource = NULL;
			}
			if (upsDataRef->upsEventTimer) {
				CFRunLoopRemoveTimer(CFRunLoopGetCurrent(), upsDataRef->upsEventTimer, kCFRunLoopDefaultMode);
				CFRelease(upsDataRef->upsEventTimer);
				upsDataRef->upsEventTimer = NULL;
			}
			free(upsDataRef);
			upsDataRef = NULL;
		}
		if (plugInInterface) {
			(*plugInInterface)->Release(plugInInterface);
			plugInInterface = NULL;
		}
		IOObjectRelease(upsDevice);
	}
}

// --- threadMain and signal handling largely unchanged, small guard added ---
static void
threadMain(void)
{
	@autoreleasepool {
		gNotifyPort = IONotificationPortCreate(kIOMasterPortDefault);
		if (!gNotifyPort) {
			NSLog(@"[UPSMonitor] ERROR: failed to create IONotificationPort");
			return;
		}
		
		CFRunLoopSourceRef rlSrc = IONotificationPortGetRunLoopSource(gNotifyPort);
		CFRunLoopAddSource(CFRunLoopGetCurrent(), rlSrc, kCFRunLoopDefaultMode);
		gBackgroundRunLoop = CFRunLoopGetCurrent();
		CFRetain(gBackgroundRunLoop);
		
		CFMutableDictionaryRef matchingDict = IOServiceMatching(kIOHIDDeviceKey);
		if (!matchingDict) {
			NSLog(@"[UPSMonitor] ERROR: IOServiceMatching returned NULL");
			goto CLEANUP_ALL;
		}
		
		// Build the usage‐pairs array:
		CFMutableArrayRef devicePairs = CFArrayCreateMutable(kCFAllocatorDefault, 4, &kCFTypeArrayCallBacks);
		if (!devicePairs) {
			CFRelease(matchingDict);
			goto CLEANUP_ALL;
		}
		
		int usagePages[] = {
			kHIDPage_PowerDevice,
			kHIDPage_BatterySystem,
			kHIDPage_AppleVendor,
			kHIDPage_PowerDevice
		};
		int usages[] = {
			0,
			0,
			kHIDUsage_AppleVendor_AccessoryBattery,
			kHIDUsage_PD_PeripheralDevice
		};
		size_t count = sizeof(usagePages) / sizeof(usagePages[0]);
		for (size_t i = 0; i < count; i++) {
			CFMutableDictionaryRef pair = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
			if (!pair) {
				CFRelease(devicePairs);
				CFRelease(matchingDict);
				goto CLEANUP_ALL;
			}
			CFNumberRef numPage = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usagePages[i]);
			if (!numPage) {
				CFRelease(pair);
				CFRelease(devicePairs);
				CFRelease(matchingDict);
				goto CLEANUP_ALL;
			}
			CFDictionarySetValue(pair, CFSTR(kIOHIDDeviceUsagePageKey), numPage);
			CFRelease(numPage);
			
			// add Usage if non‐zero (some entries are zero meaning “don’t filter on usage”)
			if (usages[i] != 0) {
				CFNumberRef numUsage = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usages[i]);
				if (!numUsage) {
					CFRelease(pair);
					CFRelease(devicePairs);
					CFRelease(matchingDict);
					goto CLEANUP_ALL;
				}
				CFDictionarySetValue(pair, CFSTR(kIOHIDDeviceUsageKey), numUsage);
				CFRelease(numUsage);
			}
			
			CFArrayAppendValue(devicePairs, pair);
			CFRelease(pair);
		}
		
		CFDictionarySetValue(matchingDict, CFSTR(kIOHIDDeviceUsagePairsKey), devicePairs);
		CFRelease(devicePairs);
		devicePairs = NULL;
		
		// Now set up the “first match” notification so UPSDeviceAdded() is called whenever a device arrives.
		kern_return_t kr = IOServiceAddMatchingNotification(gNotifyPort, kIOFirstMatchNotification, matchingDict, UPSDeviceAdded, NULL, &gAddedIter);
		// matchingDict is retained by IOKit (no need to CFRelease here; IOKit takes ownership)
		matchingDict = NULL;
		
		if (kr != kIOReturnSuccess) {
			NSLog(@"[UPSMonitor] ERROR: IOServiceAddMatchingNotification failed: 0x%08x", kr);
			goto CLEANUP_ALL;
		}
		DBGLOG(@"[UPSMonitor] thread setup");
		// Drain any already‐present devices so they don’t get missed
		UPSDeviceAdded(NULL, gAddedIter);
		
		CFRunLoopRun();
		
	CLEANUP_ALL:
		if (gBackgroundRunLoop) {
			CFRelease(gBackgroundRunLoop);
			gBackgroundRunLoop = NULL;
		}
		
		if (gAddedIter != MACH_PORT_NULL) {
			IOObjectRelease(gAddedIter);
			gAddedIter = MACH_PORT_NULL;
		}
		
		if (gAllUPSDevices) {
			size_t n = UPSDeviceSetCount(gAllUPSDevices);
			UPSDataRef *buffer = calloc(n, sizeof(UPSDataRef));
			UPSDeviceSetGetAll(gAllUPSDevices, buffer);
			for (size_t i = 0; i < n; i++) {
				FreeUPSData(buffer[i]);
			}
			free(buffer);
			UPSDeviceSetDestroy(gAllUPSDevices);
			gAllUPSDevices = NULL;
		}
		
		
		if (gNotifyPort) {
			IONotificationPortDestroy(gNotifyPort);
			gNotifyPort = NULL;
		}
	}
}

// Signal and cleanup functions unchanged except ensure cleanupAllResources uses TeardownDeviceRuntime
void SignalHandler(int sigraised) {
	syslog(LOG_INFO, "Battman: received signal %d, exiting gracefully\n", sigraised);
	[UPSMonitor cleanupAllResources];
	// app_exit();
}

// Update the CleanupAndExit function
void CleanupAndExit(void) {
	[UPSMonitor cleanupAllResources];
	CFRunLoopStop(CFRunLoopGetCurrent());
}

static bool gNotificationsPaused = false;

+ (void)cleanupAllResources
{
	if (gTerminationInProgress) {
		return; // Prevent double cleanup
	}
	gTerminationInProgress = true;
	
	NSLog(@"[UPSMonitor] Starting graceful shutdown...");
	
	// Stop power event monitoring first
	cleanupPowerEventMonitoring();
	
	// Stop the UPS monitoring run loop if it's running
	if (gBackgroundRunLoop) {
		CFRunLoopPerformBlock(gBackgroundRunLoop, kCFRunLoopDefaultMode, ^{
			CFRunLoopStop(CFRunLoopGetCurrent());
		});
		CFRunLoopWakeUp(gBackgroundRunLoop);
		
		// Give the run loop time to stop gracefully
		dispatch_semaphore_t stopSemaphore = dispatch_semaphore_create(0);
		dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
			// Wait a bit for the run loop to stop
			usleep(100000); // 100ms
			dispatch_semaphore_signal(stopSemaphore);
		});
		
		// Wait up to 1 second for graceful shutdown
		dispatch_semaphore_wait(stopSemaphore, dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC));
	}
	
	// Clean up all UPS devices
	if (gAllUPSDevices) {
		NSLog(@"[UPSMonitor] Cleaning up %zu UPS devices", UPSDeviceSetCount(gAllUPSDevices));
		size_t n = UPSDeviceSetCount(gAllUPSDevices);
		if (n > 0) {
			UPSDataRef *buffer = calloc(n, sizeof(UPSDataRef));
			if (buffer) {
				UPSDeviceSetGetAll(gAllUPSDevices, buffer);
				for (size_t i = 0; i < n; i++) {
					FreeUPSData(buffer[i]);
				}
				free(buffer);
			}
		}
		UPSDeviceSetDestroy(gAllUPSDevices);
		gAllUPSDevices = NULL;
	}
	
	// Clean up IOKit resources
	if (gAddedIter != MACH_PORT_NULL) {
		IOObjectRelease(gAddedIter);
		gAddedIter = MACH_PORT_NULL;
	}
	
	if (gNotifyPort) {
		IONotificationPortDestroy(gNotifyPort);
		gNotifyPort = NULL;
	}
	
	// Clean up run loop reference
	if (gBackgroundRunLoop) {
		CFRelease(gBackgroundRunLoop);
		gBackgroundRunLoop = NULL;
	}
	
	// Reset state
	UPSWatching = false;
	gNotificationsPaused = false;
	
	NSLog(@"[UPSMonitor] Graceful shutdown completed");
}

// app background/foreground handling — updated to teardown/recreate runtime parts (but keep CF properties)
+ (void)appWillTerminate:(NSNotification *)note
{
	NSLog(@"[UPSMonitor] App will terminate - beginning cleanup");
	[self cleanupAllResources];
}

+ (void)appDidEnterBackground:(NSNotification *)note
{
	if (gTerminationInProgress) return;
	
	extern dispatch_queue_t _powerQueue;
	
	// stop power monitoring first
	suspendPowerEventMonitoring();
	
	// If already paused or not initialized, nothing to do
	if (gBackgroundRunLoop == NULL || gNotifyPort == NULL || gNotificationsPaused)
		return;
	
	CFRunLoopSourceRef src = IONotificationPortGetRunLoopSource(gNotifyPort);
	if (!src) return;
	
	// Remove the UPS monitoring run loop source
	CFRunLoopPerformBlock(gBackgroundRunLoop, kCFRunLoopDefaultMode, ^{
		CFRunLoopRemoveSource(CFRunLoopGetCurrent(), src, kCFRunLoopDefaultMode);
	});
	CFRunLoopWakeUp(gBackgroundRunLoop);
	
	// Teardown runtime pieces of each device (leave properties/capabilities/events)
	if (gAllUPSDevices) {
		size_t n = UPSDeviceSetCount(gAllUPSDevices);
		if (n > 0) {
			UPSDataRef *buffer = calloc(n, sizeof(UPSDataRef));
			if (buffer) {
				UPSDeviceSetGetAll(gAllUPSDevices, buffer);
				for (size_t i = 0; i < n; i++) {
					TeardownDeviceRuntime(buffer[i]);
				}
				free(buffer);
			}
		}
	}
	
	gNotificationsPaused = true;
	NSLog(@"[UPSMonitor] Suspended monitoring - app entered background (plugin/interfaces released, CF props retained)");
}

+ (void)appWillEnterForeground:(NSNotification *)note
{
	if (gTerminationInProgress) return;
	
	extern dispatch_queue_t _powerQueue;
	
	// resume power monitoring first
	resumePowerEventMonitoring();
	
	if (gBackgroundRunLoop == NULL || gNotifyPort == NULL || !gNotificationsPaused)
		return;
	
	CFRunLoopSourceRef src = IONotificationPortGetRunLoopSource(gNotifyPort);
	if (!src) return;
	
	// Add back the UPS monitoring run loop source
	CFRunLoopPerformBlock(gBackgroundRunLoop, kCFRunLoopDefaultMode, ^{
		CFRunLoopAddSource(CFRunLoopGetCurrent(), src, kCFRunLoopDefaultMode);
	});
	CFRunLoopWakeUp(gBackgroundRunLoop);
	
	// Recreate plugin/interfaces/event sources for existing devices on the background runloop
	if (gAllUPSDevices) {
		CFRunLoopPerformBlock(gBackgroundRunLoop, kCFRunLoopDefaultMode, ^{
			RecreateMonitoringForExistingDevices();
		});
		CFRunLoopWakeUp(gBackgroundRunLoop);
	}
	
	gNotificationsPaused = false;
	NSLog(@"[UPSMonitor] Resumed monitoring - app will enter foreground");
}


+ (void)startWatchingUPS
{
	DBGLOG(@"[UPSMonitor] called");
	if (UPSWatching) {
		return;
	}
	
	NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
	[nc addObserver:self selector:@selector(appDidEnterBackground:) name:UIApplicationDidEnterBackgroundNotification object:nil];
	[nc addObserver:self selector:@selector(appWillEnterForeground:) name:UIApplicationWillEnterForegroundNotification object:nil];
	[nc addObserver:self selector:@selector(appWillTerminate:) name:UIApplicationWillTerminateNotification object:nil];
	
	// Install a signal handler so if someone ^C’s, we clean up
	signal(SIGINT, SignalHandler);
	signal(SIGTERM, SignalHandler);
	
	// Spawn a detached pthread that we do not affect the UIKit
	pthread_t thread;
	pthread_attr_t attrs;
	pthread_attr_init(&attrs);
	pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
	
	int err = pthread_create(&thread, &attrs, (void *(*)(void *))threadMain, NULL);
	pthread_attr_destroy(&attrs);
	
	if (err) {
		NSLog(@"[UPSMonitor] Failed to create UPS‐watch thread: %d", err);
	} else {
		gUPSWatchThread = thread;
		NSLog(@"[UPSMonitor] UPS‐watch thread launched.");
		UPSWatching = true;
	}
}

// manually stop monitoring (useful for testing ig)
+ (void)stopWatchingUPS
{
	NSLog(@"[UPSMonitor] Manually stopping UPS monitoring");
	[self cleanupAllResources];
	
	// Remove notification observers
	NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
	[nc removeObserver:self name:UIApplicationDidEnterBackgroundNotification object:nil];
	[nc removeObserver:self name:UIApplicationWillEnterForegroundNotification object:nil];
	[nc removeObserver:self name:UIApplicationWillTerminateNotification object:nil];
}

@end
