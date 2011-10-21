/*
 * GdtView.m
 *
 * Copyright (c) 2011 Rickard Edström
 * Copyright (c) 2011 Sebastian Ärleryd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#import "GdtView.h"
#include "gdt.h"
#include "gdt_ios.h"
#import <OpenGLES/EAGLDrawable.h> 
#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>

touchhandler_t touch_cb = NULL;
int __h;
const char* pathPrefix;
log_type_t t = LOG_DEBUG;

@implementation GdtView



void gdt_ios_set_log_threshold(log_type_t treshold) {
    t = treshold;
}

static NSString* logTypeToFormatString(log_type_t type) {
    switch(type) {
        case LOG_ERROR:
            return @"%s: [error] %s";
        case LOG_WARNING:
            return @"%s: [warning] %s";
        case LOG_DEBUG:
            return @"%s: [debug] %s";
    } 
}

void gdt_logv(log_type_t type, const char* tag, const char* format, va_list args) {
    if (type >= t) {
        NSString* s = [NSString stringWithFormat:
                       logTypeToFormatString(type), tag, format];
        
        NSLogv(s, args);     
    }
}



void gdt_exit(exit_type_t type) {
    [NSThread exit];
}


void gdt_open_url(const char* url) {
    NSString* s   = [NSString stringWithUTF8String: url];
    NSURL*    u   = [NSURL URLWithString: s];
    
    [[UIApplication sharedApplication] openURL: u];
}



void gdt_set_callback_touch(touchhandler_t f) {
        touch_cb = f;
}
void gdt_set_virtual_keyboard_mode(keyboard_mode_t mode) {
    
}

struct resource {
  long len;
  void* data;
};

void* gdt_resource_bytes(resource_t res) {
	return res->data;
}

long gdt_resource_length(resource_t res) {
	return res->len;
}


resource_t gdt_resource_load(const char* resourcePath) {
    char* s;
    asprintf(&s, "%s%s", pathPrefix, resourcePath);
    
    int fd = open(s, O_RDONLY);
    free(s);
    if (fd == -1) return null;
    resource_t res = (resource_t)malloc(sizeof(struct resource));
    struct stat info;
    fstat(fd, &info);
    res->len = info.st_size;
    res->data = mmap(NULL, res->len, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    return res;
}

void gdt_resource_unload(resource_t resource) {    
    munmap(gdt_resource_bytes(resource), gdt_resource_length(resource));
    
    free(resource);
}

void gdt_gc_hint(void) {
}


-(id)initWithFrame:(CGRect)frame
{
    pathPrefix = [[[NSBundle mainBundle] resourcePath] cStringUsingEncoding:NSASCIIStringEncoding];
    gdt_log(LOG_WARNING, "pathprefix", pathPrefix);
    self = [super initWithFrame:frame];
    
    if (self) {
        CAEAGLLayer* layer = (CAEAGLLayer*)super.layer;
        layer.opaque = YES;
        
        ctx = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
        [EAGLContext setCurrentContext:ctx];
        
        GLuint fb;
        glGenFramebuffersOES(1, &fb);
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, fb);
        
        GLuint rb;
        glGenRenderbuffersOES(1, &rb);
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, rb);
        
        glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES,
                                     GL_COLOR_ATTACHMENT0_OES,
                                     GL_RENDERBUFFER_OES,
                                     rb);
        
        [ctx renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:layer];
        
        gdt_hook_initialize();
        gdt_hook_visible(CGRectGetWidth(frame), __h = CGRectGetHeight(frame));
        
        CADisplayLink* link = [CADisplayLink displayLinkWithTarget:self
                                             selector:@selector(drawView:)];
        [link addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    }
    
    return self;
}

+(Class)layerClass
{
    return [CAEAGLLayer class];
}


-(void)drawView:(CADisplayLink*)_
{
    gdt_hook_render();
    
    [ctx presentRenderbuffer:GL_RENDERBUFFER_OES];
}

-(void)handleTouches:(NSSet*)touches withType:(touch_type_t)type 
{
    if (touch_cb) {
        CGPoint where = [[touches anyObject] locationInView:self];
        touch_cb(type, where.x, __h-where.y);    
    }
}


-(void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)_
{
    [self handleTouches:touches withType:TOUCH_DOWN];
}

-(void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*)_
{
    [self handleTouches:touches withType:TOUCH_MOVE];
}

-(void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)_
{
    [self handleTouches:touches withType:TOUCH_UP];
}


-(void)dealloc
{
    [EAGLContext setCurrentContext:nil];
    [ctx release];
    [super dealloc];
}

@end
