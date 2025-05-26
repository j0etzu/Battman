#include "../battery_utils/battery_info.h"
#import "BatteryCellView.h"
#import <UIKit/UIKit.h>

// TODO: Implement underlying functions creating such infos
// Display: "$description: $content"
// or: "$description" in case *content==0

@interface BatteryInfoTableViewCell : UITableViewCell
@property(nonatomic, assign, readwrite) struct battery_info_section **batteryInfo;
@property(nonatomic, strong, readonly) BatteryCellView *batteryCell;
@property(nonatomic, strong, readonly) UILabel *batteryLabel;

- (void)updateBatteryInfo;

@end
