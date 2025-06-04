//
//  NSObject+UPSMonitor.h
//  Battman
//
//  Created by Torrekie on 2025/6/3.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface UPSMonitor : NSObject
+ (void)startWatchingUPS;
@end

NS_ASSUME_NONNULL_END
