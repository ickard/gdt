/*
 * simple_example.c
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

#include "gdt/gdt.h"
#include "gdt/gdt_gles2.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>

string_t simpleVertexShader = "							   \
uniform   vec2 offset;										\
attribute vec4 position;									  \
void main(void) {											 \
	gl_Position = position + vec4(offset.x, offset.y, 0, 0);  \
}															 \
";

string_t redFragmentShader = "		 \
void main(void) {					  \
	gl_FragColor = vec4(1, 0, 0, 1);   \
}									  \
";

typedef enum {
	STATE_NOT_INITIALIZED,
	STATE_INITIALIZED_NOT_VISIBLE,
	STATE_INITIALIZED_VISIBLE_NOT_ACTIVE,
	STATE_INITIALIZED_VISIBLE_ACTIVE,
} state_t;

state_t _state = STATE_NOT_INITIALIZED;
bool _kbdVisible = false;
string_t TAG = "simple_example";
float _x = -0.5;
float _y = 0.5;
int _width;
int _height;
GLuint _offsetUniform;
string_t SAVE_FILE = "state";
string_t _save_path;

#define LOG(args...) gdt_log(LOG_NORMAL, TAG, args)
#define SIZE 0.3
#define ASSERT(COND) if (!(COND)) gdt_fatal(TAG, "Assertion failed in %s (%s)", __PRETTY_FUNCTION__, #COND)



static GLuint compileShader(string_t shaderCode, GLenum type) {
	GLuint shader = glCreateShader(type);

	int len = strlen(shaderCode);
	glShaderSource(shader, 1, &shaderCode, &len);

	glCompileShader(shader);

	GLint result;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	if (result == GL_FALSE) {
		gdt_fatal(TAG, "Error compiling shader");
	}

	return shader;
}
static GLuint linkProgram() {
	GLuint vertexShader = compileShader(simpleVertexShader, GL_VERTEX_SHADER);
	GLuint fragmentShader = compileShader(redFragmentShader, GL_FRAGMENT_SHADER);

	GLuint program = glCreateProgram();

	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	glLinkProgram(program);

	GLint result;
	glGetProgramiv(program, GL_LINK_STATUS, &result);
	if (result == GL_FALSE) {
		gdt_fatal(TAG, "Error linking program");
	}

	return program;
}



static bool inside_the_square(float x, float y) {
	return (x > _x && x < (_x + SIZE)) && (y > _y && y < (_y + SIZE));
}
static void move(float x, float y) {
	_x = x - SIZE / 2;
	_y = y - SIZE / 2;
}
static void toggle_kbd() {
	gdt_set_virtual_keyboard_mode(_kbdVisible? KBD_HIDDEN: KBD_VISIBLE);
	_kbdVisible = !_kbdVisible;
}



static void load_state() {
	LOG("checking for saved state to load");

	FILE* file = fopen(_save_path, "r");
	if (file == NULL) {
		LOG("found no state to load");
	} else {
		float xy[2];
		int count = fread(xy, sizeof(xy), 1, file);
		if (count == 1) {
			_x = xy[0]; _y = xy[1];
			LOG("success loading state (_x=%1.3f, _y=%1.3f)", _x, _y);
		} else LOG("failed to read state");
		fclose(file);
	}
}
static bool save_state() {
	FILE* file = fopen(_save_path, "w");
	if (file == NULL)
		return false;

	float xy[2];
	xy[0] = _x; xy[1] = _y;
	int count = fwrite(xy, sizeof(xy), 1, file);
	fclose(file);
	return count == 1;
}



static void on_touch(touch_type_t what, int screenX, int screenY) {
	static int state = 0;

	float x = 2 * screenX / (float) _width  - 1;
	float y = 2 * screenY / (float) _height - 1;

	if (state) {
		switch (what) {
		case TOUCH_MOVE:
			move(x, y);
			break;
		case TOUCH_UP:
			state = 0;
			break;
		default: {}
		}
	} else {
		switch (what) {
		case TOUCH_DOWN:
			if (inside_the_square(x, y)) {
				state = 1;
				move(x, y);
			} else {
				toggle_kbd();
			}
			break;
		default: {}
		}
	}
}
static void on_accelerometer_event(accelerometer_data_t* a) {
	LOG("accelerometer: x=%1.2f y=%1.2f z=%1.2f time=%1.3f", a->x, a->y, a->z, a->time);
}
static void on_text_input(string_t input) {
	LOG("(on_text_input) %s", input == gdt_backspace()? "<backspace>": input);
}



void gdt_hook_initialize() {
	ASSERT(_state == STATE_NOT_INITIALIZED);
	_state = STATE_INITIALIZED_NOT_VISIBLE;

	LOG("initialize");	

	char* output;
	asprintf(&output, "%s/%s", gdt_get_storage_directory_path(), SAVE_FILE);
	_save_path = output;
	
	//LOG(_save_path);
	load_state();
	
	gdt_set_callback_touch(&on_touch);
	gdt_set_callback_text(&on_text_input);
	//gdt_set_callback_accelerometer(&on_accelerometer_event);
}
void gdt_hook_visible(bool newContext, int32_t surfaceWidth, int32_t surfaceHeight) {
	ASSERT(_state == STATE_INITIALIZED_NOT_VISIBLE);
	_state = STATE_INITIALIZED_VISIBLE_NOT_ACTIVE;

	LOG("visible, newContext=%s, screen w=%d h=%d", newContext? "true": "false", surfaceWidth, surfaceHeight);

	if (newContext) {
		GLuint program = linkProgram();

		_offsetUniform = glGetUniformLocation(program, "offset");
		GLuint positionAttrib = glGetAttribLocation(program, "position");

		static const GLfloat v[] = { 0, SIZE,
									 0, 0,
									 SIZE, SIZE,
									 SIZE, 0	};
		GLuint vertexBuf;
		glGenBuffers(1, &vertexBuf);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuf);
		glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);

		static const GLubyte i[] = { 0, 1, 2, 3 };
		GLuint indexBuf;
		glGenBuffers(1, &indexBuf);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(i), i, GL_STATIC_DRAW);

		glEnableVertexAttribArray(positionAttrib);
		glVertexAttribPointer(positionAttrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);

		glUseProgram(program);

		glClearColor(0.4, 0.8, 0.4, 1);
	}

	_width = surfaceWidth;
	_height = surfaceHeight;
	glViewport(0, 0, _width, _height);
	
}
void gdt_hook_active() {
	ASSERT(_state == STATE_INITIALIZED_VISIBLE_NOT_ACTIVE);
	_state = STATE_INITIALIZED_VISIBLE_ACTIVE;

	LOG("active");
}
void gdt_hook_inactive() {
	ASSERT(_state == STATE_INITIALIZED_VISIBLE_ACTIVE);
	_state = STATE_INITIALIZED_VISIBLE_NOT_ACTIVE;

	LOG("inactive");
}
void gdt_hook_save_state() {
#ifdef GDT_PLATFORM_ANDROID
	ASSERT(_state == STATE_INITIALIZED_VISIBLE_NOT_ACTIVE);
#endif

#ifdef GDT_PLATFORM_IOS
	ASSERT(_state == STATE_INITIALIZED_NOT_VISIBLE);
#endif

LOG("save_state");
	
	LOG(save_state()? "saved state": "failed to save state");
}
void gdt_hook_hidden() {
	ASSERT(_state == STATE_INITIALIZED_VISIBLE_NOT_ACTIVE);
	_state = STATE_INITIALIZED_NOT_VISIBLE;


	LOG("hidden");
}
void gdt_hook_render() {
	ASSERT(_state == STATE_INITIALIZED_VISIBLE_NOT_ACTIVE || _state == STATE_INITIALIZED_VISIBLE_ACTIVE);

	glClear(GL_COLOR_BUFFER_BIT);

	glUniform2f(_offsetUniform, _x, _y);
	glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, NULL); 
}
