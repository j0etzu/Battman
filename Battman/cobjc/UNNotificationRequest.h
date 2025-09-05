#pragma once
#include "./cobjc.h"
#include "./UNNotificationContent.h"
#include "./UNNotificationTrigger.h"

typedef NSObject UNNotificationRequest;

DefineClassMethod(UNNotificationRequest, UNNotificationRequest *, UNNotificationRequestWithIdentifierContentTrigger, requestWithIdentifier:content:trigger:, CFStringRef, UNNotificationContent *, UNNotificationTrigger *);
