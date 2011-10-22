/*
 * gdt_android.c
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

#include <android/log.h>
#include <jni.h>
#include <stdint.h>
#include <stdlib.h>
#include "gdt_android.h"
#include "sys/time.h"


struct resource {
  void* ptr;
  jobject extra;
  int32_t length;
};


string_t openAssetSig = "(Ljava/lang/String;)[Ljava/lang/Object;";
string_t cleanAssetSig = "(Ljava/lang/Object;)Z";
string_t openUrlSig = "(Ljava/lang/String;)V";
string_t gcCollectSig = "()V";

touchhandler_t cb_touch = NULL;
jclass cls;
JNIEnv* env;
jmethodID loadAsset;
jmethodID openUrl;
jmethodID cleanAsset;
jmethodID gcCollect;
int _screenHeight;

static touch_type_t mapAction (int what) {
  switch (what) {
    case 1:
      return TOUCH_UP;
    case 0:
      return TOUCH_DOWN;
    case 2:
      return TOUCH_MOVE;
    default:
      return -1;
  }
}

static int mapType(log_type_t type) {
  switch (type) {
    case LOG_ERROR:
      return ANDROID_LOG_ERROR;
    case LOG_WARNING:
      return ANDROID_LOG_WARN;
    default:
      return ANDROID_LOG_INFO;
  }
}




void Java_gdt_Native_initialize(JNIEnv* e, jclass clazz) {
  static bool initialized = false;
  if (!initialized) {
    initialized = true;

    cls = clazz;
    env = e;
    loadAsset = (*env)->GetStaticMethodID(env, cls, "openAsset", openAssetSig);
    cleanAsset = (*env)->GetStaticMethodID(env, cls, "cleanAsset", cleanAssetSig);
    openUrl = (*env)->GetStaticMethodID(env, cls, "openUrl", openUrlSig);
    gcCollect = (*env)->GetStaticMethodID(env, cls, "gcCollect", gcCollectSig);

    gdt_hook_initialize();
  }
}

void Java_gdt_Native_render(JNIEnv* _, jclass __) {
  gdt_hook_render();
}

void Java_gdt_Native_hide(JNIEnv* _, jclass __, jboolean exitToo) {
  gdt_hook_hidden(exitToo);
  if (exitToo)
    gdt_hook_exit();
}

void Java_gdt_Native_eventTouch(JNIEnv* _, jclass __, jint what, jfloat x, jfloat y) {
  if (cb_touch) {
    touch_type_t action = mapAction(what);
    if (action != -1)
      cb_touch(action, x, _screenHeight-y);
  }
}

void Java_gdt_Native_eventResize(JNIEnv* _, jclass __, jint width, jint height) {
  _screenHeight = height;
  gdt_hook_visible(width, height);
}

void gdt_set_virtual_keyboard_mode(keyboard_mode_t mode) {

}

void gdt_open_url(string_t url) {
  (*env)->CallStaticObjectMethod(env, cls, openUrl, (*env)->NewStringUTF(env, url) );
}

void* gdt_resource_bytes(resource_t res) {
  return res->ptr;
}

int32_t gdt_resource_length(resource_t res) {
  return res->length;
}

resource_t gdt_resource_load(string_t p) {
  if (p == NULL || p[0] != '/')
    return NULL;

  string_t path = p+1;

  jobject arr = (*env)->NewGlobalRef(env,
                  (*env)->CallStaticObjectMethod(env, cls, loadAsset,
                    (*env)->NewStringUTF(env, path)
                  )
                );

  jobject buffer = (*env)->GetObjectArrayElement(env, arr, 0);

  resource_t res = (resource_t)malloc(sizeof(struct resource));

  res->length = (*env)->GetDirectBufferCapacity(env, buffer);
  res->ptr = (*env)->GetDirectBufferAddress(env, buffer);
  res->extra = arr;

  return res;
}

void gdt_resource_unload(resource_t res) {
  jobject arr = res->extra;

  (*env)->CallStaticObjectMethod(env, cls, cleanAsset, arr);
  (*env)->DeleteGlobalRef(env, arr);
  
  free(res);
}


void gdt_set_callback_touch(touchhandler_t on_touch)
{
  cb_touch = on_touch;
}

void gdt_logv(log_type_t type, string_t tag, string_t format, va_list args) {
    __android_log_vprint(mapType(type), tag, format, args);
}

void gdt_exit(exit_type_t type) {
  exit(type == EXIT_FAIL ? 1 : 0);
  // TODO: broken implementation
  // EXIT_FAIL should_--> "app has encountered error"
}

void gdt_gc_hint(void) {
    (*env)->CallStaticObjectMethod(env, cls, gcCollect);
}

uint64_t gdt_time_ns(void) {
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return (uint64_t) now.tv_sec * 1000000000LL + (uint64_t) now.tv_nsec;
}

