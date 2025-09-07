//
//  DatePickerCompactButton.m
//  Battman
//
//  Created by Torrekie on 2025/8/28.
//

#import "DatePickerCompactButton.h"

@interface CALayer ()
@property (atomic, assign, readwrite) BOOL continuousCorners;
@end

@implementation DatePickerCompactButton

- (instancetype)initWithTitle:(NSString *)title {
	if (self = [super initWithFrame:CGRectZero]) {
		[self commonInit];
		[self setTitle:title forState:UIControlStateNormal];
		[self sizeToFit];
	}
	return self;
}

- (instancetype)initWithFrame:(CGRect)frame {
	if (self = [super initWithFrame:frame]) {
		[self commonInit];
	}
	return self;
}

- (instancetype)initWithCoder:(NSCoder *)aDecoder {
	if (self = [super initWithCoder:aDecoder]) {
		[self commonInit];
	}
	return self;
}

- (void)commonInit {
	[self.heightAnchor constraintEqualToConstant:34.3333].active = YES;
	if (@available(iOS 13.0, *)) {
		[self setTitleColor:[UIColor linkColor] forState:UIControlStateSelected];
		[self setTitleColor:[UIColor labelColor] forState:UIControlStateNormal];
		
		self.backgroundColor = [UIColor tertiarySystemFillColor];
	} else {
		[self setTitleColor:[UIColor colorWithRed:0 green:(122.0f / 255) blue:1 alpha:1] forState:UIControlStateSelected];
		[self setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
		
		self.backgroundColor = [UIColor colorWithRed:(118.0f / 255) green:(118.0f / 255) blue:(128.0f / 255) alpha:0.12f];
	}
	self.contentEdgeInsets = UIEdgeInsetsMake(4, 8, 4, 8);
	// Rounded border
	self.layer.bounds = CGRectMake(0, 0, self.layer.frame.size.width * 1.5, self.layer.frame.size.height * 1.5);
	self.layer.cornerRadius = 8;
	self.layer.masksToBounds = YES;
	if (@available(iOS 13.0, *)) {
		[self.layer setCornerCurve:kCACornerCurveContinuous];
	}
	if ([self.layer respondsToSelector:@selector(setContinuousCorners:)])
		[self.layer setContinuousCorners:YES];
}

@end
