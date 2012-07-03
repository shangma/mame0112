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
    NSMutableDictionary *currentTouches;
}
@property (nonatomic, retain) UIView *helpView;
@end
