//
//  Connector_First.m
//  IOSFrame
//
//  Created by 安昌 on 2017/7/26.
//  Copyright © 2017年 安昌. All rights reserved.
//

#import "Connector_First.h"
#import "LDBusMediator.h"
#import "RouterURLInfo.h"
#import "ModelUserInfoServicePrt.h"
#import "UserInfo.h"

#import "FirstViewController.h"
#import "FirstDetailViewController.h"

@interface Connector_First ()<ModelUserInfoServicePrt>

@end

@implementation Connector_First

@synthesize currentController = _currentController;

#pragma mark- 注册连接器

+(void)load{
    @autoreleasepool {
        [LDBusMediator registerConnector:[self sharedConnector] withName:First_URL];
    }
}

+(nonnull Connector_First*)sharedConnector{
    static Connector_First *_sharedConnector = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        _sharedConnector = [[Connector_First alloc] init];
    });
    
    return _sharedConnector;
}

#pragma mark - url连接方法 LDBusConnectorPrt

-(BOOL)canOpenURL:(NSURL *)URL
{
    if ([URL.host isEqualToString:First_URL]
       || [URL.host isEqualToString:First_Detail_URL]) {
        return YES;
    }
    
    return NO;
}

-(nullable UIViewController *)connectToOpenURL:(NSURL *)URL params:(NSDictionary *)params{
    if ([URL.host isEqualToString:First_URL]) {
        FirstViewController  *controller= [[FirstViewController alloc] init];
        self.currentController = controller;
        
        return controller;
    }else if ( [URL.host isEqualToString:First_Detail_URL] ){
        FirstDetailViewController *controller = [[FirstDetailViewController alloc] init];
        self.currentController = controller;
        
        return controller;
    }
    
    return nil;
}

-(nullable UIViewController*)getCurrentController{
    return self.currentController;
}

- (nullable id)connectToHandleProtocol:(nonnull Protocol *)servicePrt{
    if (servicePrt == @protocol(ModelUserInfoServicePrt)) {
        return [[self class] sharedConnector];
    }
    return nil;
}

#pragma mark - model UserInfo提供的服务方法

-(void)modelUserInfo_deliveAprotocol:(nonnull id<ModelUserInfoPrt>)item{
    NSString *showText = [item description];
    
    if ( [self.currentController class] == [FirstViewController class] ) {
        FirstViewController *controller = (FirstViewController*)self.currentController;
        if( [item isKindOfClass:[UserInfo class]] ){
            controller.userInfo = item;
        }else{
            controller.userInfo.name = item.name;
            controller.userInfo.school = item.school;
            controller.userInfo.token = item.token;
        }
    }
    
    UIAlertAction *cancelAlertAction = [UIAlertAction actionWithTitle:@"cancel" style:UIAlertActionStyleCancel handler:nil];
    UIAlertAction *confirmAlertAction = [UIAlertAction actionWithTitle:@"confirm" style:UIAlertActionStyleDefault handler:nil];
    
    UIAlertController *alertController = [UIAlertController alertControllerWithTitle:@"Module A展示外部传入" message:showText preferredStyle:UIAlertControllerStyleAlert];
    [alertController addAction:cancelAlertAction];
    [alertController addAction:confirmAlertAction];
    [[UIApplication sharedApplication].keyWindow.rootViewController presentViewController:alertController animated:YES completion:nil];
}

-(nonnull id<ModelUserInfoPrt>)model_getItemUserInfo:(nullable NSString*)tname School:(nullable NSString*)tschool Token:(nonnull NSString*)ttoken{
    UserInfo *info = [[UserInfo alloc] initWIthName:tname School:tschool token:ttoken];
    
    return info;
}


-(void)moduleA_showAlertWithMessage:(nonnull NSString *)message
                       cancelAction:(void(^__nullable)(NSDictionary *__nonnull info))cancelAction
                      confirmAction:(void(^__nullable)(NSDictionary *__nonnull info))confirmAction{
    UIAlertAction *cancelAlertAction = [UIAlertAction actionWithTitle:@"cancel" style:UIAlertActionStyleCancel handler:^(UIAlertAction * _Nonnull action) {
        if (cancelAction) {
            cancelAction(@{@"alertAction":action});
        }
    }];
    
    UIAlertAction *confirmAlertAction = [UIAlertAction actionWithTitle:@"confirm" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
        if (confirmAction) {
            confirmAction(@{@"alertAction":action});
        }
    }];
    
    UIAlertController *alertController = [UIAlertController alertControllerWithTitle:@"alert from Module A" message:message preferredStyle:UIAlertControllerStyleAlert];
    [alertController addAction:cancelAlertAction];
    [alertController addAction:confirmAlertAction];
    [[UIApplication sharedApplication].keyWindow.rootViewController presentViewController:alertController animated:YES completion:nil];
}

@end
