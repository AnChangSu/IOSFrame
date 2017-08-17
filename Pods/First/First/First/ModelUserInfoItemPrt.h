//
//  ModelUserInfoPrt.h
//  IOSFrame
//
//  Created by 安昌 on 2017/7/26.
//  Copyright © 2017年 安昌. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol ModelUserInfoPrt <NSObject>

@required
@property(nonatomic,strong,nullable) NSString *name;
@property(nonatomic,strong,nullable) NSString *school;
@property(nonatomic,copy,nonnull)   NSString *token;

-(nonnull NSString*)description;

@optional
-(nonnull instancetype)initWIthName:(nullable NSString*)tname School:(nullable NSString*)tschool token:(nonnull NSString *)ttoken;

@end

