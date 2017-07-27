//
//  UserInfo.m
//  IOSFrame
//
//  Created by 安昌 on 2017/7/26.
//  Copyright © 2017年 安昌. All rights reserved.
//

#import "UserInfo.h"

@implementation UserInfo
@synthesize name;
@synthesize school;
@synthesize token;

-(nonnull instancetype)initWIthName:(NSString *)tname School:(NSString *)tschool token:(NSString *)ttoken
{
    self = [self init];
    if (self) {
        self.name = tname;
        self.school = tschool;
        self.token = token;
    }
    return self;
}

-(nonnull NSString*)description{
    return @"i am user info";
}

@end
