#import "ScreenView.h"

unsigned short imgbuffer[640 * 480 * 4];
int screen_width=0, screen_height=0;

@implementation ScreenLayer

+ (id)defaultActionForKey:(NSString *)key
{
    return nil;
}

- (void)dealloc
{
    if(bitmapContext!=nil)
    {
        CFRelease(bitmapContext);
        bitmapContext=nil;
    }
    [super dealloc];
}

- (id)init
{
	self = [super init];
    if (self) {
        if (!screen_width || !screen_height) {
            screen_width = 224;
            screen_height = 288;
        }
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();    
        bitmapContext = CGBitmapContextCreate(
                                              imgbuffer,
                                              screen_width,//512,//320,
                                              screen_height,//480,//240,
                                              /*8*/5, // bitsPerComponent
                                              2*screen_width,//2*512,///*4*320*/2*320, // bytesPerRow
                                              colorSpace,
                                              kCGImageAlphaNoneSkipFirst  | kCGBitmapByteOrder16Little/*kCGImageAlphaNoneSkipLast */);
        
        CFRelease(colorSpace);
        self.affineTransform = CGAffineTransformIdentity;
        [self setMagnificationFilter:kCAFilterLinear];
        [self setMinificationFilter:kCAFilterLinear];  
    }
	return self;
}

- (void)display
{        
    CGImageRef cgImage = CGBitmapContextCreateImage(bitmapContext);
    self.contents = (id)cgImage;
    CFRelease(cgImage);
}

@end

@implementation ScreenView

+ (Class) layerClass
{
    return [ScreenLayer class];
}

- (id)initWithFrame:(CGRect)frame
{
	if ((self = [super initWithFrame:frame])!=nil) {      
        NSLog(@"ScreenView init");
       self.opaque = YES;
       self.clearsContextBeforeDrawing = NO;
       self.multipleTouchEnabled = NO;
	   self.userInteractionEnabled = NO;
	}
	return self;
}

- (void)drawRect:(CGRect)rect
{
}

@end
