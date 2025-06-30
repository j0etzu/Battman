#import "SimpleTemperatureViewController.h"
#import "common.h"

// battery_utils/hid.m
extern NSDictionary *getTemperatureHIDData(void);
extern NSDictionary *getSensorTemperatures(void);

@implementation SimpleTemperatureViewController

- (instancetype)init {
	if (@available(iOS 13.0, *)) {
		self = [super initWithStyle:UITableViewStyleInsetGrouped];
	} else {
		self = [super initWithStyle:UITableViewStyleGrouped];
	}
	self.tableView.allowsSelection = 0;
	temperatureHIDData = getTemperatureHIDData();
	sensorTemperatures = getSensorTemperatures();
	return self;
}

- (NSString *)title {
	return _("Hardware Temperature");
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tv {
	return 2;
}

- (NSInteger)tableView:(id)tv numberOfRowsInSection:(NSInteger)section {
	if (section == 0) {
		return sensorTemperatures.count;
	}
	return temperatureHIDData.count;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
	if (section == 0) {
		return _("Device Sensors");
	}
	return _("HID");
}

- (UITableViewCell *)tableView:(UITableView *)tv cellForRowAtIndexPath:(NSIndexPath *)ip {
	UITableViewCell *cell = [tv dequeueReusableCellWithIdentifier:@"stvc:main"];
	if (!cell)
		cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"stvc:main"];
	NSDictionary *dict = NULL;
	NSString *label = NULL;
	if (ip.section == 0) {
		dict = sensorTemperatures;
		// ????? this is terrible, why not store cstring at beginning?
		label = _([dict.allKeys[ip.row] UTF8String]);
	} else if (ip.section == 1) {
		/* TODO: Explicit naming on known sensors */
		/* iPhone Skin Temperature: TSRM, TSBE or TSBH */
		/* iPad Skin Temperature: TSBM */

		/* TODO: Filter stub temperatures */
		/* Some sensors are not actually getting its temperature
		   they always looks like 0.00 or 30.00 */

		/* TODO: Better UI for sensors having Avg & Cur & Max */
		/* Not every HID temp sensors recording realtime values */
		dict = temperatureHIDData;
		label = dict.allKeys[ip.row];
	}
	cell.textLabel.text = label;
	cell.detailTextLabel.text = [NSString stringWithFormat:@"%.4g ℃", [dict[dict.allKeys[ip.row]] floatValue]];

	return cell;
}

@end
