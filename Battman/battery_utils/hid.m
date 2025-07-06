#import <Foundation/Foundation.h>

#if __has_include(<IOKit/hid/IOHIDDeviceKeys.h>)
#include <IOKit/hid/IOHIDDeviceKeys.h>
#else
#define kIOHIDProductKey "Product"
#define kIOHIDLocationIDKey "LocationID"
#define kIOHIDPrimaryUsagePageKey "PrimaryUsagePage"
#define kIOHIDPrimaryUsageKey "PrimaryUsage"
#endif

#if __has_include(<IOKit/hid/IOHIDUsageTables.h>)
#include <IOKit/hid/IOHIDUsageTables.h>
#else
#define kHIDPage_Sensor 0x20
#define kHIDUsage_Snsr_Environmental_AtmosphericPressure 0x31
#endif

#if __has_include(<IOKit/hid/AppleHIDUsageTables.h>)
#include <IOKit/hid/AppleHIDUsageTables.h>
#else
#define kHIDPage_AppleVendor 0xFF00
#define kHIDUsage_AppleVendor_TemperatureSensor 0x0005
#define kHIDUsage_AppleVendor_Accelerometer 0x03
#define kHIDUsage_AppleVendor_Gyro 0x09
#define kHIDUsage_AppleVendor_Compass 0x0A
#define kHIDUsage_AppleVendor_Jarvis 0x3E
#endif

#if __has_include(<IOKit/hid/IOHIDEventTypes.h>)
#include <IOKit/hid/IOHIDEventTypes.h>
#else
#define kIOHIDEventTypeTemperature 15
#define kIOHIDEventFieldTemperatureLevel (kIOHIDEventTypeTemperature << 16)
#endif

#if __has_include(<IOKit/hid/IOHIDEvent.h>)
#include <IOKit/hid/IOHIDEvent.h>
#else
CF_IMPLICIT_BRIDGING_ENABLED
#ifdef __LP64__
typedef double IOHIDFloat;
#else
typedef float IOHIDFloat;
#endif
typedef struct __IOHIDEvent * IOHIDEventRef;
typedef uint32_t IOHIDEventField;

extern IOHIDFloat IOHIDEventGetFloatValue(IOHIDEventRef, IOHIDEventField);
CF_IMPLICIT_BRIDGING_DISABLED
#endif

#if __has_include(<IOKit/hidsystem/IOHIDEventSystemClient.h>)
#include <IOKit/hidsystem/IOHIDEventSystemClient.h>
#else
CF_IMPLICIT_BRIDGING_ENABLED
typedef struct CF_BRIDGED_TYPE(id) __IOHIDEventSystemClient * IOHIDEventSystemClientRef;

extern CFArrayRef IOHIDEventSystemClientCopyServices(IOHIDEventSystemClientRef);
CF_IMPLICIT_BRIDGING_DISABLED
#endif

// Sadly these are SPI
#if __has_include(<IOKit/hid/IOHIDEventSystemClient.h>) && 0
#include <IOKit/hid/IOHIDEventSystemClient.h>
#else
CF_IMPLICIT_BRIDGING_ENABLED
extern IOHIDEventSystemClientRef IOHIDEventSystemClientCreate(CFAllocatorRef);
extern void IOHIDEventSystemClientSetMatching(IOHIDEventSystemClientRef, CFDictionaryRef);
extern IOHIDEventSystemClientRef IOHIDEventSystemClientCreateWithType(CFAllocatorRef allocator, uint32_t client_type, CFDictionaryRef attrs);
CF_IMPLICIT_BRIDGING_DISABLED
#endif

#if __has_include(<IOKit/hidsystem/IOHIDServiceClient.h>)
#include <IOKit/hidsystem/IOHIDServiceClient.h>
#else
CF_IMPLICIT_BRIDGING_ENABLED
typedef struct CF_BRIDGED_TYPE(id) __IOHIDServiceClient * IOHIDServiceClientRef;

extern CFTypeRef _Nullable IOHIDServiceClientCopyProperty(IOHIDServiceClientRef service, CFStringRef key);
CF_IMPLICIT_BRIDGING_DISABLED
#endif
// Should be in IOHIDServiceClient.h but nope
CF_IMPLICIT_BRIDGING_ENABLED
extern IOHIDEventRef IOHIDServiceClientCopyEvent(IOHIDServiceClientRef service, int64_t type, int32_t options, int64_t timestamp);
CF_IMPLICIT_BRIDGING_DISABLED

BOOL NSStringEquals4CC(uint32_t fourcc, NSString *string) {
	if ([string length] != 4) {
		return NO;
	}
	
	// Convert fourcc to 4-character C string
	char fourccStr[5] = {
		(char)((fourcc >> 24) & 0xFF),
		(char)((fourcc >> 16) & 0xFF),
		(char)((fourcc >> 8) & 0xFF),
		(char)(fourcc & 0xFF),
		'\0'
	};
	
	return [string isEqualToString:[NSString stringWithCString:fourccStr encoding:NSUTF8StringEncoding]];
}

// XXX: Consider migrate to pure C
NSDictionary *getTemperatureHIDData(void) {
    IOHIDEventSystemClientRef client = IOHIDEventSystemClientCreate(kCFAllocatorDefault);
	if (!client)
		return nil;

    NSDictionary *matching = @{
        @kIOHIDPrimaryUsagePageKey: @(kHIDPage_AppleVendor),
        @kIOHIDPrimaryUsageKey: @(kHIDUsage_AppleVendor_TemperatureSensor),
    };
    IOHIDEventSystemClientSetMatching(client, (__bridge CFDictionaryRef)matching);

	NSArray *ret = (__bridge NSArray *)IOHIDEventSystemClientCopyServices(client);
	NSMutableDictionary *dict = [NSMutableDictionary dictionary];
	for (id client in ret) {
		NSString *prod = (__bridge NSString *)IOHIDServiceClientCopyProperty((IOHIDServiceClientRef)client, CFSTR(kIOHIDProductKey));
		if (!prod)
			continue;
        IOHIDEventRef event = IOHIDServiceClientCopyEvent((IOHIDServiceClientRef)client, kIOHIDEventTypeTemperature, 0, 0);
		if (!event)
			continue;
		dict[prod] = [NSNumber numberWithDouble:IOHIDEventGetFloatValue(event, kIOHIDEventFieldTemperatureLevel)];
	}
	CFRelease(client);
	return dict;
}

float getTemperatureHIDAt(NSString *locID) {
	IOHIDEventSystemClientRef client = IOHIDEventSystemClientCreate(kCFAllocatorDefault);
	if (!client)
		return -1;
	
	NSDictionary *matching = @{
		@kIOHIDPrimaryUsagePageKey: @(kHIDPage_AppleVendor),
		@kIOHIDPrimaryUsageKey: @(kHIDUsage_AppleVendor_TemperatureSensor),
	};
	IOHIDEventSystemClientSetMatching(client, (__bridge CFDictionaryRef)matching);

	float temp = -1;
	NSArray *ret = (__bridge NSArray *)IOHIDEventSystemClientCopyServices(client);
	for (id client in ret) {
		NSNumber *location = (__bridge NSNumber *)IOHIDServiceClientCopyProperty((IOHIDServiceClientRef)client, CFSTR(kIOHIDLocationIDKey));
		if (!location || !NSStringEquals4CC([location unsignedIntValue], locID))
			continue;
		IOHIDEventRef event = IOHIDServiceClientCopyEvent((IOHIDServiceClientRef)client, kIOHIDEventTypeTemperature, 0, 0);
		if (!event)
			continue;
		
		temp = IOHIDEventGetFloatValue(event, kIOHIDEventFieldTemperatureLevel);
	}
	CFRelease(client);
	return temp;
}

static int GetTemperatureFromDict(CFDictionaryRef dict) {
	if (dict == NULL) {
		return -1;
	}

	// kAppleTemperatureDictionaryKey normally only one member
	CFIndex count = CFDictionaryGetCount(dict);
	if (count != 1) {
		return -1;
	}

	const void *keys[1];
	const void *values[1];
	CFDictionaryGetKeysAndValues(dict, keys, values);
	
	CFNumberRef num = (CFNumberRef)values[0];
	if (num == NULL || CFGetTypeID(num) != CFNumberGetTypeID()) {
		return -1;
	}
	
	int result = 0;
	if (!CFNumberGetValue(num, kCFNumberIntType, &result)) {
		return -1;
	}
	
	return result;
}

float getSensorTemperature(int page, int usage) {
	// kIOHIDEventSystemClientTypeMonitor
	IOHIDEventSystemClientRef client = IOHIDEventSystemClientCreateWithType(kCFAllocatorDefault, 1, NULL);
	if (!client) return -1;

	float ret = -1;
	void *keys[2], *values[2];
	CFNumberRef cfPage, cfUsage;
	CFArrayRef services = NULL;
	keys[0] = (void *)CFSTR(kIOHIDPrimaryUsagePageKey);
	keys[1] = (void *)CFSTR(kIOHIDPrimaryUsageKey);\
	
	cfPage = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &page);
	cfUsage = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &usage);
	values[0] = (void *)cfPage;
	values[1] = (void *)cfUsage;
	
	CFDictionaryRef matchingDict = CFDictionaryCreate(kCFAllocatorDefault, (const void **)keys, (const void **)values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	if (matchingDict) {
		IOHIDEventSystemClientSetMatching(client, matchingDict);
		CFRelease(matchingDict);

		services = IOHIDEventSystemClientCopyServices(client);
		CFIndex count;
		if (services && ((void)(count = CFArrayGetCount(services)), count > 0)) {
			int i = 0;
			ret = 0;
			// Even we do a loop here, the count should always be one
			while (i < count) {
				IOHIDServiceClientRef service = (IOHIDServiceClientRef)CFArrayGetValueAtIndex(services, i);
				// kAppleTemperatureDictionaryKey
				CFDictionaryRef dict = IOHIDServiceClientCopyProperty(service, CFSTR("AppleVoltageDictionary"));
				if (dict) {
					int num = GetTemperatureFromDict(dict);
					if (num != -1) {
						ret += (float)num / 100.0f;
					}
					CFRelease(dict);
				}
				i++;
			}
			ret /= count;
			CFRelease(services);
		}
	}

	CFRelease(cfPage);
	CFRelease(cfUsage);
	CFRelease(client);

	return ret;
}

static int sensorUsages[] = {
	kHIDUsage_AppleVendor_Accelerometer,
	kHIDUsage_AppleVendor_Gyro,
	kHIDUsage_AppleVendor_Compass,
	/* kHIDUsage_AppleVendor_ProximitySensor some device seems having temp on this sensor */
	/* kHIDUsage_AppleVendor_Jarvis seems the legacy compass sensor */
	kHIDUsage_Snsr_Environmental_AtmosphericPressure,
	/* kHIDUsage_Snsr_Environmental_Humidity seems only on HomePods, we can enable this once Battman running on HomePods */
};
#ifdef _C
#undef _C
#endif
#define _C(x) x
static char *sensorNames[] = {
	_C("Accelerometer"),
	_C("Gyroscope"),
	_C("Compass"),
	/* "Proximity Sensor" */
	/* "Compass (Jarvis)" */
	_C("Atmos Pressure Sensor"),
	/* "Humidity Sensor" */
};

NSDictionary *getSensorTemperatures(void) {
	NSMutableDictionary *dict = [NSMutableDictionary dictionary];
	
	@autoreleasepool {
		for (size_t i = 0; i < (sizeof(sensorUsages) / sizeof(sensorUsages[0])); i++) {
			int page = kHIDPage_AppleVendor;
			if (i > 2)
				page = kHIDPage_Sensor;
			
			float temp = getSensorTemperature(page, sensorUsages[i]);
			if (temp && temp != -1) {
				NSString *name = [NSString stringWithCString:sensorNames[i] encoding:NSUTF8StringEncoding];
				[dict setValue:[NSNumber numberWithDouble:temp] forKey:name];
			}
		}
	}
	
	return dict;
}

float getSensorAvgTemperature(void) {
	// These are all known sensors with temperatures on iOS devices (except kHIDUsage_AppleVendor_TemperatureSensor)
	// FIXME: use getSensorTemperatures
	float ret = 0;
	int cnt = 0;
	for (size_t i = 0; i < (sizeof(sensorUsages) / sizeof(sensorUsages[0])); i++) {
		int page = kHIDPage_AppleVendor;
		if (i > 2)
			page = kHIDPage_Sensor;

		float temp = getSensorTemperature(page, sensorUsages[i]);
		if (temp && temp != -1) {
			ret += temp;
			cnt++;
		}
	}
	if (cnt)
		return (ret /= cnt);

	return -1;
}
