//
//  GradientArcView.m
//  Battman
//
//  Created by Torrekie on 2025/3/15.
//

#import "common.h"
#import "GradientArcView.h"

/* Apple lied to us, CGColorCreateGenericRGB is already a thing
 */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability-new"

@implementation GradientArcView

- (instancetype)initWithFrame:(CGRect)frame {
    //DBGLOG(@"initWithFrame");
    self = [super initWithFrame:frame];
    self.backgroundColor = [UIColor clearColor];
    return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder {
    //DBGLOG(@"initWithCoder");
    self = [super initWithCoder:coder];
    self.backgroundColor = [UIColor clearColor];
    return self;
}

- (void)setupPointerLayer {
    //DBGLOG(@"setupPointerLayer");
    self.pointerLayer = [CAShapeLayer layer];
    // we want to center it at the bottom of arc.
    // Update this in layoutSubviews to match your drawing calculations.
    self.pointerLayer.position = CGPointMake(centerX, centerY);

    if (!radius) {
        DBGLOG(@"setupPointerLayer too early!");
    }
    CGFloat round_radius = radius * 0.2;
    CGMutablePathRef roundPath = CGPathCreateMutable();
    CGRect roundRect = CGRectMake(self.pointerLayer.bounds.size.width / 2 - (round_radius / 2.0), self.pointerLayer.bounds.size.height / 2 - (round_radius / 2.0), round_radius, round_radius);
    CGPathAddEllipseInRect(roundPath, NULL, roundRect);
    CAShapeLayer *roundLayer = [CAShapeLayer layer];
    roundLayer.path = roundPath;
    roundLayer.fillColor = CGColorCreateGenericRGB(0.9, 0.0, 0.0, 1.0);
    [self.pointerLayer addSublayer:roundLayer];

    CGMutablePathRef trianglePath = CGPathCreateMutable();
    CGPathMoveToPoint(trianglePath, NULL, self.pointerLayer.bounds.size.width / 2.0, self.pointerLayer.bounds.size.height / 2.0 - (round_radius / 2.0));
    CGPathAddLineToPoint(trianglePath, NULL, -radius, self.pointerLayer.bounds.size.width / 2.0);
    CGPathAddLineToPoint(trianglePath, NULL, self.pointerLayer.bounds.size.width / 2.0, self.pointerLayer.bounds.size.height / 2.0 + (round_radius / 2.0));
    CGPathCloseSubpath(trianglePath);
    CAShapeLayer *triangleLayer = [CAShapeLayer layer];
    triangleLayer.path = trianglePath;
    triangleLayer.fillColor = CGColorCreateGenericRGB(0.9, 0.0, 0.0, 1.0);
    //triangleLayer.opacity = 0.5;
    triangleLayer.anchorPoint = CGPointMake(0.5, 1.0);
    triangleLayer.frame = [self.pointerLayer bounds];
    triangleLayer.position = CGPointMake(self.pointerLayer.bounds.size.width / 2.0, self.pointerLayer.bounds.size.height);
    [self.pointerLayer addSublayer:triangleLayer];
    
    [self.layer addSublayer:self.pointerLayer];
}

- (void)layoutSubviews {
    //DBGLOG(@"layoutSubviews");
    [super layoutSubviews];
}

- (void)rotatePointerToAngle:(CGFloat)angle {
    //DBGLOG(@"rotatePointerToAngle");
    if (!self.pointerLayer) {
        DBGLOG(@"rotatePointerToAngle called too early!");
        // If pointerLayer is nil, force a redraw and schedule the rotation shortly after.
        [self setNeedsDisplay];
        [self layoutIfNeeded];
        dispatch_async(dispatch_get_main_queue(), ^{
            [self rotatePointerToAngle:angle];
        });
        return;
    }
    CABasicAnimation *rotationAnimation = [CABasicAnimation animationWithKeyPath:@"transform.rotation.z"];
    rotationAnimation.fromValue = @(previousAngle);
    rotationAnimation.toValue = @(angle);
    rotationAnimation.duration = 1; // Animation duration in seconds
    rotationAnimation.fillMode = kCAFillModeBoth;
    rotationAnimation.removedOnCompletion = NO;
    
    [self.pointerLayer addAnimation:rotationAnimation forKey:@"rotatePointer"];
    
    previousAngle = angle;
}

- (void)rotatePointerToPercentage:(CGFloat)percent {
    [self rotatePointerToAngle:6 * M_PI_4 * percent - M_PI_4];
}

- (void)drawRect:(CGRect)rect {
    //NSLog(@"drawRect");
#if TARGET_OS_IPHONE
    CGContextRef context = UIGraphicsGetCurrentContext();
#else
    CGContextRef context = [[NSGraphicsContext currentContext] graphicsPort];
#endif
    [self.backgroundColor set];

    // Define the arc parameters
    centerX = CGRectGetMidX(rect);
    centerY = CGRectGetMidY(rect);
    radius = MIN(centerX, centerY) * 0.7;
    startAngle = -M_PI_4 * 5;  // -225
    endAngle = M_PI_4;         // 45
    CGFloat lineWidth = rect.size.width / 10.0;
    
    // Calculate total angle
    CGFloat totalAngle = endAngle - startAngle;
    if (totalAngle < 0) {
        totalAngle += 2 * M_PI;
    }
    CGFloat sagitta;
    if (totalAngle <= M_PI) {
        // Smaller arc
        sagitta = radius * (1.0 - cos(totalAngle / 2.0));
    } else {
        // Larger arc
        // or: sagitta = 2*R - [the smaller arc sagitta for (2π - totalAngle)]
        sagitta = radius * (1.0 + cos((2.0 * M_PI - totalAngle) / 2.0));
    }
    // sagitta + sqrt(lineWidth) = boldedArcHeight
    // the calculated sagitta did not count the lineWidth expanded areas
    // directly sqrt() on diagonal line which is the lineWidth since the angle is 45/-225
    // which made the extra part a right triangle
    centerY = centerY + ((2 * radius - sagitta - sqrt(lineWidth)) / 2.0);

    // Number of segments to create segments
    NSInteger segments = 16;
    
    // Save the context state
    CGContextSaveGState(context);
    
    // Create color space
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    
    // Create the arc path for clipping
    CGMutablePathRef clipPath = CGPathCreateMutable();
    CGPathAddArc(clipPath, NULL, centerX, centerY, radius, startAngle, endAngle, NO);
    
    // Set up clipping path
    CGContextAddPath(context, clipPath);
    CGContextSetLineWidth(context, lineWidth);
    CGContextSetStrokeColorWithColor(context, CGColorCreateGenericRGB(0, 0, 0, 1));
    CGContextSetLineCap(context, kCGLineCapButt);
    CGContextReplacePathWithStrokedPath(context);
    CGContextClip(context);
    
    // Draw gradient segments
    for (NSInteger i = 0; i < segments; i++) {
        CGFloat fraction = (CGFloat)i / (segments - 1);
        
        // Calculate color for this segment
        CGFloat t;
        CGColorRef color;
        if (fraction <= 0.33) {
            // Cyan to Green (0.0 - 0.33)
            t = fraction / 0.33;
            color = CGColorCreateGenericRGB(0.0, 1.0, (1.0 - t), 1.0);
        } else if (fraction <= 0.66) {
            // Green to Yellow (0.33 - 0.66)
            t = (fraction - 0.33) / 0.33;
            color = CGColorCreateGenericRGB(t - 0.1, 1.0, 0.0, 1.0);
        } else {
            // Yellow to Red (0.66 - 1.0)
            t = (fraction - 0.66) / 0.34;
            color = CGColorCreateGenericRGB(1.0, (0.9 - t), 0.0, 1.0);
        }
        
        // Calculate angles for this segment
        CGFloat angle = startAngle + (totalAngle * fraction);
        CGFloat nextAngle = startAngle + (totalAngle * ((CGFloat)(i + 1) / (segments - 1)));
        
        // Draw the segment
        CGMutablePathRef segmentPath = CGPathCreateMutable();
        CGPathAddArc(segmentPath, NULL, centerX, centerY, radius, angle, nextAngle, NO);
        
        CGContextAddPath(context, segmentPath);
        CGContextSetStrokeColorWithColor(context, color);
        CGContextSetLineWidth(context, lineWidth);
        CGContextSetLineCap(context, kCGLineCapSquare); // Use butt cap for segments to avoid gaps
        CGContextStrokePath(context);
        
        CGPathRelease(segmentPath);
    }
    
    // Clean up
    CGPathRelease(clipPath);
    CGColorSpaceRelease(colorSpace);
    CGContextRestoreGState(context);

    if (!self.pointerLayer) {
        [self setupPointerLayer];
    }
    // Update the pointerLayer's position to match the recalculated arc center
    self.pointerLayer.position = CGPointMake(centerX, centerY);
}

@end

#pragma clang diagnostic pop
