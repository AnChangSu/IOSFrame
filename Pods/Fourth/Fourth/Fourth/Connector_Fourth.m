//
//  Connector_Fourth.m
//  IOSFrame
//
//  Created by 安昌 on 2017/7/26.
//  Copyright © 2017年 安昌. All rights reserved.
//

#import "Connector_Fourth.h"
#import "LDBusMediator.h"
#import "RouterURLInfo.h"
#import "FourthViewController.h"

@implementation Connector_Fourth
#pragma mark- 注册连接器

+(void)load{
    @autoreleasepool {
        [LDBusMediator registerConnector:[self sharedConnector] withName:Fourth_URL];
    }
}

+(nonnull Connector_Fourth*)sharedConnector{
    static Connector_Fourth *_sharedConnector;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        _sharedConnector = [[Connector_Fourth alloc] init];
    });
    
    return _sharedConnector;
}

#pragma mark-实现url连接方法

-(BOOL)canOpenURL:(NSURL *)URL{
    if ([URL.host isEqualToString:Fourth_URL]) {
        return YES;
    }
    return NO;
}

-(nullable UIViewController*)connectToOpenURL:(NSURL *)URL params:(NSDictionary *)params{
    
    if ( [URL.host isEqualToString:Fourth_URL] ) {
        FourthViewController *controller = [[FourthViewController alloc] init];
        
        return controller;
    }
    
    return nil;
}

@end
