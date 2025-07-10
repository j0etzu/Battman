#import "SimpleTemperatureViewController.h"
#import "common.h"

// battery_utils/hid.m
extern NSDictionary *getTemperatureHIDData(void);
extern NSDictionary *getSensorTemperatures(void);

static NSMutableDictionary *knownHIDSensors;

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
	if (knownHIDSensors == NULL) {
		extern float getTemperatureHIDAt(NSString *);
		knownHIDSensors = [[NSMutableDictionary alloc] init];
#if 0
		// Gettext
		NSArray __unused *knownKeys = @[
			_("Device Avg."),
			// _("iPad Skin"),
			_("Battery Cell 1"),
			_("Battery Cell 2"),
			_("Battery Cell 3"),
			_("Battery Cell 4"),
			_("Camera Module"),
		];
#endif
		extern NSArray *getHIDSkinModelsOf(NSString *prod);
		// XXX: Try to figure out more
		// TODO: Warn on invalid VTs
		// TODO: Show die VTs
		// TG*B: 15 ~ 46
		// Die: 17 ~ 75
		// TSFC: 8 ~ 46
		NSDictionary *dict = @{
			@"Device Avg.": getHIDSkinModelsOf([NSString stringWithCString:target_type() encoding:NSUTF8StringEncoding]),
			// @"iPad Skin": @"TSBM",
			@"Battery Cell 1": @"TG0B",
			@"Battery Cell 2": @"TG1B",
			@"Battery Cell 3": @"TG2B",
			@"Battery Cell 4": @"TG3B",
			@"Camera Module": @"TSFC",
		};
		NSArray<NSString *> *keys = [dict allKeys];
		NSArray *vals = [dict allValues];
		for (NSUInteger i = 0; i < dict.count; i++) {
			NSString *className = NSStringFromClass([vals[i] class]);
			if ([className isEqualToString:@"__NSArrayI"] || [className isEqualToString:@"NSArray"]) {
				NSArray *buf = (NSArray *)vals[i];
				float avg = 0;
				int valid_count = 0;
				for (NSUInteger j = 0; j < buf.count; j++) {
					float temp = getTemperatureHIDAt(buf[j]);
					if (temp != -1) {
						avg += temp;
						valid_count++;
					}
				}
				if (valid_count) {
					avg /= valid_count;
					[knownHIDSensors setValue:[NSNumber numberWithFloat:avg] forKey:keys[i]];
				}
			}
			if ([className isEqualToString:@"__NSCFConstantString"] || [className isEqualToString:@"NSString"]) {
				float temp = getTemperatureHIDAt(vals[i]);
				if (temp != -1) {
					[knownHIDSensors setValue:[NSNumber numberWithFloat:temp] forKey:keys[i]];
				}
			}
		}
	}
	return self;
}

- (NSString *)title {
	return _("Hardware Temperature");
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tv {
	return 3;
}

- (NSInteger)tableView:(id)tv numberOfRowsInSection:(NSInteger)section {
	if (section == 0) {
		return sensorTemperatures.count;
	} else if (section == 1) {
		return knownHIDSensors.count;
	}
	return temperatureHIDData.count;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
	switch (section) {
		case 0: return _("Device Sensors");
		case 1: return _("HID");
	}
	return _("HID Raw Data");
}

- (UITableViewCell *)tableView:(UITableView *)tv cellForRowAtIndexPath:(NSIndexPath *)ip {
	UITableViewCell *cell = [tv dequeueReusableCellWithIdentifier:@"stvc:main"];
	if (!cell)
		cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"stvc:main"];
	NSDictionary *dict = NULL;
	NSString *label = NULL;
	/* TODO: thermnote */

	if (ip.section == 0) {
		dict = sensorTemperatures;
		// ????? this is terrible, why not store cstring at beginning?
		label = _([dict.allKeys[ip.row] UTF8String]);
	} else if (ip.section == 1) {
		dict = knownHIDSensors;
		label = _([dict.allKeys[ip.row] UTF8String]);
	} else if (ip.section == 2) {
		/* TODO: Filter stub & info only VTs */
		/* Some sensors are not actually getting its temperature
		   they always looks like 0.00 or 30.00 */

		/* TODO: Better UI for sensors having Avg & Cur & Max */
		/* Not every HID temp sensors recording realtime values */
		dict = temperatureHIDData;
		label = dict.allKeys[ip.row];
	}
	cell.textLabel.text = label;
	cell.detailTextLabel.text = [NSString stringWithFormat:@"%.4g ℃", [dict[dict.allKeys[ip.row]] floatValue]];

	/* TODO: thermtune */
	return cell;
}

@end
