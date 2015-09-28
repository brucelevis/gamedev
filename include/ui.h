#ifndef UI_H
#define UI_H

#include <common.h>
#include <cstdarg> // For putText()

namespace ui {	// Functions are kept in a namespace simply
				// for organization

	extern vec2 mouse;

	extern bool debug;
	extern unsigned int fontSize;

	void initFonts(void);	// Checks for and initializes the FreeType 2 library
	
	void setFontFace(const char *ttf);		// Checks and unpacks the TTF file for use by putString() and putText()
	void setFontSize(unsigned int size);	// Sets the size of the currently loaded font to 'size' pixels
	
	void putString(const float x,const float y,const char *s);		// Draws the string 's' to the coordinates ('x','y'). The height (and therefore the width)
																	// are determined by what's currently set by setFontSize()
	void putText(const float x,const float y,const char *str,...);	// Draws the formatted string 'str' using putString()
	
	void dialogBox(const char *text);								// Prepares a dialog box to be drawn (its drawn as a black background at the top of the
																	// screen and then 'text' is putString()'d
	
	void draw(void);												// Draws things like dialogBox's if necessary
	
	void handleEvents(void);	// Handles keyboard and mouse events
}

#endif // UI_H
