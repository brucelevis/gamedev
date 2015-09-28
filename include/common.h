#ifndef COMMON_H
#define COMMON_H

///THIS FILE IS USED FOR VARIABLES THAT WILL BE ACCESED BY MULTIPLE CLASSES/FILES

#include <iostream>
#include <vector>
#include <cstdlib>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_opengl.h>

typedef struct { float x; float y; } vec2; 
enum _TYPE {STRUCTURET = -1, PLAYERT = 0, NPCT = 1}; //these are the main types of entities

#include <entities.h>

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720
//#define FULLSCREEN

#define HLINE 3										//base unit of the world

#define initRand(s) srand(s)
#define getRand()	rand()

template<typename T, size_t N>						//this fuction returns the size of any array
int eAmt(T (&)[N]){return N;}

//SDL VARIABLES
extern SDL_Window    *window;
extern SDL_Surface   *renderSurface;
extern SDL_GLContext  mainGLContext;

extern bool gameRunning;

#endif // COMMON_H
