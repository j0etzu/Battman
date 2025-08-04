#import <UIKit/UIKit.h>
#import "UberSegmentedControl/UberSegmentedControl.h"

@interface SegmentedHVCViewCell : UITableViewCell

@property (nonatomic, strong, nonnull) UILabel *titleLabel;
@property (nonatomic, strong, nullable) UILabel *detailLabel;
@property (nonatomic, strong, nullable) UIView *accessory;
@property (nonatomic, strong, nullable) UISegmentedControl *segmentedControl;
@property (nonatomic, strong, nullable) UILabel *subTitleLabel;
@property (nonatomic, strong, nullable) UILabel *subDetailLabel;

@end

@interface SegmentedFlagViewCell : UITableViewCell

@property (nonatomic, strong, nonnull) UILabel *titleLabel;
@property (nonatomic, strong, nullable) UILabel *detailLabel;

@property (nonatomic, strong, nonnull) NSArray *highBitSet;
@property (nonatomic, strong, nonnull) NSArray *lowBitSet;

@property (nonatomic, strong, nullable) UberSegmentedControl *highByte;
@property (nonatomic, strong, nullable) UberSegmentedControl *lowByte;

@property (nonatomic) UInt32 flags;

- (void)selectByFlags:(UInt32)flags;
- (void)setBitSetByModel:(NSString * _Nonnull)name;
- (void)setBitSetByTargetName;
- (void)setBitSetByGuess;

@end
