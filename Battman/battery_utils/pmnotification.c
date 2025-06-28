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
static IONotificationPortRef  _notifyPort = NULL;
static io_iterator_t      _notifyIter = MACH_PORT_NULL;
static bool _powerMonitoringSuspended = false;
static bool _powerMonitoringInitialized = false;

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

// FIXME: Prevent overstock of pending notifications when App UI suspended

void suspendPowerEventMonitoring(void) {
	if (!_powerMonitoringInitialized || _powerQueue == NULL || _notifyPort == NULL || _powerMonitoringSuspended) {
		return;
	}
	
	// Remove the notification port from the dispatch queue
	IONotificationPortSetDispatchQueue(_notifyPort, NULL);
	
	// Suspend the dispatch queue
	dispatch_suspend(_powerQueue);
	
	_powerMonitoringSuspended = true;
	os_log_info(gLog, "[pmnotification] Power event monitoring suspended");
}

void resumePowerEventMonitoring(void) {
	if (!_powerMonitoringInitialized || _powerQueue == NULL || _notifyPort == NULL || !_powerMonitoringSuspended) {
		return;
	}
	
	// Resume the dispatch queue
	dispatch_resume(_powerQueue);
	
	// Re-attach the notification port to the dispatch queue
	IONotificationPortSetDispatchQueue(_notifyPort, _powerQueue);
	
	_powerMonitoringSuspended = false;
	os_log_info(gLog, "[pmnotification] Power event monitoring resumed");
}

// Enhanced cleanup function for proper shutdown
void cleanupPowerEventMonitoring(void) {
	if (!_powerMonitoringInitialized) {
		return;
	}
	
	os_log_info(gLog, "[pmnotification] Starting power monitoring cleanup");
	
	// Resume queue if it was suspended to allow proper cleanup
	if (_powerMonitoringSuspended && _powerQueue) {
		dispatch_resume(_powerQueue);
		_powerMonitoringSuspended = false;
	}
	
	// Clean up IOKit resources
	if (_notifyIter != MACH_PORT_NULL) {
		IOObjectRelease(_notifyIter);
		_notifyIter = MACH_PORT_NULL;
	}
	
	if (_notifyPort) {
		// Remove from dispatch queue first
		IONotificationPortSetDispatchQueue(_notifyPort, NULL);
		IONotificationPortDestroy(_notifyPort);
		_notifyPort = NULL;
	}
	
	// Clean up dispatch queue
	if (_powerQueue) {
		// Dispatch a final cleanup block and then release the queue
		dispatch_async(_powerQueue, ^{
			os_log_debug(gLog, "[pmnotification] Final cleanup block executed");
		});
		
		// Don't release the queue immediately as it might still have pending blocks
		// Just set it to NULL - the system will clean it up when appropriate
		_powerQueue = NULL;
	}
	
	_powerMonitoringInitialized = false;
	os_log_info(gLog, "[pmnotification] Power event monitoring cleanup completed");
}

void subscribeToPowerEvents(void (*cb)(int, io_registry_entry_t, int32_t)) {
	if (_powerQueue != NULL) {
		// Already initialized
		return;
	}
	
	_powerQueue = dispatch_queue_create("com.torrekie.Battman.pmEvents", DISPATCH_QUEUE_SERIAL);
	if (_powerQueue == NULL) {
		os_log_error(gLog, "[pmnotification] Failed to create dispatch queue");
		return;
	}

	_notifyPort = IONotificationPortCreate(kIOMasterPortDefault);
	if (_notifyPort == NULL) {
		os_log_error(gLog, "[pmnotification] Failed to create IONotificationPort");
		return;
	}

	// Set up dispatch queue for notifications
	IONotificationPortSetDispatchQueue(_notifyPort, _powerQueue);
	
	int err = IOServiceAddMatchingNotification(_notifyPort, kIOFirstMatchNotification, IOServiceMatching("IOPMPowerSource"), (IOServiceMatchingCallback)stpe_cb, cb, &_notifyIter);
	if (err) {
		os_log_error(gLog, "[pmnotification] Failed to add matching notification: %d", err);
		cleanupPowerEventMonitoring();
		return;
	}
	
	// Process any existing power sources
	stpe_cb(cb, _notifyIter);
	_powerMonitoringInitialized = true;
	os_log_info(gLog, "[pmnotification] Power event monitoring started successfully");
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
	// Check if we're in the middle of shutdown
	if (!_powerMonitoringInitialized) {
		return;
	}

	if (c != -536723200)
		return;
	CFMutableDictionaryRef props;
	int ret = IORegistryEntryCreateCFProperties(b, &props, 0, 0);
	if (ret != 0) {
		os_log_error(gLog, "[pmnotification] Failed to get CFProperties from notification");
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
__attribute__((destructor)) static void stoppmn() { cleanupPowerEventMonitoring(); }
