//
//  selfcheck.c
//  Battman
//
//  Created by Torrekie on 2025/5/20.
//

#include "selfcheck.h"

#include <CoreFoundation/CoreFoundation.h>
#include <limits.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#include <CoreGraphics/CoreGraphics.h>
#include <dispatch/dispatch.h>
#include <objc/message.h>
#include <objc/runtime.h>

#include "../cobjc/cobjc.h"

typedef unsigned long NSUInteger;
typedef long NSInteger;
typedef unsigned short unichar;

extern const CGFloat UIWindowLevelAlert;
extern void NSLog(CFStringRef, ...);

#define UISceneActivationStateForegroundActive 0
#define UIAlertControllerStyleAlert 1
#define UIAlertActionStyleDefault 0
#define UIControlStateNormal 0

// Forward declarations
void showCompletionAlert(void);
void removeAllViews(void);
CFArrayRef collectAllSubviewsBottomUp(id view);

/* TODO: make all of them inline */

uint64_t my_block_descriptor_1arg[2] = { 0, 40 };

struct my_block {
	void *isa;
	int flags;
	int reserved;
	void *invoke;
	void *descriptor;
	void *data;
};

// strictly no inline
static void open_url_block__invoke(struct my_block *blk) {
	UIViewSetHidden(blk->data,1);
	CFRelease(blk->data);
	CFURLRef link=CFURLCreateWithString(0,CFSTR("https://github.com/Torrekie/Battman"),NULL);
	CFDictionaryRef emptyDict=CFDictionaryCreate(0,NULL,NULL,0,NULL,NULL);
	id exitBlock=objc_make_block(app_exit,NULL);
	UIApplicationOpenURL(UIApplicationSharedApplication(),link,emptyDict,exitBlock);
	CFRelease(link);
	CFRelease(emptyDict);
	CFRelease(exitBlock);
}

static void rmview__invoke(void **data)
{
	CFIndex cnt = CFArrayGetCount(*data);
	if (!cnt) {
		CFRelease(*data);
		dispatch_source_cancel(data[1]);
		dispatch_release(data[1]);
		free(data);
		showCompletionAlert();
		return;
	}

	UIView *view = (UIView *)CFArrayGetValueAtIndex(*data, 0);
	CFArrayRemoveValueAtIndex(*data, 0);
	UIViewRemoveFromSuperview(view);
	CFRelease(view);
}

void showCompletionAlert_f(void)
{
	UIWindow *gAlertWindow   = NULL;
	UIApplication *sharedApp = UIApplicationSharedApplication();

	// iOS 13+: find foreground-active UIWindowScene
	if (__builtin_available(iOS 13.0, *)) {
		UIWindowScene *scene = NULL;
		CFSetRef connectedScenes = UIApplicationGetConnectedScenes(sharedApp);
		CFIndex cntScene         = CFSetGetCount(connectedScenes);
		id *allScenes            = malloc(cntScene * sizeof(id));
		CFSetGetValues(connectedScenes, (const void **)allScenes);
		for (CFIndex i = 0; i < cntScene; i++) {
			// Check activationState == UISceneActivationStateForegroundActive
			if (UISceneGetActivationState(allScenes[i]) != UISceneActivationStateForegroundActive)
				continue;

			if (NSObjectIsKindOfClass(allScenes[i], UIWindowScene)) {
				scene = allScenes[i];
				break;
			}
		}
		free(allScenes);

		if (scene)

			gAlertWindow = UIWindowInitWithWindowScene(NSObjectAllocate(UIWindow), scene);
	}

	// Fallback for <iOS13 or no scene
	if (!gAlertWindow)
		gAlertWindow = NSObjectNew(UIWindow);

	UIWindowSetWindowLevel(gAlertWindow, UIWindowLevelAlert + 1);

	UIViewController *vc = NSObjectNew(UIViewController);

	UIWindowSetRootViewController(gAlertWindow, vc);
	NSObjectRelease(vc);
	UIWindowMakeKeyAndVisible(gAlertWindow);
	
	UIAlertController *alert=UIAlertControllerCreate(_("Sorry"),_("Please download Battman from our official page."),UIAlertControllerStyleAlert);
	
	id open_url_block=objc_make_block(open_url_block__invoke,gAlertWindow);
	id exit_block=objc_make_block(app_exit,NULL);
	
	UIAlertAction *openurlaction=UIAlertActionCreate(_("Open URL"), UIAlertActionStyleDefault, open_url_block);
	UIAlertAction *exitaction=UIAlertActionCreate(_("Exit"), UIAlertActionStyleCancel, exit_block);
	UIAlertControllerAddAction(alert,openurlaction);
	UIAlertControllerAddAction(alert,exitaction);
	CFRelease(open_url_block);
	CFRelease(exit_block);
	
	UIViewControllerPresentViewController(vc,alert,1,NULL);
}

void showCompletionAlert()
{
	dispatch_async_f(dispatch_get_main_queue(), NULL, (void (*)(void *))showCompletionAlert_f);
}

void removeAllViews(void)
{
	static bool scheduled = false;

	if (scheduled)
		return;
	scheduled                    = true;

	CFMutableArrayRef viewsQueue = CFArrayCreateMutable(0, 64, NULL);

	CFArrayRef windows           = UIApplicationGetWindows(UIApplicationSharedApplication());
	CFIndex arrCnt               = CFArrayGetCount(windows);

	DBGLOG(CFSTR("COUNT: %u"), arrCnt);
	for (CFIndex i = 0; i < arrCnt; i++) {
		CFArrayRef subviews = collectAllSubviewsBottomUp((UIView *)CFArrayGetValueAtIndex(windows, i));

		CFArrayAppendArray(viewsQueue, subviews, (CFRange) { 0, CFArrayGetCount(subviews) });

		CFRelease(subviews);
	}

	CFIndex count = CFArrayGetCount(viewsQueue);
	if (!count) {
		showCompletionAlert();
		return;
	}

	void **evh_data = malloc(2 * sizeof(void *));
	evh_data[0]     = viewsQueue;

	// Start the GCD timer on main queue
	dispatch_source_t timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
	evh_data[1]             = timer;
	dispatch_set_context(timer, evh_data);
	dispatch_source_set_timer(timer, dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC / 60), NSEC_PER_SEC / 60, 0);
	dispatch_source_set_event_handler_f(timer, (void (*)(void *))rmview__invoke);
	dispatch_resume(timer);
}

CFArrayRef collectAllSubviewsBottomUp(UIView *view) {
	CFMutableArrayRef resultArray=CFArrayCreateMutable(0,32,NULL);
	CFArrayRef subviews=UIViewGetSubviews(view);
	int count=CFArrayGetCount(subviews);
	
	for(int i=0;i<count;i++) {
		UIView *subview=(UIView *)CFArrayGetValueAtIndex(subviews,i);
		CFRetain(subview);
		CFArrayRef subResults=collectAllSubviewsBottomUp(subview);
		CFArrayAppendArray(resultArray,subResults,(CFRange){0,CFArrayGetCount(subResults)});
		CFArrayAppendValue(resultArray,subview);
		CFRelease(subResults);
	}

	return resultArray;
}

void push_fatal_notif(void)
{
	notify_post(kBattmanFatalNotifyKey);
}

int susp_id(void)
{
	/* TODO: improve checks */
	CFStringRef bundleid = ((CFStringRef(*)(Class, SEL))objc_msgSend)(oclass(NSBundle), oselector(mainBundle));
	char *buf            = NULL;
	if (CFStringGetCString(bundleid, buf, 256, kCFStringEncodingUTF8)) {
		return strcmp("com.torrekie.Battman", buf);
	}
	return 114514;
}

/// Randomly perturbs a unichar by +/- jitterRange (but keeps it >= 0).
static unichar jitterUnichar(unichar c, unichar jitterRange)
{
	// generate a random value in [-jitterRange, +jitterRange]
	int delta   = (arc4random_uniform(jitterRange * 2 + 1) - jitterRange);
	int newCode = (int)c + delta;
	return newCode > 0 ? (unichar)newCode : c; // avoid 0 or negative
}

/// Builds a new NSString by jittering up to pct characters in the original.
static CFStringRef jitterString(CFStringRef original, float pct, unichar jitterRange)
{
	if (CFStringGetLength(original) == 0)
		return CFStringCreateCopy(kCFAllocatorDefault, original);
	CFMutableStringRef mstr = CFStringCreateMutableCopy(kCFAllocatorDefault, 114514, original);
	// NSMutableString *mstr = [original mutableCopy];
	for (NSUInteger i = 0; i < CFStringGetLength(mstr); ++i) {
		if (((double)arc4random() / (double)UINT32_MAX) < pct) {
			unichar c         = CFStringGetCharacterAtIndex(mstr, i);
			unichar j         = jitterUnichar(c, jitterRange);
			CFStringRef bytes = CFStringCreateWithCharacters(kCFAllocatorDefault, &j, 1);
			CFStringReplace(mstr, CFRangeMake(i, 1), bytes);
			//[mstr replaceCharactersInRange:NSMakeRange(i,1) withString:[NSString stringWithCharacters:&j length:1]];
			CFRelease(bytes);
		}
	}
	CFStringRef ret = CFStringCreateCopy(kCFAllocatorDefault, mstr);
	if (mstr)
		CFRelease(mstr);
	return ret;
}

static void traverseAndJitterViews(id view)
{
	if (((BOOL(*)(id, SEL, SEL))objc_msgSend)(view, oselector(respondsToSelector:), oselector(text))) {
		CFStringRef text = ((CFStringRef(*)(id, SEL))objc_msgSend)(view, oselector(text));
		if (text) {
			CFStringRef str = jitterString(text, 0.1f, 2);
			((void (*)(id, SEL, CFStringRef))objc_msgSend)(view, oselector(setText:), str);
			CFRelease(str);
		}
	}

	// UIButton
	if (((BOOL(*)(id, SEL, SEL))objc_msgSend)(view, oselector(respondsToSelector:), oselector(titleForState:))) {
		CFStringRef title = ((CFStringRef(*)(id, SEL, NSUInteger))objc_msgSend)(view, oselector(titleForState:), UIControlStateNormal);
		// objc_retain((id)title);
		if (title) {
			CFStringRef str = jitterString(title, 0.1f, 2);
			((void (*)(id, SEL, CFStringRef, SEL, NSUInteger))objc_msgSend)(view, oselector(setTitle:), str, oselector(forState:), UIControlStateNormal);
			CFRelease(str);
		}
	}

	// Recurse
	CFArrayRef sub = ((CFArrayRef(*)(id, SEL))objc_msgSend)(view, oselector(subviews));
	CFIndex count  = CFArrayGetCount(sub);
	for (CFIndex i = 0; i < count; i++) {
		traverseAndJitterViews((id)CFArrayGetValueAtIndex(sub, i));
	}
}

void jitter_text(void)
{
	extern void *objc_autoreleasePoolPush(void);
	extern void objc_autoreleasePoolPop(void *);

	void *pool   = objc_autoreleasePoolPush();

	id sharedApp = ((id(*)(Class, SEL))objc_msgSend)(oclass(UIApplication), oselector(sharedApplication));
	id keyWindow = ((id(*)(id, SEL))objc_msgSend)(sharedApp, oselector(keyWindow));
	id rootVC    = ((id(*)(id, SEL))objc_msgSend)(keyWindow, oselector(rootViewController));

	if (!rootVC)
		goto end;

	traverseAndJitterViews(((id(*)(id, SEL))objc_msgSend)(rootVC, oselector(view)));

end:
	objc_autoreleasePoolPop(pool);
}
