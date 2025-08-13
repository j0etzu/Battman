#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#include "common.h"
#import "DonationViewController.h"
#import "DonationPrompter.h"

@interface NSProcessInfo ()
- (NSDate *)processStartTime;
@end

static NSTimeInterval const kBattmanDonationPushRequiredDays = 3; // >= 3 days since first launch
static NSUInteger const kBattmanDonationPushRequiredLaunches = 3; // > 3 launches
static NSTimeInterval const kBattmanDonationPushAboutSeconds = 60; // only within 60s after process start
static double const kBattmanDonationPushSamplingRate = 1.0; // Set <1.0 to sample fewer users (e.g., 0.3)

extern id find_top_controller(id rootVC);
extern id gWindow;

BOOL donation_shown(void) {
	return [[NSUserDefaults standardUserDefaults] boolForKey:(__bridge NSString *)kBattmanDonateShownKey];
}

// Call from main thread pls
void donation_prompter_request_check(void) {
	if (donation_shown())
		return;

	NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];

	// write vals
	NSInteger count = [ud integerForKey:(__bridge NSString *)kBattmanLaunchCountKey];
	count += 1;
	[ud setInteger:count forKey:(__bridge NSString *)kBattmanLaunchCountKey];
	if (![ud objectForKey:(__bridge NSString *)kBattmanFirstLaunchKey]) {
		NSTimeInterval t = CFAbsoluteTimeGetCurrent();
		[ud setDouble:t forKey:(__bridge NSString *)kBattmanFirstLaunchKey];
		goto save_config;
	}

	NSTimeInterval first = [ud doubleForKey:(__bridge NSString *)kBattmanFirstLaunchKey];
	if (first <= 0)
		goto save_config;

	if (CFAbsoluteTimeGetCurrent() - first < kBattmanDonationPushRequiredDays * 24.0 * 3600.0)
		goto save_config;

	if ([ud integerForKey:(__bridge NSString *)kBattmanLaunchCountKey] < (NSInteger)kBattmanDonationPushRequiredLaunches)
		goto save_config;

	if (kBattmanDonationPushSamplingRate < 1.0) {
		// use a simple deterministic float in [0,1)
		uint64_t v = (uint64_t)((uint64_t)first * 1000003ULL);
		double r = (double)(v % 1000000ULL) / 1000000.0;
		if (r >= kBattmanDonationPushSamplingRate)
			goto save_config;
	}
	if (CFAbsoluteTimeGetCurrent() - [[(id)[NSProcessInfo processInfo] processStartTime] timeIntervalSince1970] > kBattmanDonationPushAboutSeconds)
		goto save_config;
	
	dispatch_async(dispatch_get_main_queue(), ^{
		// verify UI is ready and not presenting another modal
		UIViewController *topController = find_top_controller([gWindow rootViewController]);
		if (!topController)
			return;

		dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
			// Another defensive check: if top VC changed to a modal in the meantime don't show
			UIViewController *top2 = find_top_controller([gWindow rootViewController]);
			if (!top2 || top2.presentedViewController) return;

			// Mark shown before calling (so reentrancy / crashes won't re-trigger)
			[[NSUserDefaults standardUserDefaults] setBool:YES forKey:(__bridge NSString *)kBattmanDonateShownKey];
			[[NSUserDefaults standardUserDefaults] synchronize];

			show_donation(false);
		});
	});

save_config:
	[ud synchronize];
	return;
}

__attribute__((constructor)) static void _donation_run() {
	CFNotificationCenterRef nc = CFNotificationCenterGetLocalCenter();
	CFNotificationCenterAddObserver(nc, NULL, (CFNotificationCallback)donation_prompter_request_check, (__bridge CFStringRef)UIApplicationDidFinishLaunchingNotification, NULL, CFNotificationSuspensionBehaviorDrop);
	CFNotificationCenterAddObserver(nc, NULL, (CFNotificationCallback)donation_prompter_request_check, (__bridge CFStringRef)UIApplicationDidBecomeActiveNotification, NULL, CFNotificationSuspensionBehaviorDrop);
}

void show_donation(bool manual) {
	static bool manually_opened = false;
	manually_opened = manual;
	dispatch_async(dispatch_get_main_queue(), ^{
		UIViewController *top = find_top_controller([gWindow rootViewController]);
		if (!top)
			return;

		// make sure we are not already presenting something heavy
		if (top.presentedViewController)
			return;

		DonationViewController *vc = [[DonationViewController alloc] initWithFlag:manually_opened];
		vc.modalPresentationStyle = UIModalPresentationPageSheet;

		UINavigationController *nav = [[UINavigationController alloc] initWithRootViewController:vc];
		nav.modalPresentationStyle = UIModalPresentationPageSheet;
		[top presentViewController:nav animated:YES completion:nil];
	});
}
