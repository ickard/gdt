/*
 * gdt.h
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

#ifndef gdt_h
#define gdt_h

#include <stdarg.h>
#include <stdint.h>


#ifdef __APPLE__
#define GDT_PLATFORM_IOS
#endif

#ifdef ANDROID
#define GDT_PLATFORM_ANDROID
#endif

#ifndef __cplusplus
typedef int bool;
#define true 1
#define false 0
#endif

typedef const char* string_t;

typedef enum {
  KBD_HIDDEN,
  KBD_PORTRAIT,
  KBD_LANDSCAPE
} keyboard_mode_t;

typedef enum {
  TOUCH_DOWN,
  TOUCH_UP,
  TOUCH_MOVE
} touch_type_t;

typedef enum {
  LOG_DEBUG,
  LOG_NORMAL,
  LOG_WARNING,
  LOG_ERROR
} log_type_t;

typedef enum {
  EXIT_SUCCEED,
  EXIT_FAIL
} exit_type_t;

struct resource;
typedef struct resource* resource_t;

struct audioplayer;
typedef struct audioplayer* audioplayer_t;

typedef void (*touchhandler_t)(touch_type_t, int, int);

#ifdef __cplusplus
extern "C" {
#endif // cplusplus

/* --- gdt_hook_X functions ---
 *  Every "hook" listed here will need to be implemented by the user of the API.
 *
 *  They are not guaranteed to run on the same thread,
 *  however if they are run on different threads then
 *  calls to all these functions will be properly synchronized.
 */
 
/* gdt_hook_initialize -- This is called when the game has just started
 * Example stuff to do here:
 *   - Load resources
 *   - Setup callbacks
 *   - If using threads, consider starting them
 *     in gdt_hook_visible() rather then here
 *   - If you have saved game state, load it.
 */
void gdt_hook_initialize(void);

/* gdt_hook_exit -- This is called when the game is about to exit:
 *   - On Android and on iOS (if the game is a background-capable app):
 *       when the system needs to reclaim memory
 *       and the game is currently in the background
 *   - On iOS (non-background app):
 *       User presses home button
 * Example things to include here:
 *   - Free resources
 *   - If appropriate, save game state
 */
void gdt_hook_exit(void);

/* gdt_hook_visible -- Called when the game has become visible and an OpenGL
 *                     ES surface is ready with the specified dimensions
 * 
 * This is called
 *   - immediately after gdt_hook_initialize()
 *   - when game has been running in background
 *     and has just returned to foreground
 * Example of things to run here:
 *   - If you have extra threads for game logic, enable them here
 *   - Prepare the gl surface
 * Example code:
 *   glViewport(... screenWidth ... screenHeight);
 *   ...
 *   glOrtof(...);
 *   glMatrixMode(...);
 */
void gdt_hook_visible(int32_t surfaceWidth, int32_t surfaceHeight);

/* gdt_hook_hidden -- The game is going to the background or exiting
 * and the OpenGL ES surface is now gone
 *
 * This is called
 *   - immediately before gdt_hook_exit()   --> exiting = true
 *   - when game has been running in the
 *     foreground and is being hidden      --> exiting = false
 * Example of things to run here:
 *   - If you have extra threads for game logic,
 *     make sure to inactive them here
 *   - Add something like a nice "Paused" screen
 *     so the player can touch to resume when the
 *     game is back in foreground again later.
 */
void gdt_hook_hidden(bool exiting);

/* gdt_hook_render -- Called when the game needs to draw a frame
 * This can only get called if an OpenGL surface is currently visible.
 * Example code:
 *   prepare_game_for_draw();
 *   glClearColor(...);
 *   ...
 *   glPushMatrix();
 *   ...
 *   glTranslate3f(...);
 *   glDrawBlabla(..);
 *   ...
 *   glPopMatrix();
 *   ...
 */
void gdt_hook_render(void);

// ----------------------------

/* --- gdt_set_callback_X functions ---
 *  Set callback that will recieve X events when the game is in foreground.
 *  Setting to NULL will unsubscribe the current listener (if any).
 */
 
void gdt_set_callback_touch(touchhandler_t on_touch);

// ------------------------------------


// --- Misc. utility functions ---


/* gdt_gc_hint -- A hint to the underlying OS that now would be a good time to do a garbage collection.
 * On platforms which does not use garbage collection, this does not do anything (no-op)
 */
void gdt_gc_hint(void);

// Return the time in nanoseconds at the highest precision available.
uint64_t gdt_time_ns(void);
void gdt_set_virtual_keyboard_mode(keyboard_mode_t mode);
void gdt_open_url                 (string_t url);
    
void gdt_log                      (log_type_t type,    string_t tag,
								   string_t format, ...);
void gdt_logv                     (log_type_t type,    string_t tag,
								   string_t format, va_list args);

// Log (as LOG_ERROR), and then exit with EXIT_FAIL
void gdt_fatal(string_t tag, string_t format, ...);

/* Immediately exit the program, with the specified error code.
 * Calling this will not invoke gdt_hook_hidden() or gdt_hook_exit(). 
 */
void gdt_exit(exit_type_t type);

// ------------------------------


/* --- Resource management functions ---
 * A resourcePath is a path relative to the root directory
 * of the bundled resources in an application. Example of a path:
 * 
 * "/gfx/test.tga" 
 * On Android, this will refer to "assets/gfx/test.tga"
 * On iOS this will refer to "gfx/test.tga" within the bundled resources
 * of the application.
 */

void*      gdt_resource_bytes (resource_t resource);
int32_t    gdt_resource_length(resource_t resource);
resource_t gdt_resource_load  (string_t   resourcePath);
void       gdt_resource_unload(resource_t resource);

// -------------------------------------


/* --- AudioPlayer functions ---
 * Simple audio playback using the underlying OS.
 * Error handling:
 * - if anything goes wrong in gdt_audiplayer_create(), it returns NULL
 * - errors in gdt_audioplayer_destroy() are ignored
 * - if anything goes wrong in the other functions, they will return false,
 * and that audioplayer_t object should not be used again at all (no need to even destroy it)
 */

audioplayer_t gdt_audioplayer_create(string_t resourcePath);
void gdt_audioplayer_destroy(audioplayer_t player);
bool gdt_audioplayer_play(audioplayer_t player);
/*
bool gdt_audioplayer_change_source(audioplayer_t player, string_t resourcePath);
bool gdt_audioplayer_pause(audioplayer_t player);
bool gdt_audioplayer_set_position(audioplayer_t player, double seconds);
double gdt_audioplayer_get_position(audioplayer_t player);
*/

// -----------------------------

#ifdef __cplusplus
}
#endif // cplusplus


#endif // gdt_h
