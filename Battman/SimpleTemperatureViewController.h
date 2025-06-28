#import "BatterySubscriberViewControllerBase.h"

@interface SimpleTemperatureViewController : BatterySubscriberViewControllerBase
{
	NSDictionary *temperatureHIDData;
	NSDictionary *sensorTemperatures;
}
@end
