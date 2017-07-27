//
//  Connector_Third.m
//  IOSFrame
//
//  Created by 安昌 on 2017/7/26.
//  Copyright © 2017年 安昌. All rights reserved.
//

#import "Connector_Third.h"
#import "LDBusMediator.h"
#import "RouterURLInfo.h"
#import "ThirdViewController.h"

@implementation Connector_Third

#pragma mark- 注册连接器

+(void)load{
    @autoreleasepool {
        [LDBusMediator registerConnector:[self sharedConnector] withName:Third_URL];
    }
}

+(nonnull Connector_Third*)sharedConnector{
    static Connector_Third *_sharedConnector;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        _sharedConnector = [[Connector_Third alloc] init];
    });
    
    return _sharedConnector;
}

#pragma mark-实现url连接方法

-(BOOL)canOpenURL:(NSURL *)URL{
    if ([URL.host isEqualToString:Third_URL]) {
        return YES;
    }
    return NO;
}

-(nullable UIViewController*)connectToOpenURL:(NSURL *)URL params:(NSDictionary *)params{
    
    if ( [URL.host isEqualToString:Third_URL] ) {
        ThirdViewController *controller = [[ThirdViewController alloc] init];
        
        return controller;
    }
    
    return nil;
}

@end
