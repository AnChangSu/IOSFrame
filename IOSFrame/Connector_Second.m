//
//  Connector_Second.m
//  IOSFrame
//
//  Created by 安昌 on 2017/7/26.
//  Copyright © 2017年 安昌. All rights reserved.
//

#import "Connector_Second.h"
#import "LDBusMediator.h"
#import "SecondViewController.h"
#import "RouterURLInfo.h"

@implementation Connector_Second

#pragma mark- 注册连接器
+(void)load{
    @autoreleasepool {
        [LDBusMediator registerConnector:[self sharedConnector] withName:Second_RUL];
    }
}

+(nonnull Connector_Second*)sharedConnector{
    static Connector_Second *_sharedConnector = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        _sharedConnector = [[Connector_Second alloc] init];
    });
    
    return _sharedConnector;
}

#pragma mark-实现url连接方法

-(BOOL)canOpenURL:(NSURL *)URL{
    if ([URL.host isEqualToString:Second_RUL]) {
        return YES;
    }
    
    return NO;
}

-(nullable UIViewController *)connectToOpenURL:(NSURL *)URL params:(NSDictionary *)params
{
    if ([URL.host isEqualToString:Second_RUL]) {
        SecondViewController *controller = [[SecondViewController alloc] init];
        
        return controller;
    }
    
    return nil;
}

@end
