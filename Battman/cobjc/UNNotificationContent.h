#pragma once
#include "./cobjc.h"

typedef NSObject UNNotificationContent;
typedef NSObject UNMutableNotificationContent;

DefineObjcMethod(void, UNMutableNotificationContentSetTitle, setTitle:, CFStringRef);
DefineObjcMethod(void, UNMutableNotificationContentSetSubtitle, setSubtitle:, CFStringRef);
DefineObjcMethod(void, UNMutableNotificationContentSetBody, setBody:, CFStringRef);
DefineObjcMethod(void, UNMutableNotificationContentSetThreadIdentifier, setThreadIdentifier:, CFStringRef);
