//
//  ThermalTunesViewContoller.m
//  Battman
//
//  Created by Torrekie on 2025/8/2.
//

#import "ThermalTunesViewContoller.h"

#import "common.h"
#import "intlextern.h"
#import "UberSegmentedControl.h"

@interface ThermalSegmentedControl : UIView
@property (nonatomic, assign) BOOL toggled;
@property (nonatomic, assign) BOOL persist;
@property (nonatomic) BOOL isLockerSwitch;
@property (nonatomic, strong) UberSegmentedControl *control;
@end

@implementation ThermalSegmentedControl
@dynamic toggled;
@dynamic persist;

- (instancetype)initWithLockerSwitch {

	NSArray *items;
	if (@available(iOS 13.0, *)) {
		items = @[[UIImage systemImageNamed:@"lock.fill"], [UIImage systemImageNamed:@"checkmark"]];
	} else {
		// lock.fill U+1003A1
		// checkmark U+100185
		items = @[@"􀎡", @"􀆅"];
	}
	self = [super initWithFrame:CGRectMake(0, 0, 74, 32)];
	if (self) {
		self.isLockerSwitch = YES;

		UberSegmentedControlConfig *conf = [[UberSegmentedControlConfig alloc] initWithFont:[UIFont systemFontOfSize:UIFont.systemFontSize weight:UIFontWeightRegular] tintColor:nil allowsMultipleSelection:YES];

		_control = [[UberSegmentedControl alloc] initWithItems:items config:conf];
		[_control setFrame:CGRectMake(0, 0, 74, 32)];
		[self addSubview:_control];
		
		_control.translatesAutoresizingMaskIntoConstraints = NO;
		[_control.leadingAnchor constraintEqualToAnchor:self.leadingAnchor].active = YES;
		[_control.trailingAnchor constraintEqualToAnchor:self.trailingAnchor].active = YES;
	}
	return self;
}

- (BOOL)toggled {
	BOOL ret = NO;
	if (self.isLockerSwitch) {
		ret = [[_control selectedSegmentIndexes] containsIndex:1];
	}
	return ret;
}

- (BOOL)persist {
	BOOL ret = NO;
	if (self.isLockerSwitch) {
		ret = [[_control selectedSegmentIndexes] containsIndex:0];
	}
	return ret;
}

- (void)setToggled:(BOOL)toggled {
	if (self.isLockerSwitch) {
		NSMutableIndexSet *indexes = [[NSMutableIndexSet alloc] init];
		[indexes addIndex:1];
		[_control setSelectedSegmentIndexes:indexes];
	}
}
- (void)setPersist:(BOOL)persist {
	if (self.isLockerSwitch) {
		NSMutableIndexSet *indexes = [[NSMutableIndexSet alloc] init];
		[indexes addIndex:0];
		[_control setSelectedSegmentIndexes:indexes];
	}
}
- (void)updateLockerSwitchByFunction:(void (*)(bool *, bool *))function {
	bool enabled = false;
	bool persist = false;
	function(&enabled, &persist);
	NSLog(@"ENABLED %d PERSIST %d", enabled, persist);
	[self setToggled:enabled];
	[self setPersist:persist];
}

@end

typedef enum {
	TT_SECT_GENERAL,
	TT_SECT_HIP,

	TT_SECT_COUNT
} TTSects;

// TT_SECT_GENERAL
typedef enum {
	TT_ROW_GENERAL_ENABLED,
} TTSectGeneral;

// TT_SECT_HIP
typedef enum {
	TT_ROW_HIP_ENABLED,
	TT_ROW_HIP_SIMULATE,
} TTSectHIP;

@implementation ThermalTunesViewContoller

- (NSString *)title {
	return _("Thermal Tunes");
}

- (instancetype)init {
	if (@available(iOS 13.0, *)) {
		self = [super initWithStyle:UITableViewStyleInsetGrouped];
	} else {
		self = [super initWithStyle:UITableViewStyleGrouped];
	}
	return self;
}

- (void)viewDidLoad {
	[super viewDidLoad];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return TT_SECT_COUNT;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
	TTSects sect = (TTSects)section;
	switch (sect) {
		case TT_SECT_GENERAL: return _("General");
		case TT_SECT_HIP: return _("Hot-In-Pocket Mode"); // Sadly, HIP heuristics has no official translations
		case TT_SECT_COUNT: break;
	}
	return nil;
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
	TTSects sect = (TTSects)section;
	switch (sect) {
		case TT_SECT_GENERAL: return _("Changing the default thermal behavior may increase wear on your battery and reduce its lifespan.");
		case TT_SECT_HIP: return _("Hot-In-Pocket Protection automatically reduces CPU & GPU power when the display is off and no media is playing, to prevent overheating while the device is stored in a pocket.");
		case TT_SECT_COUNT: break;
	}
	return nil;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	TTSects sect = (TTSects)section;
	switch (sect) {
		case TT_SECT_GENERAL: return 1;
		case TT_SECT_HIP: return 2;
		case TT_SECT_COUNT: break;
	}
	return 0;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
	cell.selectionStyle = UITableViewCellSelectionStyleNone;

	if (indexPath.section == TT_SECT_GENERAL) {
		TTSectGeneral row = (TTSectGeneral)indexPath.row;
		switch (row) {
			case TT_ROW_GENERAL_ENABLED: {
				cell.textLabel.text = _("Enable");
				cell.detailTextLabel.text = _("Whether applications receive thermal state updates");
				ThermalSegmentedControl *control = [[ThermalSegmentedControl alloc] initWithLockerSwitch];
				extern void getOSNotifEnabled(bool *enable, bool *persist);
				[control updateLockerSwitchByFunction:getOSNotifEnabled];
				cell.accessoryView = control;
				[cell.accessoryView sizeToFit];
				// Consider create a protocol
				[control.control addTarget:self action:@selector(controllerChanged:) forControlEvents:UIControlEventValueChanged];
			}
		}
	}
	if (indexPath.section == TT_SECT_HIP) {
		TTSectHIP row = (TTSectHIP)indexPath.row;
		switch (row) {
			case TT_ROW_HIP_ENABLED: {
				cell.textLabel.text = _("Enable");
				ThermalSegmentedControl *control = [[ThermalSegmentedControl alloc] initWithLockerSwitch];
				cell.accessoryView = control;
				[cell.accessoryView sizeToFit];
				extern void getHIPEnabled(bool *enable, bool *persist);
				[control updateLockerSwitchByFunction:getHIPEnabled];
				[control.control addTarget:self action:@selector(controllerChanged:) forControlEvents:UIControlEventValueChanged];
				break;
			}
			case TT_ROW_HIP_SIMULATE: {
				cell.textLabel.text = _("Simulate");
				UISwitch *button = [UISwitch new];
				extern void getSimulateHIPEnabled(bool *enable, bool *persist);
				bool toggled = false;
				getSimulateHIPEnabled(&toggled, NULL);
				button.on = toggled;
				cell.accessoryView = button;
				[button addTarget:self action:@selector(controllerChanged:) forControlEvents:UIControlEventValueChanged];
				break;
			}
		}
	}
	return cell;
}

- (void)controllerChanged:(UIView *)controller {
	UIView *view = controller;
	while (view && ![view isKindOfClass:[UITableViewCell class]]) {
		view = [view superview];
	}
	if (view) {
		UITableViewCell *cell = (UITableViewCell *)view;
		UIView *tb = view;
		while (tb && ![tb isKindOfClass:[UITableView class]]) {
			tb = [tb superview];
		}
		if (tb) {
			UITableView *tv = (UITableView *)tb;
			return [self writeThermalBoolByIndexPath:[tv indexPathForCell:cell] control:(UIControl *)cell.accessoryView];
		}
	}
	
	DBGLOG(@"FIXME: controllerChanged without cell view!");
}

// 0    0   00
// SECT ROW VALUE
//          0x1: On/Off
//          0x2: Persist
#define WORKER_THERMAL_BOOL_CMD (uint32_t)((indexPath.section << 12) | (indexPath.row << 8) | (ctrl.persist << 1) | (ctrl.toggled))

- (void)writeThermalBoolCmd:(uint32_t)cmd {
	extern uint64_t battman_worker_call(char cmd, void *arg, uint64_t arglen);
	uint64_t ret = battman_worker_call(5, (void *)&cmd, 4);
	if (ret != 0) {
		char *errstr = calloc(1024, 1);
		sprintf(errstr, "%s: %llu", _C("Thermal Tuning failed with error"), ret);
		show_alert(L_FAILED, errstr, L_OK);
		free(errstr);
	}
}

- (void)writeThermalBoolByIndexPath:(NSIndexPath *)indexPath control:(UIControl *)control {
	if ([control isKindOfClass:[ThermalSegmentedControl class]]) {
		ThermalSegmentedControl *ctrl = (ThermalSegmentedControl *)control;
		if (ctrl.isLockerSwitch)
			[self writeThermalBoolCmd:WORKER_THERMAL_BOOL_CMD];
	}
	if ([control isKindOfClass:[UISwitch class]]) {
		UISwitch *ctrl = (UISwitch *)control;
		[self writeThermalBoolCmd:(uint32_t)((indexPath.section << 12) | (indexPath.row << 8) | (ctrl.on))];
	}
}

@end
