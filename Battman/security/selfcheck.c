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

#include "cobjc.h"

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
	((void (*)(id, SEL, BOOL))objc_msgSend)(blk->data,oselector(setHidden:),1);
	objc_release(blk->data);
	CFURLRef link=CFURLCreateWithString(0,CFSTR("https://github.com/Torrekie/Battman"),NULL);
	CFDictionaryRef emptyDict=CFDictionaryCreate(0,NULL,NULL,0,NULL,NULL);
	struct my_block eblk;
	eblk.isa=&_NSConcreteStackBlock;
	eblk.flags=(1<<29);
	eblk.reserved=0;
	eblk.invoke=app_exit;
	eblk.data=NULL;
	eblk.descriptor=&my_block_descriptor_1arg;
	id eblk_c=((id(*)(struct my_block*,SEL))objc_msgSend)(&eblk,oselector(copy));
	((BOOL(*)(id,SEL,CFURLRef,CFDictionaryRef,id))objc_msgSend)(((id(*)(Class,SEL))objc_msgSend)(oclass(UIApplication),oselector(sharedApplication)),oselector(openURL:options:completionHandler:),link,emptyDict,eblk_c);
	CFRelease(link);
	CFRelease(emptyDict);
	objc_release(eblk_c);
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
	id view=(id)CFArrayGetValueAtIndex(*data,0);
	CFArrayRemoveValueAtIndex(*data,0);
	((void (*)(id, SEL))objc_msgSend)(view, oselector(removeFromSuperview));
	objc_release(view);
}

void showCompletionAlert_f(void) {
	id gAlertWindow=NULL;
	id sharedApp=((id (*)(Class, SEL))objc_msgSend)(oclass(UIApplication), oselector(sharedApplication));
	
	// iOS 13+: find foreground-active UIWindowScene
	if (__builtin_available(iOS 13.0, *)) {
		id scene;
		CFSetRef connectedScenes = ((CFSetRef (*)(id, SEL))objc_msgSend)(sharedApp, oselector(connectedScenes));
		int cntScene=CFSetGetCount(connectedScenes);
		id *allScenes=malloc(cntScene*sizeof(id));
		CFSetGetValues(connectedScenes,(const void**)allScenes);
		for(int i=0;i<cntScene;i++) {
			// Check activationState == UISceneActivationStateForegroundActive
			NSInteger state = ((NSInteger (*)(id, SEL))objc_msgSend)(allScenes[i], oselector(activationState));
			if (state != UISceneActivationStateForegroundActive)
				continue;
			
			BOOL isWindowScene = ((BOOL (*)(id, SEL, Class))objc_msgSend)(allScenes[i], oselector(isKindOfClass:), oclass(UIWindowScene));
			if (isWindowScene) {
				scene = allScenes[i];
				break;
			}
		}
		free(allScenes);
		
		if (scene)
			gAlertWindow=((id (*)(id, SEL, id))objc_msgSend)(objc_alloc(oclass(UIWindow)), oselector(initWithWindowScene:), scene);
	}
	
	// Fallback for <iOS13 or no scene
	if (!gAlertWindow)
		gAlertWindow = objc_alloc_init(oclass(UIWindow));
	
	SEL selSetLevel = sel_registerName("setWindowLevel:");
	((void (*)(id, SEL, CGFloat))objc_msgSend)(gAlertWindow, selSetLevel, UIWindowLevelAlert + 1);
	
	id vc = objc_alloc_init(oclass(UIViewController));
	
	id viewObj = ((id (*)(id, SEL))objc_msgSend)(vc, oselector(view));
	/*Class colorClass = objc_getClass("UIColor");
	SEL selClear = sel_registerName("clearColor");
	id clear = ((id (*)(Class, SEL))objc_msgSend)(colorClass, selClear);
	SEL selSetBG = sel_registerName("setBackgroundColor:");
	((void (*)(id, SEL, id))objc_msgSend)(viewObj, selSetBG, clear);
	commented bc I think uivc is clear by default
	*/
	
	((void (*)(id, SEL, id))objc_msgSend)(gAlertWindow, oselector(setRootViewController:), vc);
	((void (*)(id, SEL))objc_msgSend)(gAlertWindow, oselector(makeKeyAndVisible));
	
	Class alertClass = oclass(UIAlertController);
	SEL selAlert = oselector(alertControllerWithTitle:message:preferredStyle:);
	CFStringRef title = _("Sorry");
	CFStringRef msg   = _("Please download Battman from our official page.");
	id alert = ((id (*)(Class, SEL, CFStringRef, CFStringRef, NSInteger))objc_msgSend)(alertClass, selAlert, title, msg, (NSInteger)UIAlertControllerStyleAlert);
	
	Class actionClass = oclass(UIAlertAction);
	SEL selAction = oselector(actionWithTitle:style:handler:);
	
	
	struct my_block open_url_block;
	open_url_block.isa=&_NSConcreteStackBlock;
	open_url_block.flags=(1<<29);
	open_url_block.reserved=0;
	open_url_block.invoke=open_url_block__invoke;
	open_url_block.data=gAlertWindow;
	open_url_block.descriptor=&my_block_descriptor_1arg;
	id open_url_block_c=((id(*)(struct my_block*,SEL))objc_msgSend)(&open_url_block,oselector(copy));
	
	/*void (^handlerBlock)(id) = ^(id action) {
		SEL selHide = sel_registerName("setHidden:");
		((void (*)(id, SEL, BOOL))objc_msgSend)(gAlertWindow, selHide, YES);
		gAlertWindow = NULL; // also this line will not work in C
		open_url("https://github.com/Torrekie/Battman");
	};
	// Franken C ???
	*/
	
	id ok = ((id (*)(Class, SEL, CFStringRef, NSInteger, id))objc_msgSend)(actionClass, selAction, _("Open URL"), (NSInteger)UIAlertActionStyleDefault, open_url_block_c);
	open_url_block.invoke=app_exit;
	id open_url_block_d=((id(*)(struct my_block*,SEL))objc_msgSend)(&open_url_block,oselector(copy));
	id exit_btn=((id (*)(Class, SEL, CFStringRef, NSInteger, id))objc_msgSend)(actionClass, selAction, _("Exit"), 1, open_url_block_d);

	SEL selAdd = oselector(addAction:);
	((void (*)(id, SEL, id))objc_msgSend)(alert, selAdd, ok);
	((void (*)(id, SEL, id))objc_msgSend)(alert, selAdd, exit_btn);
	objc_release(open_url_block_c);
	objc_release(open_url_block_d);
	
	SEL selPresent = oselector(presentViewController:animated:completion:);
	((void (*)(id, SEL, id, BOOL, id))objc_msgSend)(vc, selPresent, alert, YES, NULL);
}

void showCompletionAlert() {
	dispatch_async_f(dispatch_get_main_queue(),NULL,(void(*)(void*))showCompletionAlert_f);
}

void removeAllViews(void) {
	static bool scheduled = false;
	if (scheduled) return;
	scheduled = true;
	
	CFMutableArrayRef viewsQueue=CFArrayCreateMutable(0,64,NULL);
	
	CFArrayRef windows=((CFArrayRef(*)(id,SEL))objc_msgSend)(((id(*)(Class,SEL))objc_msgSend)(oclass(UIApplication),oselector(sharedApplication)),oselector(windows));
	int arrCnt=CFArrayGetCount(windows);
	
	DBGLOG(CFSTR("COUNT: %u"), arrCnt);
	for (int i=0;i<arrCnt;i++) {
		CFArrayRef subviews = collectAllSubviewsBottomUp((id)CFArrayGetValueAtIndex(windows,i));
		
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

CFArrayRef collectAllSubviewsBottomUp(id view) {
	CFMutableArrayRef resultArray=CFArrayCreateMutable(0,32,NULL);
	CFArrayRef subviews=((CFArrayRef (*)(id, SEL))objc_msgSend)(view, oselector(subviews));
	int count=CFArrayGetCount(subviews);
	
	for(int i=0;i<count;i++) {
		id subview=(id)CFArrayGetValueAtIndex(subviews,i);
		objc_retain(subview);
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
