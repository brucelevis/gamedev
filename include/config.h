#ifndef CONFIG_H
#define CONFIG_H

#include <iostream>
#include <SDL2/SDL_mixer.h>
#include <tinyxml2.h>

void readConfig(void);

void updateConfig(void);

void saveConfig();

#endif //CONFIG_H