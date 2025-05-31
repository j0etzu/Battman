#include "cobjc/cobjc.h"

void BSVCBatteryStatusDidUpdateWithInfo(id self, void *emptyRef, CFDictionaryRef info) {
	ocall(self, batteryStatusDidUpdate);
}

static void BSVCBatteryStatusCallback1(void **userInfo) {
	ocall(userInfo[0], batteryStatusDidUpdate:, userInfo[1]);
}

static void BSVCBatteryStatusCallback(CFNotificationCenterRef center, void *observer, CFNotificationName name, const void *object, CFDictionaryRef userInfo) {
	void *observerAndUserInfo[2] = {
		observer,
		(void *)userInfo
	};
	dispatch_sync_f(dispatch_get_main_queue(), observerAndUserInfo, (void (*)(void *))BSVCBatteryStatusCallback1);
	// stack variable is ok bc it waits until execution finishes
}

void BSVCBatteryStatusDidUpdate(id self) {
	UITableViewReloadData(UITableViewControllerGetTableView(self));
}

void BSVCViewDidDisappear(id self, void *er, BOOL animated) {
	osupercall(BatterySubscriberViewControllerBase, self, viewDidDisappear:, animated);
	CFNotificationCenterRemoveObserver(CFNotificationCenterGetLocalCenter(), self, CFSTR("SMC60000"), NULL);
}

void BSVCViewDidAppear(id self, void *er, BOOL animated) {
	osupercall(BatterySubscriberViewControllerBase, self, viewDidAppear:, animated);
	CFNotificationCenterAddObserver(CFNotificationCenterGetLocalCenter(), self, BSVCBatteryStatusCallback, CFSTR("SMC60000"), NULL, 1);
}

MAKE_CLASS(BatterySubscriberViewControllerBase, UITableViewController, 0,
    ,
    BSVCBatteryStatusDidUpdateWithInfo, batteryStatusDidUpdate:,
    BSVCBatteryStatusDidUpdate, batteryStatusDidUpdate,
    BSVCViewDidDisappear, viewDidDisappear:,
    BSVCViewDidAppear, viewDidAppear:
)
