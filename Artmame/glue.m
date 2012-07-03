//
//  glue.cpp
//  Artnestopia
//
//  Created by arthur on 20/01/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#import <AudioToolbox/AudioQueue.h>
#import <AudioToolbox/AudioToolbox.h>

#import <UIKit/UIKit.h>
#import "ScreenView.h"
#import "TouchInputView.h"
#import "Helper.h"
#import "RootViewController.h"
#import "glue.h"

#include "osd_cpu.h"
#include "mamecore.h"
#include "mame.h"
#include "osdepend.h"
#include "osdcore.h"
#include "render.h"
#include "rendlay.h"

#define FUNC_PREFIX(x)          screenview_rgb555_##x
#define PIXEL_TYPE                      UINT16
#define SRCSHIFT_R                      3
#define SRCSHIFT_G                      3
#define SRCSHIFT_B                      3
#define DSTSHIFT_R                      10
#define DSTSHIFT_G                      5
#define DSTSHIFT_B                      0

#include "rendersw.c"

int load_game(int game);
void unload_game();

void audio_open(void);
void audio_close(void);

int inputcode[IOS_INPUTCODE_MAX];

render_target *screen_target = NULL;
int got_next_frame = 0;
INT16 *samplebuf = NULL;
extern running_machine *Machine;
int loaded = 0;

#define AUDIO_BUFFERS 3

typedef struct AQCallbackStruct {
    AudioQueueRef queue;
    UInt32 frameCount;
    AudioQueueBufferRef mBuffers[AUDIO_BUFFERS];
    AudioStreamBasicDescription mDataFormat;
} AQCallbackStruct;

AQCallbackStruct in;

int audio_is_initialized = 0;
int audio_do_not_initialize = 0;

void blit_to_screenview()
{
    extern RootViewController *rootViewController;
    extern ScreenView *screenView;
    if (!screen_target)
        return;
    if (!rootViewController)
        return;
    if (!screenView)
        return;
    extern unsigned short imgbuffer[];
    extern int screen_width, screen_height;
    UINT32 minwidth, minheight;
    render_target_get_minimum_size(screen_target, &minwidth, &minheight);
//    NSLog(@"blit_to_screenview %d %d", minwidth, minheight);
    if ((screen_width != minwidth) || (screen_height != minheight)) {
        NSLog(@"changing screen size from %d %d to %d %d", screen_width, screen_height, minwidth, minheight);
        [rootViewController changeScreenViewToSize:CGSizeMake(minwidth, minheight)];
        NSLog(@"changed screen size %d %d", screen_width, screen_height);
    }
    render_target_set_bounds(screen_target, minwidth, minheight, 0.0);
    const render_primitive_list *primlist = render_target_get_primitives(screen_target);
    int w = screen_width;
    int h = screen_height;
    screenview_rgb555_draw_primitives(primlist->head, (void *)imgbuffer, w, h, w);
    extern CGRect get_screen_rect(UIInterfaceOrientation toInterfaceOrientation);
    [screenView.layer setNeedsDisplay];
}

static void audio_queue_callback(void *userdata,
                                 AudioQueueRef outQ,
                                 AudioQueueBufferRef outQB)
{
    outQB->mAudioDataByteSize = in.mDataFormat.mBytesPerFrame * in.frameCount;
    if (loaded) {
/*        static unsigned char pixels[MAX_SCREEN_WIDTH*MAX_SCREEN_HEIGHT*2];*/
        /* data = outQB->mAudioData; */
/*        extern TouchInputView *inputView;*/
        /* buttons = inputView.joypadButtons; */
        /* execute */
        while (!got_next_frame) {
            cpuexec_timeslice();
        }
        got_next_frame = 0;
        if (samplebuf) {
            memcpy(outQB->mAudioData, samplebuf, 735*4);
            samplebuf = NULL;
        } else {
            memset(outQB->mAudioData, 0, outQB->mAudioDataByteSize);
        }
    } else {
        memset(outQB->mAudioData, 0, outQB->mAudioDataByteSize);
    }
    AudioQueueEnqueueBuffer(outQ, outQB, 0, NULL);
}


void audio_close()
{
    if(audio_is_initialized) {
        AudioQueueDispose(in.queue, true);
        audio_is_initialized = 0;
    }
}


void audio_open()
{
    if (audio_do_not_initialize)
        return;
    
    if (audio_is_initialized)
        return;
    
    memset (&in.mDataFormat, 0, sizeof (in.mDataFormat));
    in.mDataFormat.mSampleRate = 44100.0;
    in.mDataFormat.mFormatID = kAudioFormatLinearPCM;
    in.mDataFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger|kAudioFormatFlagIsPacked;
    in.mDataFormat.mBytesPerPacket = 4;
    in.mDataFormat.mFramesPerPacket = 1;
    in.mDataFormat.mBytesPerFrame = 4;
    in.mDataFormat.mChannelsPerFrame = 2;
    in.mDataFormat.mBitsPerChannel = 16;
    in.frameCount = 735.0; // 44100.0 / 60.0;
    UInt32 err;
    err = AudioQueueNewOutput(&in.mDataFormat,
                              audio_queue_callback,
                              NULL,
                              CFRunLoopGetMain(),
                              kCFRunLoopDefaultMode,
                              0,
                              &in.queue);
    
    unsigned long bufsize;
    bufsize = in.frameCount * in.mDataFormat.mBytesPerFrame;
    
    for (int i=0; i<AUDIO_BUFFERS; i++) {
        err = AudioQueueAllocateBuffer(in.queue, bufsize, &in.mBuffers[i]);
        in.mBuffers[i]->mAudioDataByteSize = bufsize;
        AudioQueueEnqueueBuffer(in.queue, in.mBuffers[i], 0, NULL);
    }
    
    audio_is_initialized = 1;
    err = AudioQueueStart(in.queue, NULL);
}

int load_game_wrapper(const char *gamename)
{
    memset(&options, 0, sizeof(options));
    options.brightness = 1.0f;
    options.contrast = 1.0f;
    options.gamma = 1.0f;
    options.samplerate = 44100;
    options.use_samples = 1;
    options.skip_disclaimer = options.skip_warnings = options.skip_gameinfo = TRUE;
    options.beam = 0;
    options.vector_flicker = 0;
    options.antialias = 1;
    for(int i=0; i<IOS_INPUTCODE_MAX; i++) {
        inputcode[i] = 0;
    }
    int index = driver_get_index(gamename);
    if (index != -1) {
        return load_game(index);
    }
    return MAMERR_FATALERROR;
}

NSDictionary *dictNotifierTitle;
NSMutableDictionary *dictNotifierState;

NSString *notifierStateToString(void);
NSString *notifierStateToString()
{
    return [[dictNotifierState allValues] componentsJoinedByString:@" "];
}

void notifierSetState(const char *outname, INT32 value)
{
    NSString *key = [NSString stringWithFormat:@"%s", outname];
    NSString *val = [dictNotifierTitle objectForKey:key];
    if (!val)
        val = key;
    if ([key hasSuffix:@":"]) {
        [dictNotifierState setValue:[NSString stringWithFormat:@"%@ %s", key, (char *)value] forKey:key];
    } else {
        if (value) {
            [dictNotifierState setValue:[NSString stringWithFormat:@"[%@]", val] forKey:key];
        } else {
            [dictNotifierState setValue:[NSString stringWithFormat:@" %@ ", val] forKey:key];
        }
    }
}

int osd_init(running_machine *machine)
{
    memset(inputcode, 0, sizeof(int)*IOS_INPUTCODE_MAX);
    screen_target = render_target_alloc(NULL, 0);
    render_target_set_layer_config(screen_target, 0);
    return 0;
}

void osd_customize_inputport_list(input_port_default_entry *defaults)
{
}

int osd_update_audio_stream(INT16 *buffer)
{
    samplebuf = buffer;
    return 735;
}

void osd_joystick_calibrate(void)
{
}

void osd_sound_enable(int enable)
{
}

const os_code_info *osd_get_code_list(void)
{
    static os_code_info codes[] = {
        { "COIN1", IOS_KEYCODE_COIN1, KEYCODE_1 },
        { "COIN2", IOS_KEYCODE_COIN2, KEYCODE_2 },
        { "COIN3", IOS_KEYCODE_COIN3, KEYCODE_3 },
        { "COIN4", IOS_KEYCODE_COIN4, KEYCODE_4 },
        { "START1", IOS_KEYCODE_START1, KEYCODE_5 },
        { "START2", IOS_KEYCODE_START2, KEYCODE_6 },
        { "START3", IOS_KEYCODE_START3, KEYCODE_7 },
        { "START4", IOS_KEYCODE_START4, KEYCODE_8 },
        { "1_UP", IOS_JOYCODE_1_UP, JOYCODE_1_UP },
        { "1_DOWN", IOS_JOYCODE_1_DOWN, JOYCODE_1_DOWN },
        { "1_LEFT", IOS_JOYCODE_1_LEFT, JOYCODE_1_LEFT },
        { "1_RIGHT", IOS_JOYCODE_1_RIGHT, JOYCODE_1_RIGHT },
        { "1_BUTTON1", IOS_JOYCODE_1_BUTTON1, JOYCODE_1_BUTTON1 },
        { "1_BUTTON2", IOS_JOYCODE_1_BUTTON2, JOYCODE_1_BUTTON2 },
        { "1_BUTTON3", IOS_JOYCODE_1_BUTTON3, JOYCODE_1_BUTTON3 },
        { "1_BUTTON4", IOS_JOYCODE_1_BUTTON4, JOYCODE_1_BUTTON4 },
        { "1_BUTTON5", IOS_JOYCODE_1_BUTTON5, JOYCODE_1_BUTTON5 },
        { "1_BUTTON6", IOS_JOYCODE_1_BUTTON6, JOYCODE_1_BUTTON6 },
        { 0, 0, 0 }
    };
    return codes;
}

int osd_get_mastervolume(void)
{
    return 0;
}

int osd_start_audio_stream(int stereo)
{
    return 735;
}

int osd_joystick_needs_calibration(void)
{
    return 0;
}

int osd_is_absolute_path(const char *path)
{
    return 1;
}

void osd_stop_audio_stream(void)
{
}

const char *osd_joystick_calibrate_next(void)
{
    return 0;
}

const char *osd_get_fps_text(const performance_info *performance)
{
    return 0;
}

mame_file_error osd_rmfile(const char *filename)
{
    return FILERR_NONE;
}

INT32 dec_inputcode(os_code oscode)
{
    if (inputcode[oscode] > 0) {
        inputcode[oscode]--;
        return 1;
    }
    return 0;
}

INT32 osd_get_code_value(os_code oscode)
{
    switch (oscode) {
        case IOS_KEYCODE_START1:
        case IOS_KEYCODE_START2:
        case IOS_KEYCODE_COIN1:
        case IOS_KEYCODE_COIN2:
        case IOS_JOYCODE_1_BUTTON1:
        case IOS_JOYCODE_1_BUTTON2:
        case IOS_JOYCODE_1_BUTTON3:
        case IOS_JOYCODE_1_BUTTON4:
        case IOS_JOYCODE_1_BUTTON5:
        case IOS_JOYCODE_1_BUTTON6:
            return dec_inputcode(oscode);
        default:
            return inputcode[oscode];
    }
}

void osd_set_mastervolume(int attenuation)
{
}

void osd_joystick_start_calibration(void)
{
}

int osd_update(mame_time emutime)
{
    blit_to_screenview();
    got_next_frame = 1;
    return 0;
}

int osd_uchar_from_osdchar(UINT32 /* unicode_char */ *uchar, const char *osdchar, size_t count)
{
    NSLog(@"osd_uchar_from_osdchar");
    return 0;
}

void osd_joystick_end_calibration(void)
{
}

mame_file_error osd_open_internal(const char *path, UINT32 openflags, osd_file **file, UINT64 *filesize)
{
	const char *mode;
	FILE *fileptr;
    
	// based on the flags, choose a mode
	if (openflags & OPEN_FLAG_WRITE)
	{
		if (openflags & OPEN_FLAG_READ)
			mode = (openflags & OPEN_FLAG_CREATE) ? "w+b" : "r+b";
		else
			mode = "wb";
	}
	else if (openflags & OPEN_FLAG_READ)
		mode = "rb";
	else
		return FILERR_INVALID_ACCESS;
    
//    NSLog(@"osd_open mode '%s'", mode);
    
	// open the file
	fileptr = fopen(path, mode);
	if (fileptr == NULL)
		return FILERR_NOT_FOUND;
    
	// store the file pointer directly as an osd_file
	*file = (osd_file *)fileptr;
    
	// get the size -- note that most fseek/ftell implementations are limited to 32 bits
	fseek(fileptr, 0, SEEK_END);
	*filesize = ftell(fileptr);
	fseek(fileptr, 0, SEEK_SET);
    
	return FILERR_NONE;
}

mame_file_error osd_open(const char *name, UINT32 openflags, osd_file **file, UINT64 *filesize)
{
    char *path;
    mame_file_error err;
    path = (char *) getCString(getPathInDocs([NSString stringWithCString:name encoding:NSASCIIStringEncoding]));
    err = osd_open_internal(path, openflags, file, filesize);
//    NSLog(@"osd_open pathInDocs %d '%s' '%s'", err, name, path);
    if (err != FILERR_NOT_FOUND) {
        return err;
    }
    path = (char *) getCString(getPathInBundle([NSString stringWithCString:name encoding:NSASCIIStringEncoding]));
    err = osd_open_internal(path, openflags, file, filesize);
//    NSLog(@"osd_open pathInBundle %d '%s' '%s'", err, name, path);
    return err;
}
