//
//  PickerAccessoryView.h
//  Battman
//
//  Created by Torrekie on 2025/8/29.
//

#import <UIKit/UIKit.h>

@interface PickerAccessoryView : UIPickerView

@property (nonatomic, strong) NSArray *options;

- (instancetype)initWithFrame:(CGRect)frame font:(UIFont *)font options:(NSArray *)options;

- (void)selectAutomaticRow:(NSInteger)row animated:(BOOL)animated;

- (void)addTarget:(id)target action:(SEL)action;

@end
