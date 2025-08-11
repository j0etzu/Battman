#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#include "common.h"
#import "DonationViewController.h"
#import "DonationPrompter.h"

static NSTimeInterval const kBattmanDonationPushRequiredDays = 3; // >= 3 days since first launch
static NSUInteger const kBattmanDonationPushRequiredLaunches = 3; // > 3 launches
static NSTimeInterval const kBattmanDonationPushAboutSeconds = 60; // only within 60s after process start
static double const kBattmanDonationPushSamplingRate = 1.0; // Set <1.0 to sample fewer users (e.g., 0.3)

extern id find_top_controller(id root);

@interface DonationPrompter : NSObject
@property (nonatomic, assign) NSTimeInterval processStartTime;
+ (instancetype)shared;
- (void)appDidFinishLaunching;
- (void)appDidBecomeActive;
- (void)evaluateAndShowIfEligible;
@end

@implementation DonationPrompter

+ (instancetype)shared {
	static DonationPrompter *g;
	static dispatch_once_t once;
	dispatch_once(&once, ^{
		g = [DonationPrompter new];
	});
	return g;
}

// consider avoid this design
+ (void)load {
	dispatch_async(dispatch_get_main_queue(), ^{
		DonationPrompter *mgr = [DonationPrompter shared];
		// Prepare process start time now (approx time app binary loaded on main thread)
		mgr.processStartTime = CFAbsoluteTimeGetCurrent();
		NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
		[nc addObserver:mgr selector:@selector(appDidFinishLaunching) name:UIApplicationDidFinishLaunchingNotification object:nil];
		[nc addObserver:mgr selector:@selector(appDidBecomeActive) name:UIApplicationDidBecomeActiveNotification object:nil];
	});
}

#pragma mark - Launch bookkeeping

- (void)appDidFinishLaunching {
	NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];
	
	// first launch date
	if (![ud objectForKey:(__bridge NSString *)kBattmanFirstLaunchKey]) {
		// save CFAbsoluteTime as double to avoid timezone issues
		NSTimeInterval t = CFAbsoluteTimeGetCurrent();
		[ud setDouble:t forKey:(__bridge NSString *)kBattmanFirstLaunchKey];
	}
	
	// increment launch count (counting process starts)
	NSInteger count = [ud integerForKey:(__bridge NSString *)kBattmanLaunchCountKey];
	count += 1;
	[ud setInteger:count forKey:(__bridge NSString *)kBattmanLaunchCountKey];
	[ud synchronize];
}

- (void)appDidBecomeActive {
	// We only attempt to show at app start and within the early window: check elapsed since processStartTime
	// If it's past the window, we will not attempt to show.
	NSTimeInterval elapsed = CFAbsoluteTimeGetCurrent() - self.processStartTime;
	if (elapsed > kBattmanDonationPushAboutSeconds) {
		// Too late in the launch; don't attempt.
		return;
	}
	// Evaluate eligibility and maybe show.
	[self evaluateAndShowIfEligible];
}

#pragma mark - Eligibility and presentation

- (BOOL)hasShownBefore {
	return [[NSUserDefaults standardUserDefaults] boolForKey:(__bridge NSString *)kBattmanDonateShownKey];
}

- (BOOL)meetsTimeCondition {
	NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];
	NSTimeInterval first = [ud doubleForKey:(__bridge NSString *)kBattmanFirstLaunchKey];
	if (first <= 0) return NO;
	NSTimeInterval elapsed = CFAbsoluteTimeGetCurrent() - first;
	NSTimeInterval required = kBattmanDonationPushRequiredDays * 24.0 * 3600.0;
	return (elapsed >= required);
}

- (BOOL)meetsLaunchesCondition {
	NSInteger launches = [[NSUserDefaults standardUserDefaults] integerForKey:(__bridge NSString *)kBattmanLaunchCountKey];
	return (launches > (NSInteger)kBattmanDonationPushRequiredLaunches);
}

- (BOOL)passesSampling {
	if (kBattmanDonationPushSamplingRate >= 1.0) return YES;
	// deterministic sampling: hash-based on install date so user is consistently sampled
	NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];
	NSTimeInterval first = [ud doubleForKey:(__bridge NSString *)kBattmanFirstLaunchKey];
	if (first <= 0) return NO;
	// use a simple deterministic float in [0,1)
	uint64_t v = (uint64_t)((uint64_t)first * 1000003ULL);
	double r = (double)(v % 1000000ULL) / 1000000.0;
	return (r < kBattmanDonationPushSamplingRate);
}

- (UIViewController *)topViewController {
	UIApplication *app = [UIApplication sharedApplication];
	UIViewController *root = nil;
	UIWindow *win = nil;
	// Search for a key window that has a rootViewController
	for (UIWindow *w in app.windows.reverseObjectEnumerator) {
		if (w.isKeyWindow && w.rootViewController) { win = w; break; }
	}
	if (!win) {
		// fallback: first window with root
		for (UIWindow *w in app.windows) {
			if (w.rootViewController) { win = w; break; }
		}
	}
	root = win.rootViewController ?: app.delegate.window.rootViewController;
	if (!root) return nil;
	
	UIViewController *top = root;
	while (top.presentedViewController) {
		top = top.presentedViewController;
	}
	// if nav controller, get topViewController; if tab, selected
	if ([top isKindOfClass:[UINavigationController class]]) {
		top = [(UINavigationController *)top topViewController] ?: top;
	} else if ([top isKindOfClass:[UITabBarController class]]) {
		top = [(UITabBarController *)top selectedViewController] ?: top;
	}
	return top;
}

- (void)evaluateAndShowIfEligible {
	// quick checks
	if ([self hasShownBefore]) return;
	if (![self meetsTimeCondition]) return;
	if (![self meetsLaunchesCondition]) return;
	if (![self passesSampling]) return;
	
	// only show within the early window (double-check)
	NSTimeInterval elapsed = CFAbsoluteTimeGetCurrent() - self.processStartTime;
	if (elapsed > kBattmanDonationPushAboutSeconds) return;
	
	// verify UI is ready and not presenting another modal
	UIViewController *top = [self topViewController];
	if (!top) return;
	
	// don't present if top is already presenting something or not in window yet
	if (top.presentedViewController) return;
	if (!top.isViewLoaded || !top.view.window) return;
	
	// As safety, run on next runloop tick so we don't interfere with first-run UI transitions.
	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
		// double-check the one-time flag again (race-safe)
		if ([self hasShownBefore]) return;

		// Another defensive check: if top VC changed to a modal in the meantime don't show
		UIViewController *top2 = [self topViewController];
		if (!top2 || top2.presentedViewController) return;

		// Mark shown before calling (so reentrancy / crashes won't re-trigger)
		[[NSUserDefaults standardUserDefaults] setBool:YES forKey:(__bridge NSString *)kBattmanDonateShownKey];
		[[NSUserDefaults standardUserDefaults] synchronize];

		show_donation();
	});
}

@end

#pragma mark - C helper

bool donation_shown(void) {
	return [[DonationPrompter shared] hasShownBefore];
}

void donation_prompter_request_check(void) {
	dispatch_async(dispatch_get_main_queue(), ^{
		[[DonationPrompter shared] evaluateAndShowIfEligible];
	});
}

void show_donation(void) {
	extern id gWindow;
	dispatch_async(dispatch_get_main_queue(), ^{
		UIViewController *top = find_top_controller([gWindow rootViewController]);
		if (!top) return;

		// make sure we are not already presenting something heavy
		if (top.presentedViewController) return;

		DonationViewController *vc = [[DonationViewController alloc] init];
		vc.modalPresentationStyle = UIModalPresentationPageSheet;

		UINavigationController *nav = [[UINavigationController alloc] initWithRootViewController:vc];
		nav.modalPresentationStyle = UIModalPresentationPageSheet;
		[top presentViewController:nav animated:YES completion:nil];
	});
}
