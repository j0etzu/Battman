//
//  SliderTableViewCell.h
//  Battman
//
//  Created by Torrekie on 2025/5/1.
//

#import <UIKit/UIKit.h>

@protocol SliderTableViewCellDelegate;

@interface SliderTableViewCell : UITableViewCell <UITextFieldDelegate>

@property (nonatomic, strong) UISlider *slider;
@property (nonatomic, strong) UITextField *textField;
@property (nonatomic, weak) id<SliderTableViewCellDelegate> delegate;

@end

@protocol SliderTableViewCellDelegate <NSObject>
@optional
- (void)sliderTableViewCell:(SliderTableViewCell *)cell didChangeValue:(float)value;
- (void)sliderTableViewCell:(SliderTableViewCell *)cell didEndChangingValue:(float)value;
- (void)sliderTableViewCellDidBeginChanging:(SliderTableViewCell *)cell;
@end
