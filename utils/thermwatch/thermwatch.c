#include <CoreFoundation/CoreFoundation.h>
#include <libkern/OSThermalNotification.h>

static uint32_t loop_count = 0;
static uint32_t counter = 0;

void callback(CFNotificationCenterRef center, void *observer, CFNotificationName name, const void *object, CFDictionaryRef userInfo) {
    time_t current_time;

    current_time = time(0);
    fprintf(stderr, "level = %d: %s", OSThermalNotificationCurrentLevel(), ctime(&current_time));
	if (loop_count) {
        if (++counter >= loop_count) {
            CFNotificationCenterRemoveEveryObserver(center, NULL);
            CFRunLoopStop(CFRunLoopGetCurrent());
        }
    }
}

int main(int argc, char *argv[]) {
	if (argc == 2 && !strcmp("--loop", argv[1]))
		loop_count = atoi(argv[2]);

	if (argc < 2) {
		CFStringRef name = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, kOSThermalNotificationName, kCFStringEncodingUTF8, kCFAllocatorNull);
		CFNotificationCenterAddObserver(CFNotificationCenterGetDarwinNotifyCenter(), NULL, (CFNotificationCallback)callback, name, NULL, CFNotificationSuspensionBehaviorDeliverImmediately);
		CFRelease(name);
		CFRunLoopRun();
		return EXIT_SUCCESS;
	}

	fwrite("usage: --loop XX | Where XX is a number\n", 40, 1, stderr);
	return EXIT_SUCCESS;
}
