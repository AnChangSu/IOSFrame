//
//  FirstDetailViewController.m
//  IOSFrame
//
//  Created by 安昌 on 2017/7/26.
//  Copyright © 2017年 安昌. All rights reserved.
//

#import "FirstDetailViewController.h"
#import "CDVViewController.h"

@interface FirstDetailViewController ()

@end

@implementation FirstDetailViewController

- (void)viewDidLoad {
    
    //需要在super viewDidLoad上面，因为在super的加载方法中，已经使用startPage来判断是否读取本地资源了
//    self.startPage = @"https://www.baidu.com";
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    self.navigationItem.title = @"firstDetail";
    
    self.webView.frame = CGRectMake(0, 0, CGRectGetWidth(self.view.frame), CGRectGetHeight(self.view.frame) );
    
    UIBarButtonItem *rightItem = [[UIBarButtonItem alloc] initWithTitle:@"测试" style:UIBarButtonItemStylePlain target:self action:@selector(testClick)];
    self.navigationItem.rightBarButtonItem = rightItem;
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)testClick
{
    NSString *jsStr = @"asyncAlert('哈哈啊哈')";
    [self.commandDelegate evalJs:jsStr];
}

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

@end
