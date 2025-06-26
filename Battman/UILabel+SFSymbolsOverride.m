//
//  UILabel+SFSymbolsOverride.m
//  Battman
//
//  Created by Torrekie on 2025/6/20.
//

#import "UILabel+SFSymbolsOverride.h"
#import <objc/runtime.h>

@implementation UILabel (SFSymbolsOverride)

// Range for Private Use Area‑B: U+100000 … U+10FFFD
static BOOL isPUAB(unichar high, unichar low) {
	// Build the 21‑bit scalar from the surrogate pair
	uint32_t highBits = (uint32_t)(high  - 0xD800) << 10;
	uint32_t lowBits  = (uint32_t)(low   - 0xDC00);
	uint32_t scalar   = highBits + lowBits + 0x10000;
	return (scalar >= 0x100000 && scalar <= 0x10FFFD);
}

+ (void)load {
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		Class cls = [UILabel class];
		SEL origSel = @selector(setText:);
		SEL newSel  = @selector(swizzled_setText:);
		
		Method m1 = class_getInstanceMethod(cls, origSel);
		Method m2 = class_getInstanceMethod(cls, newSel);
		
		method_exchangeImplementations(m1, m2);
	});
}

- (void)swizzled_setText:(NSString *)text {
	if (text == nil) {
		// If they clear the text, just call through to the original setter
		[self swizzled_setText:nil];
		return;
	}
	
	// Build an attributed string that swaps in the symbol font on PUA‑B codepoints
	UIFont *bodyFont   = self.font ?: [UIFont systemFontOfSize:[UIFont labelFontSize]];
	UIFont *symbolFont = [UIFont fontWithName:@"SFProDisplay-Regular" size:bodyFont.pointSize];
	
	NSMutableAttributedString *m = [[NSMutableAttributedString alloc] initWithString:text attributes:@{ NSFontAttributeName: bodyFont }];
	
	// Walk the string as UTF‑16, detect surrogate pairs
	for (NSUInteger i = 0; i + 1 < text.length; ++i) {
		unichar c1 = [text characterAtIndex:i];
		if (0xD800 <= c1 && c1 <= 0xDBFF) {
			unichar c2 = [text characterAtIndex:i+1];
			if (0xDC00 <= c2 && c2 <= 0xDFFF && isPUAB(c1,c2)) {
				// Replace that pair with the symbol font
				[m addAttribute:NSFontAttributeName value:symbolFont range:NSMakeRange(i,2)];
			}
			i++; // skip the low surrogate
		}
	}
	
	// Finally call the original -setText: (really swizzled_setText:)
	[self swizzled_setText:nil];
	[self setAttributedText:m];
}

@end
