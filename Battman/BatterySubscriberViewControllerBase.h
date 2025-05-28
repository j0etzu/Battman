#pragma once
#import <UIKit/UIKit.h>

@interface BatterySubscriberViewControllerBase : UITableViewController
- (void)batteryStatusDidUpdate;
- (void)batteryStatusDidUpdate:(NSDictionary *)info;
@end
