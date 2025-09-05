#pragma once
#include "./cobjc.h"
#include "./UNNotificationRequest.h"

typedef NSObject UNUserNotificationCenter;

DefineClassMethod(UNUserNotificationCenter, UNUserNotificationCenter *, UNUserNotificationCenterCurrentNotificationCenter, currentNotificationCenter);

DefineObjcMethod(UNUserNotificationCenter *, UNUserNotificationCenterInitWithBundleIdentifier, initWithBundleIdentifier:, CFStringRef);

DefineObjcMethod(void, UNUserNotificationCenterRequestAuthorizationWithOptions, requestAuthorizationWithOptions:completionHandler:, int, id);
DefineObjcMethod(void, UNUserNotificationAddNotificationRequest, addNotificationRequest:withCompletionHandler:, UNNotificationRequest *, id);
