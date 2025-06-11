#include "../common.h"
#include "../iokitextern.h"
#include <CoreFoundation/CFDictionary.h>
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFNotificationCenter.h>
#include <stdint.h>
#include <stdlib.h>

#if __has_include(<IOKit/IOKitKeys.h>)
#include <IOKit/IOKitKeys.h>
#else
#define kIOFirstMatchNotification "IOServiceFirstMatch"
#define kIOGeneralInterest "IOGeneralInterest"
#endif

#if __has_include(<dispatch/dispatch.h>)
#include <dispatch/dispatch.h>
#else
extern void *dispatch_get_global_queue(int, int);
#endif

dispatch_queue_t   _powerQueue = NULL;
static IONotificationPortRef  _notifyPort;
static io_iterator_t      _notifyIter;

static void stpe_cb(void *cb, io_iterator_t it) {
	if (!it) return;
	io_object_t next;
	while ((next = IOIteratorNext(it))) {
		void *refCon = NULL;
		int err = IOServiceAddInterestNotification(_notifyPort, next, kIOGeneralInterest, (IOServiceInterestCallback)cb, NULL, (void *)&refCon);
		if (err) abort();
		IOObjectRelease(next);
	}
}


void subscribeToPowerEvents(void (*cb)(int, io_registry_entry_t, int32_t)) {
	_powerQueue = dispatch_queue_create("com.torrekie.Battman.pmEvents", DISPATCH_QUEUE_SERIAL);
	_notifyPort = IONotificationPortCreate(kIOMasterPortDefault);
	// Alternative dispatch queue for suspending, check UPSMonitor.m
	IONotificationPortSetDispatchQueue(_notifyPort, _powerQueue);

	int err = IOServiceAddMatchingNotification(_notifyPort, kIOFirstMatchNotification, IOServiceMatching("IOPMPowerSource"), (IOServiceMatchingCallback)stpe_cb, cb, &_notifyIter);
	if (err) abort();

	stpe_cb(cb, _notifyIter);

	IOObjectRelease(_notifyIter);
}

#if 0
// IOServiceMatchingCallback
static void stpe_cb(void **pcb, io_iterator_t it) {
	if (!it)
		return;
	io_object_t next;
	while ((next = IOIteratorNext(it))) {
		void *buf;
		int err = IOServiceAddInterestNotification(*pcb, next, kIOGeneralInterest, (IOServiceInterestCallback)pcb[1], 0, (void *)&buf);
		if (err)
			abort();
		IOObjectRelease(next);
	}
}

void subscribeToPowerEvents(void (*cb)(int, io_registry_entry_t, int32_t)) {
    void *port[] = {IONotificationPortCreate(0), cb};
    IONotificationPortSetDispatchQueue(*port, dispatch_get_global_queue(0, 0));
    io_iterator_t nit = 0;
    int err = IOServiceAddMatchingNotification(*port, kIOFirstMatchNotification, IOServiceMatching("IOPMPowerSource"), (IOServiceMatchingCallback)stpe_cb, port, &nit);
    if (err)
        abort();
    stpe_cb(port, nit);
    IOObjectRelease(nit);
}
#endif

void pmncb(int a, io_registry_entry_t b, int32_t c) {
	if (c != -536723200)
		return;
	CFMutableDictionaryRef props;
	int ret=IORegistryEntryCreateCFProperties(b,&props,0,0);
	if(ret!=0) {
		NSLog(CFSTR("Failed to get CFProperties from notification"));
		return;
	}
	//CFStringRef desc=CFCopyDescription(props);
	//CFRelease(props);
	//NSLog(CFSTR("Power Update: %@"),desc);
	//show_alert("Power",CFStringGetCStringPtr(desc,0x08000100),"ok");
	//CFRelease(desc);
	CFNotificationCenterPostNotification(CFNotificationCenterGetLocalCenter(), CFSTR("SMC60000"), NULL, props, 1);
	CFRelease(props);
}

__attribute__((constructor)) static void startpmn() { subscribeToPowerEvents(pmncb); }
