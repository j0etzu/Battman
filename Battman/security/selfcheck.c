//
//  selfcheck.c
//  Battman
//
//  Created by Torrekie on 2025/5/20.
//

#include "selfcheck.h"

#include <mach-o/dyld.h>
#include <CoreFoundation/CoreFoundation.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <objc/runtime.h>
#include <objc/message.h>
#include <CoreGraphics/CoreGraphics.h>
#include <dispatch/dispatch.h>

#include "../cobjc/cobjc.h"

typedef unsigned long NSUInteger;
typedef long NSInteger;

extern const CGFloat UIWindowLevelAlert;
extern void NSLog(CFStringRef, ...);

#define UISceneActivationStateForegroundActive 0
#define UIAlertControllerStyleAlert 1
#define UIAlertActionStyleDefault 0

// Forward declarations
void showCompletionAlert(void);
void removeAllViews(void);
CFArrayRef collectAllSubviewsBottomUp(id view);

/* TODO: make all of them inline */

uint64_t my_block_descriptor_1arg[2]={0,40};

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
	NSObjectRelease(blk->data);
	CFURLRef link=CFURLCreateWithString(0,CFSTR("https://github.com/Torrekie/Battman"),NULL);
	CFDictionaryRef emptyDict=CFDictionaryCreate(0,NULL,NULL,0,NULL,NULL);
	id exitBlock=objc_make_block(app_exit,NULL);
	UIApplicationOpenURL(UIApplicationSharedApplication(),link,emptyDict,exitBlock);
	CFRelease(link);
	CFRelease(emptyDict);
	NSObjectRelease(exitBlock);
}

static void rmview__invoke(void **data) {
	int cnt=CFArrayGetCount(*data);
	if(!cnt) {
		CFRelease(*data);
		dispatch_source_cancel(data[1]);
		dispatch_release(data[1]);
		free(data);
		showCompletionAlert();
		return;
	}
	UIView *view=(UIView*)CFArrayGetValueAtIndex(*data,0);
	CFArrayRemoveValueAtIndex(*data,0);
	UIViewRemoveFromSuperview(view);
	NSObjectRelease(view);
}

void showCompletionAlert_f(void) {
	UIWindow *gAlertWindow=NULL;
	UIApplication *sharedApp=UIApplicationSharedApplication();
	
	// iOS 13+: find foreground-active UIWindowScene
	if (__builtin_available(iOS 13.0, *)) {
		UIWindowScene *scene;
		CFSetRef connectedScenes=UIApplicationGetConnectedScenes(sharedApp);
		int cntScene=CFSetGetCount(connectedScenes);
		id *allScenes=malloc(cntScene*sizeof(id));
		CFSetGetValues(connectedScenes,(const void**)allScenes);
		for(int i=0;i<cntScene;i++) {
			// Check activationState == UISceneActivationStateForegroundActive
			if(UISceneGetActivationState(allScenes[i])!=UISceneActivationStateForegroundActive)
				continue;
			
			if (NSObjectIsKindOfClass(allScenes[i],UIWindowScene)) {
				scene = allScenes[i];
				break;
			}
		}
		free(allScenes);
		
		if (scene)
			gAlertWindow=UIWindowInitWithWindowScene(NSObjectAllocate(UIWindow), scene);
	}
	
	// Fallback for <iOS13 or no scene
	if (!gAlertWindow)
		gAlertWindow = NSObjectNew(UIWindow);
	
	UIWindowSetWindowLevel(gAlertWindow,UIWindowLevelAlert+1);
	
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
	NSObjectRelease(open_url_block);
	NSObjectRelease(exit_block);
	
	UIViewControllerPresentViewController(vc,alert,1,NULL);
}

void showCompletionAlert() {
	dispatch_async_f(dispatch_get_main_queue(),NULL,(void(*)(void*))showCompletionAlert_f);
}

void removeAllViews(void) {
	static bool scheduled = false;
	if (scheduled) return;
	scheduled = true;
	
	CFMutableArrayRef viewsQueue=CFArrayCreateMutable(0,64,NULL);
	
	CFArrayRef windows=UIApplicationGetWindows(UIApplicationSharedApplication());
	int arrCnt=CFArrayGetCount(windows);
	
	DBGLOG(CFSTR("COUNT: %u"), arrCnt);
	for (int i=0;i<arrCnt;i++) {
		CFArrayRef subviews = collectAllSubviewsBottomUp((UIView *)CFArrayGetValueAtIndex(windows,i));
		
		CFArrayAppendArray(viewsQueue,subviews,(CFRange){0,CFArrayGetCount(subviews)});
		CFRelease(subviews);
	}

	int count=CFArrayGetCount(viewsQueue);
	if(!count) {
		showCompletionAlert();
		return;
	}
	
	void **evh_data=malloc(2*sizeof(void*));
	evh_data[0]=viewsQueue;
	
	// Start the GCD timer on main queue
	dispatch_source_t timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
	evh_data[1]=timer;
	dispatch_set_context(timer,evh_data);
	dispatch_source_set_timer(timer, dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC / 60), NSEC_PER_SEC / 60, 0);
	dispatch_source_set_event_handler_f(timer, (void(*)(void*))rmview__invoke);
	dispatch_resume(timer);
}

CFArrayRef collectAllSubviewsBottomUp(UIView *view) {
	CFMutableArrayRef resultArray=CFArrayCreateMutable(0,32,NULL);
	CFArrayRef subviews=UIViewGetSubviews(view);
	int count=CFArrayGetCount(subviews);
	
	for(int i=0;i<count;i++) {
		UIView *subview=(UIView *)CFArrayGetValueAtIndex(subviews,i);
		NSObjectRetain(subview);
		CFArrayRef subResults=collectAllSubviewsBottomUp(subview);
		CFArrayAppendArray(resultArray,subResults,(CFRange){0,CFArrayGetCount(subResults)});
		CFArrayAppendValue(resultArray,subview);
		CFRelease(subResults);
	}
	
	return resultArray;
}

void push_fatal_notif(void) {
	notify_post(kBattmanFatalNotifyKey);
}
