//
//  FirstViewController.m
//  IOSFrame
//
//  Created by 安昌 on 2017/7/26.
//  Copyright © 2017年 安昌. All rights reserved.
//

#import "FirstViewController.h"
#import "LDBusMediator.h"
#import "RouterURLInfo.h"

@interface FirstViewController ()

@property (nonatomic,strong) UIButton *jumpButton;
@property (nonatomic,strong) UILabel *userInfoLabel;

@end

@implementation FirstViewController

@synthesize jumpButton = _jumpButton;
@synthesize userInfo = _userInfo;
@synthesize userInfoLabel = _userInfoLabel;

- (void)viewDidLoad {
    [super viewDidLoad];
    
    self.navigationItem.title = @"first";
    [self.view addSubview:self.jumpButton];
    [self.view addSubview:self.userInfoLabel];
    // Do any additional setup after loading the view.
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark- action
-(void)actionJumpButton{
    UIViewController *controller = [LDBusMediator viewControllerForURL:[NSURL URLWithString:CAT(RouterName, First_Detail_URL)]];
    controller.view.backgroundColor = [UIColor greenColor];
    [self.navigationController pushViewController:controller animated:YES];
}

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

-(void)updateView
{
    self.userInfoLabel.text = [self.userInfo description];
}

-(UILabel*)userInfoLabel{
    if ( _userInfoLabel == nil ) {
        _userInfoLabel = [[UILabel alloc] initWithFrame:CGRectMake(20, 100, 200, 30)];
        _userInfoLabel.backgroundColor = [UIColor blackColor];
        _userInfoLabel.textColor = [UIColor whiteColor];
    }
    
    return _userInfoLabel;
}

-(FirstViewUserInfo*)userInfo{
    if ( _userInfo == nil ) {
        _userInfo = [[FirstViewUserInfo alloc] init];
    }
    
    return _userInfo;
}

-(UIButton*)jumpButton{
    if ( _jumpButton == nil ) {
        _jumpButton = [UIButton buttonWithType:UIButtonTypeCustom];
        [_jumpButton setBackgroundColor:[UIColor blackColor]];
        [_jumpButton setTitle:@"跳转" forState:UIControlStateNormal];
        [_jumpButton setFrame:CGRectMake(50, 150, 50, 50)];
        [_jumpButton addTarget:self action:@selector(actionJumpButton) forControlEvents:UIControlEventTouchUpInside];
    }
    return _jumpButton;
    
}

@end
