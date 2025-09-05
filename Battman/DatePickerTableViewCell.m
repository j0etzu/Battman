//
//  DatePickerTableViewCell.m
//  testpicker
//
//  Created by Torrekie on 2025/7/3.
//

#import "DatePickerTableViewCell.h"

@interface DatePickerTableViewCell ()
@end

@implementation DatePickerTableViewCell

- (instancetype)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier {
	if (self = [super initWithStyle:style reuseIdentifier:reuseIdentifier]) {
		_isExpanded = NO; // Initially collapsed
		
		// This must exist for our custom labels to align
		self.textLabel.text = @"TITLE";
		// Hide
		self.textLabel.hidden = YES;
		self.accessoryView.hidden = YES;
		
		// Alternative title
		_titleLabel = [[UILabel alloc] init];
		_titleLabel.translatesAutoresizingMaskIntoConstraints = NO;
		_titleLabel.font = self.textLabel.font;
		[self.contentView addSubview:_titleLabel];
		
		_button = [[DatePickerCompactButton alloc] initWithTitle:@"-"];
		_button.translatesAutoresizingMaskIntoConstraints = NO;
		[_button addTarget:self action:@selector(buttonTapped:) forControlEvents:UIControlEventTouchUpInside];
		
		// Should not be accessory
		[self.contentView addSubview:_button];
		_button.titleLabel.font = self.textLabel.font;
		
		_picker = [[UIDatePicker alloc] initWithFrame:CGRectZero];
		_picker.translatesAutoresizingMaskIntoConstraints = NO;
		// Force whells (inline is buggy in current impl)
		if (@available(iOS 13.4, *)) {
			_picker.preferredDatePickerStyle = UIDatePickerStyleWheels;
		}
		_picker.date = [NSDate date];
		[_picker addTarget:self action:@selector(pickerChanged:) forControlEvents:UIControlEventValueChanged];
		_pickerBorder = [[UIView alloc] init];
		_pickerBorder.translatesAutoresizingMaskIntoConstraints = NO;
		[_pickerBorder addSubview:_picker];
		[_pickerBorder sizeToFit];
		
		[self.contentView addSubview:_pickerBorder];
		
		
		_pickerBorder.hidden = !_isExpanded; // Initially hidden
		
		_pickerHeightConstraint = [_pickerBorder.heightAnchor constraintEqualToConstant:0];
		[NSLayoutConstraint activateConstraints:@[
			// Title label
			[_titleLabel.topAnchor constraintEqualToAnchor:self.textLabel.topAnchor constant:(self.frame.size.height - self.textLabel.font.pointSize) / 2],
			[_titleLabel.leadingAnchor constraintEqualToAnchor:self.textLabel.leadingAnchor],
			[_titleLabel.trailingAnchor constraintEqualToAnchor:self.contentView.trailingAnchor constant:-16],
			
			// Button
			[_button.topAnchor constraintEqualToAnchor:self.textLabel.topAnchor constant:(self.frame.size.height - _button.frame.size.height) / 2 - 2],
			//[_button.leadingAnchor constraintEqualToAnchor:self.textLabel.leadingAnchor],
			[_button.trailingAnchor constraintEqualToAnchor:self.contentView.trailingAnchor constant:-16],
			
			_pickerHeightConstraint,
			[_picker.centerXAnchor constraintEqualToAnchor:self.centerXAnchor constant:0],
			[_pickerBorder.topAnchor constraintGreaterThanOrEqualToAnchor:_button.bottomAnchor constant:-4],
			[_pickerBorder.leadingAnchor constraintEqualToAnchor:self.contentView.leadingAnchor constant:16],
			[_pickerBorder.trailingAnchor constraintEqualToAnchor:self.contentView.trailingAnchor constant:-16],
			
			[_pickerBorder.bottomAnchor constraintEqualToAnchor:self.contentView.bottomAnchor constant:-8],
		]];
	}
	return self;
}

- (void)buttonTapped:(UIButton *)sender {
	self.isExpanded = !self.isExpanded;
	[self updatePickerVisibility:YES];
	
	_button.selected = self.isExpanded;
	// Notify delegate about the change so table view can update its height
	if ([self.delegate respondsToSelector:@selector(datePickerCellDidToggleExpansion:)]) {
		[self.delegate datePickerCellDidToggleExpansion:self];
	}
}

- (void)setIsExpanded:(BOOL)isExpanded {
	if (_isExpanded != isExpanded) {
		_isExpanded = isExpanded;
		[self updatePickerVisibility:NO];
	}
}

- (void)updatePickerVisibility:(BOOL)animated {
	if (animated) {
		[UIView animateWithDuration:0.5 animations:^{
			[self configurePickerConstraints];
			[self.contentView layoutIfNeeded];
		}];
	} else {
		[self configurePickerConstraints];
	}
}

- (void)configurePickerConstraints {
	if (self.isExpanded) {
		_pickerBorder.hidden = NO;
		_pickerHeightConstraint.active = NO;
	} else {
		_pickerHeightConstraint.active = YES;
		_pickerHeightConstraint.constant = 0;
	}
}

- (void)pickerChanged:(UIDatePicker *)sender {
	[_button setTitle:[NSDateFormatter localizedStringFromDate:sender.date dateStyle:NSDateFormatterShortStyle timeStyle:NSDateFormatterShortStyle] forState:UIControlStateNormal];
}

@end
