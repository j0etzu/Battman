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
	if(donation_shown())
		return;
	NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];
	NSTimeInterval first = [ud doubleForKey:(__bridge NSString *)kBattmanFirstLaunchKey];
	if(first <= 0)
		return;
	NSTimeInterval elapsed = CFAbsoluteTimeGetCurrent() - first;
	NSTimeInterval required = kBattmanDonationPushRequiredDays * 24.0 * 3600.0;
	if(CFAbsoluteTimeGetCurrent() - first< kBattmanDonationPushRequiredDays * 24.0 * 3600.0)
		return;
	if([ud integerForKey:(__bridge NSString *)kBattmanLaunchCountKey]<(NSInteger)kBattmanDonationPushRequiredLaunches)
		return;
	if (kBattmanDonationPushSamplingRate<1.0) {
		// deterministic sampling: hash-based on install date so user is consistently sampled
		NSTimeInterval first = [ud doubleForKey:(__bridge NSString *)kBattmanFirstLaunchKey];
		if (first <= 0)
			return;
		// use a simple deterministic float in [0,1)
		uint64_t v = (uint64_t)((uint64_t)first * 1000003ULL);
		double r = (double)(v % 1000000ULL) / 1000000.0;
		if(r>=kBattmanDonationPushSamplingRate)
			return;
	}
	if(CFAbsoluteTimeGetCurrent() - [[(id)[NSProcessInfo processInfo] processStartTime] timeIntervalSince1970] > kBattmanDonationPushAboutSeconds)
		return;
	
	// verify UI is ready and not presenting another modal
	UIViewController *topController=find_top_controller([gWindow rootViewController]);
	if(!topController)
		return;
	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
		if(donation_shown())
			return;

		// Another defensive check: if top VC changed to a modal in the meantime don't show
		UIViewController *top2 =find_top_controller([gWindow rootViewController]);
		if (!top2 || top2.presentedViewController) return;

		// Mark shown before calling (so reentrancy / crashes won't re-trigger)
		[[NSUserDefaults standardUserDefaults] setBool:YES forKey:(__bridge NSString *)kBattmanDonateShownKey];
		[[NSUserDefaults standardUserDefaults] synchronize];

		show_donation();
	});
}

__attribute__((constructor)) static void _donation_run() {
	CFNotificationCenterRef nc=CFNotificationCenterGetLocalCenter();
	CFNotificationCenterAddObserver(nc,NULL,(CFNotificationCallback)donation_prompter_request_check,(__bridge CFStringRef)UIApplicationDidFinishLaunchingNotification,NULL,1);
	CFNotificationCenterAddObserver(nc,NULL,(CFNotificationCallback)donation_prompter_request_check,(__bridge CFStringRef)UIApplicationDidBecomeActiveNotification,NULL,1);
}

void show_donation(void) {
	dispatch_async(dispatch_get_main_queue(), ^{
		UIViewController *top = find_top_controller([gWindow rootViewController]);
		if(!top)
			return;

		// make sure we are not already presenting something heavy
		if(top.presentedViewController)
			return;

		DonationViewController *vc = [[DonationViewController alloc] init];
		vc.modalPresentationStyle = UIModalPresentationPageSheet;

		UINavigationController *nav = [[UINavigationController alloc] initWithRootViewController:vc];
		nav.modalPresentationStyle = UIModalPresentationPageSheet;
		[top presentViewController:nav animated:YES completion:nil];
	});
}
