//
//  RootViewController.h
//  Artnestopia
//
//  Created by arthur on 20/01/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "ScreenView.h"

@interface RootViewController : UIViewController
- (id)initWithGame:(NSString *)name;
- (void)handleReset;
- (void)changeScreenViewToSize:(CGSize)size;
- (void)setDebugText:(NSString *)text;
@property (nonatomic, retain) NSString *path;
@property (nonatomic, retain) UILabel *cornerLabel;
@property (nonatomic, retain) UILabel *debugLabel;
@end
