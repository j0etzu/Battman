#pragma once
#import <UIKit/UIKit.h>
#import "BatterySubscriberViewControllerBase.h"
#include "battery_utils/battery_info.h"

@interface BatteryDetailsViewController : BatterySubscriberViewControllerBase {
	// There should be strictly ONE (1) head pointer stored
	// (which is left in BatteryInfoViewController)
	struct battery_info_section **batteryInfo;
	unsigned char *pendingLoadOffsets[BI_MAX_SECTION_NUM];

	int last_charging;
}
- (instancetype)initWithBatteryInfo:(struct battery_info_section **)bi;
@end
