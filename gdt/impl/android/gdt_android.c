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

struct audioplayer {
	jobject player;
};

string_t openUrlSig = "(Ljava/lang/String;)V";
string_t gcCollectSig = "()V";
string_t openAssetSig = "(Ljava/lang/String;)[Ljava/lang/Object;";
string_t cleanAssetSig = "(Ljava/lang/Object;)Z";
string_t playerCreateSig = "(Ljava/lang/String;)Ljava/lang/Object;";
string_t playerDestroySig = "(Landroid/media/MediaPlayer;)V";
string_t playerPlaySig = "(Landroid/media/MediaPlayer;)Z";
string_t setKbdModeSig = "(I)V";
string_t eventSubscribeSig = "(IZ)V";

string_t cacheDir;
string_t storageDir;
touchhandler_t cb_touch = NULL;
jclass cls;
JNIEnv* env;
int _screenHeight;
jmethodID openUrl;
jmethodID gcCollect;
jmethodID loadAsset;
jmethodID cleanAsset;
jmethodID playerCreate;
jmethodID playerDestroy;
jmethodID playerPlay;
jmethodID setKbdMode;
jmethodID eventSubscribe;

static accelerometerhandler_t cb_accelerometer = NULL;

void gdt_set_callback_accelerometer(accelerometerhandler_t on_accelerometer_event) {
  if (on_accelerometer_event && !cb_accelerometer)
    (*env)->CallStaticVoidMethod(env, cls, eventSubscribe, 0, true);
  else if (!on_accelerometer_event && cb_accelerometer)
    (*env)->CallStaticVoidMethod(env, cls, eventSubscribe, 0, false);

  cb_accelerometer = on_accelerometer_event;
}

void Java_gdt_Native_eventAccelerometer(JNIEnv* _, jclass __, jdouble time, jfloat x, jfloat y, jfloat z) {
  static accelerometer_data_t a;
  if (cb_accelerometer) {
    a.x = x;
	a.y = y;
	a.z = z;
	a.time = time;
	cb_accelerometer(&a);
  }
}


void gdt_set_callback_text(texthandler_t on_text_input) {

}
string_t gdt_backspace() {

}

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
  	case LOG_DEBUG:
	  return ANDROID_LOG_DEBUG;
    case LOG_ERROR:
      return ANDROID_LOG_ERROR;
    case LOG_WARNING:
      return ANDROID_LOG_WARN;
    default:
      return ANDROID_LOG_INFO;
  }
}




void Java_gdt_Native_initialize(JNIEnv* e, jclass clazz, jstring cachePath, jstring storagePath) {
  static bool initialized = false;
  if (!initialized) {
    initialized = true;

    cls = clazz;
    env = e;
    openUrl = (*env)->GetStaticMethodID(env, cls, "openUrl", openUrlSig);
    gcCollect = (*env)->GetStaticMethodID(env, cls, "gcCollect", gcCollectSig);
    loadAsset = (*env)->GetStaticMethodID(env, cls, "openAsset", openAssetSig);
    cleanAsset = (*env)->GetStaticMethodID(env, cls, "cleanAsset", cleanAssetSig);
    playerCreate = (*env)->GetStaticMethodID(env, cls, "playerCreate", playerCreateSig);
    playerDestroy = (*env)->GetStaticMethodID(env, cls, "playerDestroy", playerDestroySig);
    playerPlay = (*env)->GetStaticMethodID(env, cls, "playerPlay", playerPlaySig);
    eventSubscribe = (*env)->GetStaticMethodID(env, cls, "eventSubscribe", eventSubscribeSig);
    cacheDir = (*env)->GetStringUTFChars(env, cachePath, NULL);
    storageDir = (*env)->GetStringUTFChars(env, storagePath, NULL);

    setKbdMode = (*env)->GetStaticMethodID(env, cls, "setKbdMode", setKbdModeSig);

    gdt_hook_initialize();
  }
}

void Java_gdt_Native_render(JNIEnv* _, jclass __) {
  gdt_hook_render();
}

void Java_gdt_Native_hidden(JNIEnv* _, jclass __) {
  gdt_hook_hidden();
}

void Java_gdt_Native_active(JNIEnv* _, jclass __) {
  gdt_hook_active();
}

void Java_gdt_Native_inactive(JNIEnv* _, jclass __) {
  gdt_hook_inactive();
  gdt_hook_save_state();
}

void Java_gdt_Native_eventTouch(JNIEnv* _, jclass __, jint what, jfloat x, jfloat y) {
  if (cb_touch) {
    touch_type_t action = mapAction(what);
    if (action != -1)
      cb_touch(action, x, _screenHeight-y);
  }
}

void Java_gdt_Native_visible(JNIEnv* _, jclass __, jboolean newSurface, jint width, jint height) {
  _screenHeight = height;
  gdt_hook_visible(newSurface, width, height);
}

void gdt_set_virtual_keyboard_mode(keyboard_mode_t mode) {
  (*env)->CallStaticVoidMethod(env, cls, setKbdMode, mode);
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

audioplayer_t gdt_audioplayer_create(string_t p) {
	  if (p == NULL || p[0] != '/')
	    return NULL;

	  string_t path = p+1;

	  jobject player = (*env)->NewGlobalRef(env,
	                  (*env)->CallStaticObjectMethod(env, cls, playerCreate,
	                    (*env)->NewStringUTF(env, path)
	                  )
	                );

	  // todo: if player null return null

	  audioplayer_t ap = (audioplayer_t)malloc(sizeof(struct audioplayer));

	  ap->player = player;

	  return ap;
}

void gdt_audioplayer_destroy(audioplayer_t player) {
	(*env)->CallStaticVoidMethod(env, cls, playerDestroy, player->player);
	(*env)->DeleteGlobalRef(env, player->player);
	free(player);
}

bool gdt_audioplayer_play(audioplayer_t player) {
	return (*env)->CallStaticBooleanMethod(env, cls, playerPlay, player->player);
}

string_t gdt_get_storage_directory_path(void) {
	return storageDir;
}

string_t gdt_get_cache_directory_path(void) {
	return cacheDir;
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

