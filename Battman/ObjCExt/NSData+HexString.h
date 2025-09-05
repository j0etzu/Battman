//
//  NSData+HexString.h
//  Battman
//
//  Created by Torrekie on 2025/7/7.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface NSData (HexString)
+(id)dataWithHexString:(NSString*)hex;
@end

NS_ASSUME_NONNULL_END
