//
//  RouterURLInfo.h
//  IOSFrame
//
//  Created by 安昌 on 2017/7/26.
//  Copyright © 2017年 安昌. All rights reserved.
//

#ifndef RouterURLInfo_h
#define RouterURLInfo_h

#import <Foundation/Foundation.h>

static NSString *RouterName = @"IOSFrame";
static NSString *First_URL = @"First";
static NSString *Second_RUL = @"Second";
static NSString *Third_URL = @"Third";
static NSString *Fourth_URL = @"Fourth";

static NSString *First_Detail_URL = @"First_Detail";

#define CAT(str1,str2) [NSMutableString stringWithFormat:@"%@://%@",str1,str2]


#endif /* RouterURLInfo_h */
