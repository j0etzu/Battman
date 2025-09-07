//
//  DatePickerTableViewCell.h
//  testpicker
//
//  Created by Torrekie on 2025/7/3.
//

#import <UIKit/UIKit.h>
#import "DatePickerCompactButton.h"

@protocol DatePickerTableViewCellDelegate;

@interface DatePickerTableViewCell : UITableViewCell
@property (nonatomic, strong) UILabel *titleLabel;
@property (nonatomic, strong) DatePickerCompactButton *button;
@property (nonatomic, strong) UIDatePicker *picker;
@property (nonatomic, strong) UIView *pickerBorder;


@property (nonatomic, assign) BOOL isExpanded;
@property (nonatomic, weak) id<DatePickerTableViewCellDelegate> delegate;
@property (nonatomic, strong) NSLayoutConstraint *pickerHeightConstraint;
@end

@protocol DatePickerTableViewCellDelegate <NSObject>
- (void)datePickerCellDidToggleExpansion:(DatePickerTableViewCell *)cell;
@end
