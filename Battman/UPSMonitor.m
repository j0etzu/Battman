//
//  UPSMonitor.m
//  Battman
//
//  Created by Torrekie on 2025/6/3.
//  This is exactly what ioupsd does, but ported to work with iOS App
//

#import "UPSMonitor.h"
#import "iokitextern.h"
#include <pthread/pthread.h>
#include <syslog.h>
#import <UIKit/UIKit.h>

#if __has_include(<IOKit/hid/AppleHIDUsageTables.h>)
#include <IOKit/hid/AppleHIDUsageTables.h>
#else
#define kHIDPage_AppleVendor 0xFF00
#define kHIDUsage_AppleVendor_AccessoryBattery 0x0014
#endif
#if __has_include(<IOKit/hid/IOHIDUsageTables.h>)
#include <IOKit/hid/IOHIDUsageTables.h>
#else
#define kHIDPage_PowerDevice 0x0084
#define kHIDPage_BatterySystem 0x0085
#define kHIDUsage_PD_PeripheralDevice 0x0006
#define kHIDUsage_BS_PrimaryBattery 0x002E
#endif
#if __has_include(<IOKit/hid/IOHIDKeys.h>)
#include <IOKit/hid/IOHIDKeys.h>
#else
#define kIOFirstMatchNotification "IOServiceFirstMatch"
#define kIOServicePlane "IOService"
#define kIOHIDDeviceKey "IOHIDDevice"
#define kIOHIDDeviceUsagePageKey "DeviceUsagePage"
#define kIOHIDDeviceUsageKey "DeviceUsage"
#define kIOHIDDeviceUsagePairsKey "DeviceUsagePairs"
#endif
#if __has_include(<IOKit/ps/IOUPSPlugIn.h>)
#include <IOKit/ps/IOUPSPlugIn.h>
#else

#define kIOMessageServiceIsTerminated 0xe0000010
#define kIOCFPlugInInterfaceID CFUUIDGetConstantUUIDWithBytes(NULL,	\
0xC2, 0x44, 0xE8, 0x58, 0x10, 0x9C, 0x11, 0xD4,			\
0x91, 0xD4, 0x00, 0x50, 0xE4, 0xC6, 0x42, 0x6F)
#define kIOUPSPlugInTypeID CFUUIDGetConstantUUIDWithBytes(NULL, 	\
0x40, 0xa5, 0x7a, 0x4e, 0x26, 0xa0, 0x11, 0xd8,			\
0x92, 0x95, 0x00, 0x0a, 0x95, 0x8a, 0x2c, 0x78)
#define kIOUPSPlugInInterfaceID_v140 CFUUIDGetConstantUUIDWithBytes(NULL, 	\
0xe6, 0xe, 0x7, 0x99, 0x9a, 0xa6, 0x49, 0xdf,               \
0xb5, 0x5b, 0xa5, 0xc9, 0x4b, 0xa0, 0x7a, 0x4a)
#define kIOUPSPlugInInterfaceID CFUUIDGetConstantUUIDWithBytes(NULL, 	\
0x63, 0xf8, 0xbf, 0xc4, 0x26, 0xa0, 0x11, 0xd8, 			\
0x88, 0xb4, 0x0, 0xa, 0x95, 0x8a, 0x2c, 0x78)

typedef void (*IOUPSEventCallbackFunction)
(void *	 		target,
 IOReturn 		result,
 void * 			refcon,
 void * 			sender,
 CFDictionaryRef  event);

#define IOUPSPLUGINBASE							\
IOReturn (*getProperties)(	void * thisPointer, 			\
CFDictionaryRef * properties);		\
IOReturn (*getCapabilities)(void * thisPointer, 			\
CFSetRef * capabilities);		\
IOReturn (*getEvent)(	void * thisPointer, 			\
CFDictionaryRef * event);		\
IOReturn (*setEventCallback)(void * thisPointer, 			\
IOUPSEventCallbackFunction callback,	\
void * callbackTarget,  		\
void * callbackRefcon);			\
IOReturn (*sendCommand)(	void * thisPointer, 			\
CFDictionaryRef command)

#define IOUPSPLUGIN_V140							\
IOReturn (*createAsyncEventSource)(void * thisPointer,      \
CFTypeRef * source)


typedef struct IOUPSPlugInInterface {
	IUNKNOWN_C_GUTS;
	IOUPSPLUGINBASE;
} IOUPSPlugInInterface;

typedef struct IOUPSPlugInInterface_v140 {
	IUNKNOWN_C_GUTS;
	IOUPSPLUGINBASE;
	IOUPSPLUGIN_V140;
} IOUPSPlugInInterface_v140;
#endif

static bool UPSWatching = false;
static CFRunLoopRef   gBackgroundRunLoop = NULL;

@implementation UPSMonitor

typedef struct UPSData {
	io_object_t                 notification;          // returned by IOServiceAddInterestNotification
	IOUPSPlugInInterface **     upsPlugInInterface;    // the v140 or old plugin Interface
	CFDictionaryRef             upsProperties;         // retained via getProperties
	CFDictionaryRef             upsEvent;              // retained via getEvent
	CFSetRef                    upsCapabilities;       // retained via getCapabilities
	CFRunLoopSourceRef          upsEventSource;        // from createAsyncEventSource
	CFRunLoopTimerRef           upsEventTimer;         // from createAsyncEventSource
} UPSData;

typedef UPSData * UPSDataRef;

static IONotificationPortRef    gNotifyPort     = NULL;
static io_iterator_t            gAddedIter      = MACH_PORT_NULL;
static CFMutableSetRef          gAllUPSDevices  = NULL;   // holds (UPSDataRef) values

static void
FreeUPSData(UPSDataRef upsDataRef)
{
	if (upsDataRef == NULL) return;

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

	if (upsDataRef->upsPlugInInterface) {
		(*upsDataRef->upsPlugInInterface)->Release(upsDataRef->upsPlugInInterface);
		upsDataRef->upsPlugInInterface = NULL;
	}

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

	if (upsDataRef->notification != MACH_PORT_NULL) {
		IOObjectRelease(upsDataRef->notification);
		upsDataRef->notification = MACH_PORT_NULL;
	}

	free(upsDataRef);
}

void DeviceNotification(void *refCon, io_service_t service, natural_t messageType, void *messageArgument ) {
	UPSDataRef upsData = (UPSDataRef)refCon;
	if (upsData == NULL) {
		return;
	}
	
	if (messageType == kIOMessageServiceIsTerminated) {
		if (gAllUPSDevices && CFSetContainsValue(gAllUPSDevices, upsData)) {
			CFSetRemoveValue(gAllUPSDevices, upsData);
		}
		FreeUPSData(upsData);
	}
}

static void
ProcessUPSEventSource(CFTypeRef typeRef, CFRunLoopTimerRef * pTimer, CFRunLoopSourceRef * pSource)
{
	if ( CFGetTypeID(typeRef) == CFRunLoopTimerGetTypeID() ) {
		*pTimer = (CFRunLoopTimerRef)typeRef;
	}
	else if ( CFGetTypeID(typeRef) == CFRunLoopSourceGetTypeID() ) {
		*pSource = (CFRunLoopSourceRef)typeRef;
	}
}

static void
UPSDeviceAdded(void *refCon, io_iterator_t iterator)
{
	io_object_t             upsDevice           = MACH_PORT_NULL;
	
	while ( (upsDevice = IOIteratorNext(iterator)) ) {
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

		upsDataRef->notification       = MACH_PORT_NULL;
		upsDataRef->upsPlugInInterface = NULL;
		upsDataRef->upsProperties      = NULL;
		upsDataRef->upsCapabilities    = NULL;
		upsDataRef->upsEvent           = NULL;
		upsDataRef->upsEventSource     = NULL;
		upsDataRef->upsEventTimer      = NULL;

		// Create the CF plugin for this device
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
			upsDataRef->upsPlugInInterface = (IOUPSPlugInInterface **)upsPlugInInterface;

			(*plugInInterface)->Release(plugInInterface);
			plugInInterface = NULL;

			kr = (*upsPlugInInterface)->getProperties(upsPlugInInterface, &upsDataRef->upsProperties);
			if ((kr != kIOReturnSuccess) || (!upsDataRef->upsProperties)) {
				goto CLEANUP_PARTIAL;
			}
			
			kr = (*upsPlugInInterface)->getCapabilities(upsPlugInInterface, &upsDataRef->upsCapabilities);
			if ((kr != kIOReturnSuccess) || (!upsDataRef->upsCapabilities)) {
				goto CLEANUP_PARTIAL;
			}
			
			kr = (*upsPlugInInterface)->getEvent(upsPlugInInterface, &upsDataRef->upsEvent);
			if ((kr != kIOReturnSuccess) || (!upsDataRef->upsEvent)) {
				goto CLEANUP_PARTIAL;
			}

			kr = IOServiceAddInterestNotification(gNotifyPort, upsDevice, "IOGeneralInterest", DeviceNotification, upsDataRef, &(upsDataRef->notification));
			if (kr != kIOReturnSuccess)
				goto CLEANUP_PARTIAL;
			
			if (!gAllUPSDevices) {
				gAllUPSDevices = CFSetCreateMutable(kCFAllocatorDefault, 0, &kCFTypeSetCallBacks);
			}
			CFSetAddValue(gAllUPSDevices, upsDataRef);
			
			IOObjectRelease(upsDevice);
			continue;
		}
		
	CLEANUP_PARTIAL:
		// (same cleanup logic as before)
		if (upsDataRef) {
			if (upsDataRef->notification != MACH_PORT_NULL) {
				IOObjectRelease(upsDataRef->notification);
			}
			if (upsDataRef->upsPlugInInterface) {
				(*upsDataRef->upsPlugInInterface)->Release(upsDataRef->upsPlugInInterface);
			}
			if (upsDataRef->upsProperties)     CFRelease(upsDataRef->upsProperties);
			if (upsDataRef->upsCapabilities)   CFRelease(upsDataRef->upsCapabilities);
			if (upsDataRef->upsEvent)          CFRelease(upsDataRef->upsEvent);
			if (upsDataRef->upsEventSource) {
				CFRunLoopRemoveSource(CFRunLoopGetCurrent(), upsDataRef->upsEventSource, kCFRunLoopDefaultMode);
				CFRelease(upsDataRef->upsEventSource);
			}
			if (upsDataRef->upsEventTimer) {
				CFRunLoopRemoveTimer(CFRunLoopGetCurrent(), upsDataRef->upsEventTimer, kCFRunLoopDefaultMode);
				CFRelease(upsDataRef->upsEventTimer);
			}
			free(upsDataRef);
			upsDataRef = NULL;
		}
		if (plugInInterface) {
			(*plugInInterface)->Release(plugInInterface);
		}
		IOObjectRelease(upsDevice);
	}
}

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
			CFIndex n = CFSetGetCount(gAllUPSDevices);
			if (n > 0) {
				UPSDataRef *buffer = calloc(n, sizeof(UPSDataRef));
				CFSetGetValues(gAllUPSDevices, (const void **)buffer);
				for (CFIndex i = 0; i < n; i++) {
					FreeUPSData(buffer[i]);
				}
				free(buffer);
			}
			CFRelease(gAllUPSDevices);
			gAllUPSDevices = NULL;
		}

		if (gNotifyPort) {
			IONotificationPortDestroy(gNotifyPort);
			gNotifyPort = NULL;
		}
	}
}

void CleanupAndExit(void) {
	CFRunLoopStop(CFRunLoopGetCurrent());
}

void SignalHandler(int sigraised) {
	syslog(LOG_INFO, "Battman: exiting\n");
	CleanupAndExit();
}

static bool gNotificationsPaused = false;

+ (void)appDidEnterBackground:(NSNotification *)note
{
	if (gBackgroundRunLoop == NULL || gNotifyPort == NULL || gNotificationsPaused)
		return;
	CFRunLoopSourceRef src = IONotificationPortGetRunLoopSource(gNotifyPort);
	if (!src) return;
	
	CFRunLoopPerformBlock(gBackgroundRunLoop, kCFRunLoopDefaultMode, ^{
		CFRunLoopRemoveSource(CFRunLoopGetCurrent(), src, kCFRunLoopDefaultMode);
	});
	CFRunLoopWakeUp(gBackgroundRunLoop);
	
	gNotificationsPaused = true;
}

+ (void)appWillEnterForeground:(NSNotification *)note
{
	if (gBackgroundRunLoop == NULL || gNotifyPort == NULL || !gNotificationsPaused)
		return;
	CFRunLoopSourceRef src = IONotificationPortGetRunLoopSource(gNotifyPort);
	if (!src) return;
	
	CFRunLoopPerformBlock(gBackgroundRunLoop, kCFRunLoopDefaultMode, ^{
		CFRunLoopAddSource(CFRunLoopGetCurrent(), src, kCFRunLoopDefaultMode);
	});
	CFRunLoopWakeUp(gBackgroundRunLoop);
	
	gNotificationsPaused = false;
}


+ (void)startWatchingUPS
{
	if (UPSWatching) {
		return;
	}

	NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
	[nc addObserver:self selector:@selector(appDidEnterBackground:) name:UIApplicationDidEnterBackgroundNotification object:nil];
	[nc addObserver:self selector:@selector(appWillEnterForeground:) name:UIApplicationWillEnterForegroundNotification object:nil];

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
		NSLog(@"[UPSMonitor] UPS‐watch thread launched.");
		UPSWatching = true;
	}
}

@end
