#import "MultilineViewCell.h"

@implementation MultilineViewCell

- (instancetype)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier {
    self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
    if (self) {
        // Remove or ignore the built-in textLabel
        self.textLabel.hidden = YES;
        self.textLabel.text = @"Multiline Title";

        // Setup titleLabel
        self.titleLabel = [[UILabel alloc] init];
        self.titleLabel.translatesAutoresizingMaskIntoConstraints = NO;
        self.titleLabel.font = self.textLabel.font;
        [self.contentView addSubview:self.titleLabel];

        // Setup detailLabel as a UILabel for simplicity
        _detailTextLabel = [[UILabel alloc] init];
        self.detailTextLabel.translatesAutoresizingMaskIntoConstraints = NO;
        // For use as our custom labels' template
        UITableViewCell *cell;
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:nil];
        cell.hidden = YES;
        [self.contentView addSubview:cell];
        cell.detailTextLabel.text = @"DETAIL";
        self.detailTextLabel.numberOfLines = 0;
        self.detailTextLabel.font = cell.detailTextLabel.font;
        self.detailTextLabel.textColor = cell.detailTextLabel.textColor;
        self.detailTextLabel.textAlignment = cell.detailTextLabel.textAlignment;
        [self.contentView addSubview:self.detailTextLabel];

        // Setup Auto Layout constraints relative to contentView
        [NSLayoutConstraint activateConstraints:@[
            // Title label constraints
            [self.titleLabel.topAnchor constraintEqualToAnchor:self.textLabel.topAnchor constant:(self.frame.size.height - self.textLabel.font.pointSize) / 2],
            [self.titleLabel.leadingAnchor constraintEqualToAnchor:self.textLabel.leadingAnchor],
            [self.titleLabel.trailingAnchor constraintEqualToAnchor:self.textLabel.trailingAnchor],
            
            // Detail label constraints
            [self.detailTextLabel.topAnchor constraintEqualToAnchor:self.textLabel.topAnchor constant:(self.frame.size.height - self.textLabel.font.pointSize) / 2],
            [self.detailTextLabel.leadingAnchor constraintEqualToAnchor:self.textLabel.leadingAnchor],
            [self.detailTextLabel.trailingAnchor constraintEqualToAnchor:self.textLabel.trailingAnchor],
            [self.detailTextLabel.bottomAnchor constraintEqualToAnchor:self.contentView.bottomAnchor constant:-12],
        ]];
    }
    return self;
}

- (id)detailTextLabel {
	return _detailTextLabel;
}

@end
