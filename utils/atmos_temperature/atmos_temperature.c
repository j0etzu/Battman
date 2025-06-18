#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDServiceClient.h>
#include <IOKit/hid/IOHIDEventSystemClient.h>
#include <IOKit/hid/IOHIDUsageTables.h>
#include <IOKit/hid/IOHIDDeviceKeys.h>
#include <stdlib.h>
#include <unistd.h>

typedef CF_ENUM(UInt32, IOHIDEventSystemClientType) {
	kIOHIDEventSystemClientTypeMonitor = 1,
};

#define kAppleTemperatureDictionaryKey "AppleVoltageDictionary"

extern int IOHIDEventSystemClientSetMatching(IOHIDEventSystemClientRef client, CFDictionaryRef match);
extern IOHIDEventSystemClientRef IOHIDEventSystemClientCreateWithType(CFAllocatorRef allocator, IOHIDEventSystemClientType type, void *whatisthat);

int main() {
	int ret = 1;
	CFArrayRef services = NULL;
	void *keys[2], *values[2];
	int32_t usage, page;
	IOHIDEventSystemClientRef sysclient;
	CFNumberRef cfUsage, cfPage;
	CFDictionaryRef matchingDict;
	int i;
	CFIndex count;

	sysclient = IOHIDEventSystemClientCreateWithType(kCFAllocatorDefault, kIOHIDEventSystemClientTypeMonitor, NULL);
	if (!sysclient) {
		fprintf(stderr, "IOHID error\n");
		exit(1);
	}

	keys[0] = (void *)CFSTR(kIOHIDPrimaryUsagePageKey);
	keys[1] = (void *)CFSTR(kIOHIDPrimaryUsageKey);

	page = kHIDPage_Sensor;
	usage = kHIDUsage_Snsr_Environmental_AtmosphericPressure;

	cfPage = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &page);
	cfUsage = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &usage);
	values[0] = (void *)cfPage;
	values[1] = (void *)cfUsage;

	matchingDict = CFDictionaryCreate(kCFAllocatorDefault, (const void **)keys, (const void **)values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	if (!matchingDict) {
		fprintf(stderr, "CF error\n");
		goto err_exit;
	}

	IOHIDEventSystemClientSetMatching(sysclient, matchingDict);
	services = IOHIDEventSystemClientCopyServices(sysclient);
	if (!services || (count = CFArrayGetCount(services), count < 1)) {
		fprintf(stderr, "No Atmospheric Pressure Sensor on this device.\n");
		goto err_exit;
	}
	printf("Sensor count: %ld\n", count);

	i = 0;
	while (i < count) {
		IOHIDServiceClientRef service = (IOHIDServiceClientRef)CFArrayGetValueAtIndex(services, i);
		CFTypeRef buffer = IOHIDServiceClientCopyProperty(service, CFSTR(kIOHIDLocationIDKey));
		if (buffer) {
			if (CFStringGetTypeID() == CFGetTypeID(buffer)) {
				const char *location = CFStringGetCStringPtr((CFStringRef)buffer, kCFStringEncodingUTF8);
				printf("Sensor[%d]: %s", i, location);
			} else if (CFNumberGetTypeID() == CFGetTypeID(buffer)) {
				unsigned long locID = 0;
				CFNumberGetValue((CFNumberRef)buffer, kCFNumberLongType, &locID);
				unsigned int foo = htonl(locID);
				char* fourCC = (char*)&foo;
				fourCC[4] = '\0';
				printf("Sensor[%d]: 0x%lx (%s)", i, locID, fourCC);
			} else {
				printf("Sensor[%d]: ", i);
				CFShow(buffer);
			}
			CFRelease(buffer);
		}
		CFDictionaryRef dict = IOHIDServiceClientCopyProperty(service, CFSTR(kAppleTemperatureDictionaryKey));
		if (dict) {
			CFNumberRef number = CFDictionaryGetValue(dict, CFSTR("PRESSURE_TEMP"));
			if (number) {
				long tempC = 0;
				CFNumberGetValue(number, kCFNumberLongType, &tempC);
				printf(", Temperature: %.2f C", (float)tempC / 100.0f);
			}
			CFRelease(dict);
		}
		putchar('\n');
		i++;
	}
	// You can also get Samples/PressureLevel in a IOHIDEventSystemClientRegisterEventCallback, but not demonstrated in this program

err_exit:
	CFRelease(cfUsage);
	CFRelease(cfPage);
	if (services) CFRelease(services);

	return ret;
}
