//
//  ModelUserInfoServicePrt.h
//  IOSFrame
//
//  Created by 安昌 on 2017/7/26.
//  Copyright © 2017年 安昌. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "ModelUserInfoItemPrt.h"

@protocol ModelUserInfoServicePrt<NSObject>

@required

-(void)modelUserInfo_deliveAprotocol:(nonnull id<ModelUserInfoPrt>)item;

-(nonnull id<ModelUserInfoPrt>)model_getItemUserInfo:(nullable NSString*)tname School:(nullable NSString*)tschool Token:(nonnull NSString*)ttoken;

@optional

-(void)moduleA_showAlertWithMessage:(nonnull NSString *)message
                       cancelAction:(void(^__nullable)(NSDictionary *__nonnull info))cancelAction
                      confirmAction:(void(^__nullable)(NSDictionary *__nonnull info))confirmAction;

@end
