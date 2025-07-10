//
//  GradientArcView.h
//  Battman
//
//  Created by Torrekie on 2025/3/15.
//

#import <UIKit/UIKit.h>

@interface GradientArcView : UIView {
    CGFloat centerX, centerY;
    CGFloat radius;
    CGFloat startAngle, endAngle;
    CGFloat previousAngle;
}
@property (nonatomic, strong) CAShapeLayer *pointerLayer;

- (void)rotatePointerToPercentage:(CGFloat)percent;
- (void)rotatePointerToPercentage:(CGFloat)percent duration:(NSTimeInterval)duration;
@end
