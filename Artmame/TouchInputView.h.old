//
//  TouchInputView.h
//  Artsnes9x
//
//  Created by arthur on 28/01/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface TouchInputView : UIView <UIAlertViewDelegate>
{
    int touch_buttons;
    int gesture_buttons;
    int gesture_length;
    UITapGestureRecognizer *grTap;
    UISwipeGestureRecognizer *grSwipeLeft;
    UISwipeGestureRecognizer *grSwipeRight;
}
@property (nonatomic, readonly) int gestureButtons;
@property (nonatomic, readonly) int joypadButtons;
@property (nonatomic, retain) UIView *helpView;
@end
