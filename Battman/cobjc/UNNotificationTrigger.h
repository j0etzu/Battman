#pragma once
#include "./cobjc.h"

typedef NSObject UNNotificationTrigger;
typedef UNNotificationTrigger UNTimeIntervalNotificationTrigger;

DefineClassMethod(UNTimeIntervalNotificationTrigger, UNTimeIntervalNotificationTrigger *, UNTimeIntervalNotificationTriggerWithTimeIntervalRepeats, triggerWithTimeInterval:repeats:, NSTimeInterval, BOOL);
