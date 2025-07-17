#import "BatteryInfoViewController.h"
#import "BatteryCellView/BatteryInfoTableViewCell.h"
#import "BatteryCellView/TemperatureInfoTableViewCell.h"
#import "BatteryDetailsViewController.h"
#import "ChargingManagementViewController.h"
#import "ChargingLimitViewController.h"
#include "battery_utils/battery_utils.h"
#import "SimpleTemperatureViewController.h"
#import "UPSMonitor.h"

#include "common.h"

// TODO: UI Refreshing

enum sections_batteryinfo {
	BI_SECT_BATTERY_INFO,
	BI_SECT_HW_TEMP,
	BI_SECT_MANAGE,
	BI_SECT_COUNT
};

@implementation BatteryInfoViewController

- (NSString *)title {
    return _("Battman");
}

- (void)batteryStatusDidUpdate:(NSDictionary *)info {
	battery_info_update(&batteryInfo);
	//battery_info_update_iokit_with_data(batteryInfo,(__bridge CFDictionaryRef)info,0);
	[super batteryStatusDidUpdate];
}

- (void)viewDidLoad {
    [super viewDidLoad];

    // Copyright text
    UILabel *copyright;
    copyright = [[UILabel alloc] init];
    NSString *me = _("2025 Ⓒ Torrekie <me@torrekie.dev>");
#ifdef DEBUG
    /* FIXME: GIT_COMMIT_HASH should be a macro */
    copyright.text = [NSString stringWithFormat:@"%@\n%@ %@\n%s %s", me, _("Debug Commit"), [[NSBundle mainBundle] objectForInfoDictionaryKey:@"GIT_COMMIT_HASH"], __DATE__, __TIME__];
    copyright.numberOfLines = 0;
#else
    copyright.text = me;
#endif

    /* FIXME: Containered is not Sandboxed, try some extra checks */
    char *home = getenv("HOME");
    if (match_regex(home, IOS_CONTAINER_FMT) || match_regex(home, MAC_CONTAINER_FMT)) {
        copyright.text = [copyright.text stringByAppendingFormat:@"\n%@", _("Sandboxed")];
    } else if (match_regex(home, SIM_CONTAINER_FMT)) {
        copyright.text = [copyright.text stringByAppendingFormat:@"\n%@", _("Simulator Sandboxed")];
    } else if (match_regex(home, SIM_UNSANDBOX_FMT)){
        copyright.text = [copyright.text stringByAppendingFormat:@"\n%@", _("Simulator Unsandboxed")];
    } else {
        DBGLOG(@"HOME: %s", home);
        copyright.text = [copyright.text stringByAppendingFormat:@"\n%@", _("Unsandboxed")];
    }

	if (is_platformized())
		copyright.text = [copyright.text stringByAppendingFormat:@", %@", _("Platfomized")];

	if (is_debugged())
		copyright.text = [copyright.text stringByAppendingFormat:@", %@", _("Debugger Attached")];

	copyright.font = [UIFont systemFontOfSize:12];
    copyright.textAlignment = NSTextAlignmentCenter;
    copyright.textColor = [UIColor grayColor];
    [copyright sizeToFit];
    self.tableView.tableFooterView = copyright;
}

- (instancetype)init {
    UITabBarItem *tabbarItem = [UITabBarItem new];
    tabbarItem.title = _("Battery");
    if (@available(iOS 13.0, *)) {
        tabbarItem.image = [UIImage systemImageNamed:@"battery.100"];
    } else {
        // U+1006E8
        tabbarItem.image = imageForSFProGlyph(@"􀛨", @SFPRO, 22, [UIColor grayColor]);
    }
    tabbarItem.tag = 0;
    self.tabBarItem = tabbarItem;
    battery_info_init(&batteryInfo);
	[UPSMonitor startWatchingUPS];

    return [super initWithStyle:UITableViewStyleGrouped];
}

- (NSInteger)tableView:(id)tv numberOfRowsInSection:(NSInteger)section {
	if (section == BI_SECT_MANAGE)
		return 2;
    return 1;
}

- (NSInteger)numberOfSectionsInTableView:(id)tv {
    return BI_SECT_COUNT;
}

- (NSString *)tableView:(id)t titleForHeaderInSection:(NSInteger)sect {
	switch(sect) {
        case BI_SECT_BATTERY_INFO:
            return _("Battery Info");
        case BI_SECT_HW_TEMP:
            return _("Hardware Temperature");
        case BI_SECT_MANAGE:
            return _("Manage");
        default:
            return nil;
	};
}

- (NSString *)tableView:(id)tv titleForFooterInSection:(NSInteger)section {
    return nil;
}

- (void)tableView:(UITableView *)tv didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    if (indexPath.section == BI_SECT_BATTERY_INFO)
        [self.navigationController pushViewController:[[BatteryDetailsViewController alloc] initWithBatteryInfo:&batteryInfo] animated:YES];
	else if (indexPath.section == BI_SECT_MANAGE)
		[self.navigationController pushViewController:indexPath.row == 0 ? [ChargingManagementViewController new] : [ChargingLimitViewController new] animated:YES];
    else if(indexPath.section==BI_SECT_HW_TEMP)
    	[self.navigationController pushViewController:[SimpleTemperatureViewController new] animated:1];
    [tv deselectRowAtIndexPath:indexPath animated:YES];
}

- (UITableViewCell *)tableView:(UITableView *)tv
         cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    if (indexPath.section == BI_SECT_BATTERY_INFO) {
        BatteryInfoTableViewCell *cell=[tv dequeueReusableCellWithIdentifier:@"BTTVC-cell"];
        if(!cell)
        	cell=[BatteryInfoTableViewCell new];
        cell.batteryInfo = &batteryInfo;
        // battery_info_update shall be called within cell impl.
        [cell updateBatteryInfo];
        return cell;
    } else if (indexPath.section == BI_SECT_HW_TEMP) {
        TemperatureInfoTableViewCell *cell=[tv dequeueReusableCellWithIdentifier:@"TITVC-ri"];
        if(!cell)
        	cell=[TemperatureInfoTableViewCell new];
        return cell;
    } else if (indexPath.section == BI_SECT_MANAGE) {
        UITableViewCell *cell = [UITableViewCell new];
        cell.textLabel.text = indexPath.row == 0 ? _("Charging Management") : _("Charging Limit");
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
        return cell;
    }

    return nil;
}

- (CGFloat)tableView:(id)tv heightForRowAtIndexPath:(NSIndexPath *)indexPath {
    if (indexPath.section == BI_SECT_BATTERY_INFO && indexPath.row == 0) {
        return 130;
    } else if (indexPath.section == BI_SECT_HW_TEMP && indexPath.row == 0) {
        return 130;
    } else {
        return [super tableView:tv heightForRowAtIndexPath:indexPath];
        // return 30;
    }
}

- (void)dealloc {
	for(struct battery_info_section *sect=batteryInfo;sect;) {
		struct battery_info_section *next=sect->next;
		for (struct battery_info_node *i = sect->data; i->name; i++) {
			if (i->content && !(i->content & BIN_IS_SPECIAL)) {
				bi_node_free_string(i);
			}
		}
		bi_destroy_section(sect);
		sect=next;
	}
}

@end
