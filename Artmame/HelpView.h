//
//  HelpView.h
//  Artsnes9x
//
//  Created by arthur on 4/02/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>

void addHelpBox(UIView *v, CGFloat x, CGFloat y, CGFloat w, CGFloat h, UIColor *color, NSString *text);
UIView *helpForZapper(CGRect frame);
UIView *helpForTouch(CGRect frame);
