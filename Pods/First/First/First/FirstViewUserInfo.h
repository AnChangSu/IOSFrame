//
//  FirstViewUserInfo.h
//  IOSFrame
//
//  Created by 安昌 on 2017/8/14.
//  Copyright © 2017年 安昌. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "ModelUserInfoItemPrt.h"

@interface FirstViewUserInfo : NSObject<ModelUserInfoPrt>

@property(nonatomic,strong,nullable) NSString *name;
@property(nonatomic,strong,nullable) NSString *school;
@property(nonatomic,copy,nonnull)   NSString *token;

-(nonnull instancetype)initWIthName:(nullable NSString*)tname School:(nullable NSString*)tschool token:(nonnull NSString *)ttoken;

@end
