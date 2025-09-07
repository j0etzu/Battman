//
//  SliderTableViewCell.m
//  Battman
//
//  Created by Torrekie on 2025/5/1.
//

#import "SliderTableViewCell.h"

@implementation SliderTableViewCell

- (instancetype)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier {
    if (self = [super initWithStyle:style reuseIdentifier:reuseIdentifier]) {
        // Create slider
        _slider = [[UISlider alloc] initWithFrame:CGRectZero];
        _slider.translatesAutoresizingMaskIntoConstraints = NO;
        [_slider addTarget:self action:@selector(sliderValueChanged:) forControlEvents:UIControlEventValueChanged];
		[_slider addTarget:self action:@selector(sliderTouchDown:) forControlEvents:UIControlEventTouchDown];
		[_slider addTarget:self action:@selector(sliderTouchUp:) forControlEvents:UIControlEventTouchUpInside | UIControlEventTouchUpOutside | UIControlEventTouchCancel];
        [self.contentView addSubview:_slider];
        
        // Create text field
        _textField = [[UITextField alloc] initWithFrame:CGRectZero];
        _textField.translatesAutoresizingMaskIntoConstraints = NO;
        _textField.textAlignment = NSTextAlignmentCenter;
        _textField.keyboardType = UIKeyboardTypeDecimalPad;  // Decimal number input
        _textField.returnKeyType = UIReturnKeyDone;
        _textField.placeholder = @"0";
        _textField.delegate = self;  // Set the delegate to self to handle textFieldDidEndEditing
        [self.contentView addSubview:_textField];
        
        // Layout constraints for slider and text field
        UILayoutGuide *m = self.contentView.layoutMarginsGuide;
        [NSLayoutConstraint activateConstraints:@[
            [self.slider.leadingAnchor constraintEqualToAnchor:m.leadingAnchor],
            [self.slider.trailingAnchor constraintEqualToAnchor:_textField.leadingAnchor constant:-10],
            [self.slider.centerYAnchor constraintEqualToAnchor:self.contentView.centerYAnchor],
            
            [self.textField.trailingAnchor constraintEqualToAnchor:m.trailingAnchor],
            [self.textField.centerYAnchor constraintEqualToAnchor:self.contentView.centerYAnchor],
            [self.textField.widthAnchor constraintEqualToConstant:60]
        ]];
    }
    return self;
}

- (void)sliderValueChanged:(UISlider *)sender {
    // Update text field when slider value changes
    self.textField.text = [NSString stringWithFormat:@"%.4g", sender.value];
    if ([self.delegate respondsToSelector:@selector(sliderTableViewCell:didChangeValue:)]) {
        [self.delegate sliderTableViewCell:self didChangeValue:sender.value];
    }
}

- (void)sliderTouchDown:(UISlider *)sender {
	if ([self.delegate respondsToSelector:@selector(sliderTableViewCellDidBeginChanging:)]) {
		[self.delegate sliderTableViewCellDidBeginChanging:self];
	}
}

- (void)sliderTouchUp:(UISlider *)sender {
	// Make sure text field shows final value
	self.textField.text = [NSString stringWithFormat:@"%.4g", sender.value];
	
	if ([self.delegate respondsToSelector:@selector(sliderTableViewCell:didEndChangingValue:)]) {
		[self.delegate sliderTableViewCell:self didEndChangingValue:sender.value];
	}
}

#pragma mark - UITextFieldDelegate

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
    [textField resignFirstResponder];
    return YES;
}

- (void)textFieldDidEndEditing:(UITextField *)sender {
    // Validate the input: Ensure it's a valid number within the slider's min and max range
    NSString *inputText = sender.text;
    NSCharacterSet *nonNumberCharacterSet = [[NSCharacterSet decimalDigitCharacterSet] invertedSet];
    inputText = [inputText stringByTrimmingCharactersInSet:nonNumberCharacterSet];
    
    float newValue = [inputText floatValue];

    if (inputText.length == 0 || newValue == 0) {
        newValue = 80;
    }
        
    // Ensure the value is within the slider's min and max range
    newValue = fminf(fmaxf(newValue, self.slider.minimumValue), self.slider.maximumValue);
    self.slider.value = newValue;
    sender.text = [NSString stringWithFormat:@"%.4g", newValue];
        
    if ([self.delegate respondsToSelector:@selector(sliderTableViewCell:didChangeValue:)]) {
        [self.delegate sliderTableViewCell:self didChangeValue:newValue];
    }
}

@end
