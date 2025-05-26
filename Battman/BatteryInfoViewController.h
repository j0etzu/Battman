#import <UIKit/UIKit.h>
#import "BatterySubscriberViewControllerBase.h"
#include "battery_utils/battery_info.h"

@interface BatteryInfoViewController : BatterySubscriberViewControllerBase
{
	struct battery_info_section *batteryInfo;
}
@end

UIImage *imageForSFProGlyph(NSString *glyph, NSString *fontName, CGFloat fontSize, UIColor *tintColor);
