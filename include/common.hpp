/** @file common.h
 * @brief Common items needed by most other files.
 *
 * This file contains headers, variables and functions that are needed in
 * most other files included in this project.
 */

#ifndef COMMON_H
#define COMMON_H

// holy moly
#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <string>
#include <fstream>
#include <thread>
#include <mutex>
#include <future>
#include <math.h>
#include <threadpool.hpp>
#include <algorithm>

#define GLEW_STATIC
#include <GL/glew.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

#ifdef __WIN32__
typedef unsigned int uint;
#undef near
#endif

/**
 * Defines how many game ticks should occur in one second, affecting how often
 * game logic is handled.
 */

#define TICKS_PER_SEC 20

/**
 * Defines how many milliseconds each game tick will take.
 */

#define MSEC_PER_TICK (1000 / TICKS_PER_SEC)

/**
 * This flag lets the compuler know that we are testing for segfault locations.
 * If this flag is enabled, the function C(x) will print 'x' to terminal
 */

//#define SEGFAULT

/**
 * This flag lets the compiler know that we want to use shaders.
 */

#define SHADERS

template<typename N>
N abso(N v) {
	if (v < 0) {
		return v * -1;
	}else
		return v;
}

template<class A>
float averagef(A v) {
	float avg = 0;
	for(auto &a : v) {
		avg += a;
	}
	avg /= v.size();
	return avg;
}


extern GLuint colorIndex;	// Texture.cpp?

/**
 * This structure contains a set of coordinates for ease of coding.
 */

typedef struct {
	int x;
	int y;
} ivec2;

struct _vec2 {
	float x;
	float y;

	bool operator==(const _vec2 &v) {
		return (x == v.x) && (y == v.y);
	}
	template<typename T>
	const _vec2 operator=(const T &n) {
		x = y = n;
		return *this;
	}
	template<typename T>
	const _vec2 operator+(const T &n) {
		return _vec2 {x + n, y + n};
	}
};
typedef struct _vec2 vec2;

typedef struct {
	float x;
	float y;
	float z;
} vec3;

typedef ivec2 dim2;

/**
 * This structure contains two sets of coordinates for ray drawing.
 */

typedef struct {
	vec2 start;
	vec2 end;
} Ray;

struct col {
	float red;
	float green;
	float blue;
	col operator-=(float a) {
		red-=a;
		green-=a;
		blue-=a;
		return{red+a,green+a,blue+a};
	}
	col operator+=(float a) {
		return{red+a,green+a,blue+a};
	}
	col operator=(float a) {
		return{red=a,green=a,blue=a};
	}
};

typedef col Color;

/**
 * Define the game's name (displayed in the window title).
 */

#define GAME_NAME		"Independent Study v0.7 alpha - NOW WITH lights and snow and stuff"

/**
 * The desired width of the game window.
 */

extern unsigned int SCREEN_WIDTH;

/**
 * The desired height of the game window.
 */

extern unsigned int SCREEN_HEIGHT;

extern bool FULLSCREEN;
extern bool uiLoop;
extern std::mutex mtx;

/**
 * Define the length of a single HLINE.
 * The game has a great amount of elements that need to be drawn or detected, and having each
 * of them use specific hard-coded numbers would be painful to debug. As a solution, this
 * definition was made. Every item being drawn to the screen and most object detection/physic
 * handling is done based off of this number. Increasing it will give the game a zoomed-in
 * feel, while decreasing it will do the opposite.
 *
 */

#define HLINES(n) (HLINE * n)

extern unsigned int HLINE;

extern float VOLUME_MASTER;
extern float VOLUME_MUSIC;
extern float VOLUME_SFX;
/**
 * A 'wrapper' for libc's srand(), as we hope to eventually have our own random number
 * generator.
 */

#define initRand(s) srand(s)



/**
 * A 'wrapper' for libc's rand(), as we hope to eventually have our own random number
 * generator.
 */

#define getRand() rand()

#define randGet     rand
#define randInit    srand

/**
 * Included in common.h is a prototype for DEBUG_prints, which writes a formatted
 * string to the console containing the callee's file and line number. This macro simplifies
 * it to a simple printf call.
 *
 * DEBUG must be defined for this macro to function.
 */

#define DEBUG_printf(message, ...) DEBUG_prints(__FILE__, __LINE__, message, __VA_ARGS__)

#ifdef SEGFAULT
#define C(x) std::cout << x << std::endl
#else
#define C(x)
#endif

/**
 * Defines pi for calculations that need it.
 */

#define PI 3.1415926535


// references the variable in main.cpp, used for drawing with the player
extern vec2 offset;

// counts the number of times logic() (see main.cpp) has been called, for animating sprites
extern unsigned int loops;

extern GLuint shaderProgram;

/**
 *	Prints a formatted debug message to the console, along with the callee's file and line
 *	number.
 */

void DEBUG_prints(const char* file, int line, const char *s,...);

/**
 * Sets color using glColor3ub(), but handles potential overflow.
 */

void safeSetColor(int r,int g,int b);

/**
 * Sets color using glColor4ub(), but handles potential overflow.
 */

void safeSetColorA(int r,int g,int b,int a);


/**
 * We've encountered many problems when attempting to create delays for triggering
 * the logic function. As a result, we decided on using the timing libraries given
 * by <chrono> in the standard C++ library. This function simply returns the amount
 * of milliseconds that have passed since the epoch.
 */

#ifdef __WIN32__
#define millis()	SDL_GetTicks()
#else
unsigned int millis(void);
#endif // __WIN32__

int getdir(std::string dir, std::vector<std::string> &files);
void strVectorSortAlpha(std::vector<std::string> *v);

const char *readFile(const char *path);

int strCreateFunc(const char *equ);

template<typename N, size_t s>
size_t arrAmt(N (&)[s]) {return s;}

void UserError(std::string reason);

#endif // COMMON_H
