#include <ui_menu.hpp>

extern bool gameRunning;

extern Menu *currentMenu;

static Menu pauseMenu;
static Menu optionsMenu;

void Menu::gotoParent(void)
{
	if (!parent) {
		currentMenu = nullptr;
		game::config::update();
	} else {
		currentMenu = parent;
	}
}

void Menu::gotoChild(void)
{
	currentMenu = child;
}

inline void segFault() {
	(*((int *)NULL))++;
}

namespace ui {
    namespace menu {
        menuItem createButton(vec2 l, dim2 d, Color c, const char* t, menuFunc f) {
            menuItem temp;

            temp.member = 0;
            temp.button.loc = l;
            temp.button.dim = d;
            temp.button.color = c;
            temp.button.text = t;
            temp.button.func = f;

            return temp;
        }

        menuItem createChildButton(vec2 l, dim2 d, Color c, const char* t) {
            menuItem temp;

            temp.member = -1;
            temp.button.loc = l;
            temp.button.dim = d;
            temp.button.color = c;
            temp.button.text = t;
            temp.button.func = NULL;

            return temp;
        }

        menuItem createParentButton(vec2 l, dim2 d, Color c, const char* t) {
            menuItem temp;

            temp.member = -2;
            temp.button.loc = l;
            temp.button.dim = d;
            temp.button.color = c;
            temp.button.text = t;
            temp.button.func = NULL;

            return temp;
        }

        menuItem createSlider(vec2 l, dim2 d, Color c, float min, float max, const char* t, float* v) {
            menuItem temp;

            temp.member = 1;
            temp.slider.loc = l;
            temp.slider.dim = d;
            temp.slider.color = c;
            temp.slider.minValue = min;
            temp.slider.maxValue = max;
            temp.slider.text = t;
            temp.slider.var = v;
            temp.slider.sliderLoc = *v;

            return temp;
        }

		void init(void) {
			pauseMenu.items.push_back(ui::menu::createParentButton({-256/2,0},{256,75},{0.0f,0.0f,0.0f}, "Resume"));
			pauseMenu.items.push_back(ui::menu::createChildButton({-256/2,-100},{256,75},{0.0f,0.0f,0.0f}, "Options"));
			pauseMenu.items.push_back(ui::menu::createButton({-256/2,-200},{256,75},{0.0f,0.0f,0.0f}, "Save and Quit", ui::quitGame));
			pauseMenu.items.push_back(ui::menu::createButton({-256/2,-300},{256,75},{0.0f,0.0f,0.0f}, "Segfault", segFault));
			pauseMenu.child = &optionsMenu;

			optionsMenu.items.push_back(ui::menu::createSlider({0-static_cast<float>(game::SCREEN_WIDTH)/4,0-(512/2)}, {50,512}, {0.0f, 0.0f, 0.0f}, 0, 100, "Master", &game::config::VOLUME_MASTER));
			optionsMenu.items.push_back(ui::menu::createSlider({-200,100}, {512,50}, {0.0f, 0.0f, 0.0f}, 0, 100, "Music", &game::config::VOLUME_MUSIC));
			optionsMenu.items.push_back(ui::menu::createSlider({-200,000}, {512,50}, {0.0f, 0.0f, 0.0f}, 0, 100, "SFX", &game::config::VOLUME_SFX));
			optionsMenu.parent = &pauseMenu;
		}

		void toggle(void) {
			currentMenu = &pauseMenu;
		}

        void draw(void) {
			auto SCREEN_WIDTH = game::SCREEN_WIDTH;
			auto SCREEN_HEIGHT = game::SCREEN_HEIGHT;

            SDL_Event e;

			useShader(&textShader,
					  &textShader_uniform_texture,
					  &textShader_attribute_coord,
					  &textShader_attribute_tex);

            setFontSize(24);
            game::config::update();

            mouse.x = ui::premouse.x+offset.x-(SCREEN_WIDTH/2);
            mouse.y = (offset.y+SCREEN_HEIGHT/2)-ui::premouse.y;

            //custom event polling for menu's so all other events are ignored
            while(SDL_PollEvent(&e)) {
                switch (e.type) {
                case SDL_QUIT:
                    gameRunning = false;
                    return;
                    break;
                case SDL_MOUSEMOTION:
                    premouse.x=e.motion.x;
                    premouse.y=e.motion.y;
                    break;
                case SDL_KEYUP:
                    if (SDL_KEY == SDLK_ESCAPE) {
                        currentMenu->gotoParent();
                        return;
                    }
                    break;
                default:break;
                }
            }

			static GLuint backTex = Texture::genColor(Color(0, 0, 0, 204));
			//static GLuint bsTex =	Texture::genColor(Color(30, 30, 30));
			static GLuint border =	Texture::genColor(Color(245, 245, 245));            

			GLfloat line_tex[] = {0.0, 0.0,
								  1.0, 0.0,
								  1.0, 1.0,
								  0.0, 1.0,
								  0.0, 0.0};

			//draw the dark transparent background
            glColor4f(0.0f, 0.0f, 0.0f, .8f);
			glUseProgram(textShader);
			
			glBindTexture(GL_TEXTURE_2D, backTex);
			drawRect(vec2(offset.x - SCREEN_WIDTH / 2, offset.y - (SCREEN_HEIGHT / 2)), vec2(offset.x + SCREEN_WIDTH / 2, offset.y + (SCREEN_HEIGHT / 2)));

			glUseProgram(0);

            //loop through all elements of the menu
            for (auto &m : currentMenu->items) {
                //if the menu is any type of button
                if (m.member == 0 || m.member == -1 || m.member == -2) {

                    //draw the button background
                    GLuint bsTex = Texture::genColor(Color(m.button.color.red,m.button.color.green,m.button.color.blue));
					
					glUseProgram(textShader);
					glBindTexture(GL_TEXTURE_2D, bsTex);

					drawRect(vec2(offset.x + m.button.loc.x, offset.y + m.button.loc.y),
							 vec2(offset.x + m.button.loc.x + m.button.dim.x, offset.y + m.button.loc.y + m.button.dim.y));
                    //draw the button text
                    putStringCentered(offset.x + m.button.loc.x + (m.button.dim.x/2),
                                      (offset.y + m.button.loc.y + (m.button.dim.y/2)) - ui::fontSize/2,
                                      m.button.text);

                    //tests if the mouse is over the button
                    if (mouse.x >= offset.x+m.button.loc.x && mouse.x <= offset.x+m.button.loc.x + m.button.dim.x) {
                        if (mouse.y >= offset.y+m.button.loc.y && mouse.y <= offset.y+m.button.loc.y + m.button.dim.y) {

                            //if the mouse if over the button, it draws this white outline
							glBindTexture(GL_TEXTURE_2D, border);
						
							GLfloat verts[] = {offset.x+m.button.loc.x, 					offset.y+m.button.loc.y,				1.0,
                                			   offset.x+m.button.loc.x+m.button.dim.x, 		offset.y+m.button.loc.y,				1.0,
                                			   offset.x+m.button.loc.x+m.button.dim.x, 		offset.y+m.button.loc.y+m.button.dim.y, 1.0,
                                			   offset.x+m.button.loc.x, 					offset.y+m.button.loc.y+m.button.dim.y, 1.0,
                                			   offset.x+m.button.loc.x, 					offset.y+m.button.loc.y, 				1.0};

							glUseProgram(textShader);
							glEnableVertexAttribArray(textShader_attribute_coord);
							glEnableVertexAttribArray(textShader_attribute_tex);

							glVertexAttribPointer(textShader_attribute_coord, 3, GL_FLOAT, GL_FALSE, 0, verts);
							glVertexAttribPointer(textShader_attribute_tex, 2, GL_FLOAT, GL_FALSE, 0, line_tex);
							glDrawArrays(GL_LINE_STRIP, 0, 5);

							glDisableVertexAttribArray(textShader_attribute_coord);
							glDisableVertexAttribArray(textShader_attribute_tex);
							
                            //if the mouse is over the button and clicks
                            if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
                                switch(m.member) {
                                    case 0: //normal button
                                        m.button.func();
                                        break;
                                    case -1:
                                        currentMenu->gotoChild();
                                        break;
                                    case -2:
                                        currentMenu->gotoParent();
                                    default:break;
                                }
                            }
                        }
                    }

                    //if element is a slider
                }else if (m.member == 1) {
                    //combining slider text with variable amount
                    char outSV[32];
                    sprintf(outSV, "%s: %.1f",m.slider.text, *m.slider.var);

                    float sliderW, sliderH;

                    if (m.slider.dim.y > m.slider.dim.x) {
                        //width of the slider handle
                        sliderW = m.slider.dim.x;
                        sliderH = m.slider.dim.y * .05;
                        //location of the slider handle
                        m.slider.sliderLoc = m.slider.minValue + (*m.slider.var/m.slider.maxValue)*(m.slider.dim.y-sliderW);
                    }else{
                        //width of the slider handle
                        sliderW = m.slider.dim.x * .05;
                        sliderH = m.slider.dim.y;
                        //location of the slider handle
                        m.slider.sliderLoc = m.slider.minValue + (*m.slider.var/m.slider.maxValue)*(m.slider.dim.x-sliderW);
                    }
                    //draw the background of the slider
					glUseProgram(textShader);
                    GLuint bsTex = Texture::genColor(Color(m.slider.color.red, m.slider.color.green, m.slider.color.blue, 175));
					GLuint hTex =  Texture::genColor(Color(m.slider.color.red, m.slider.color.green, m.slider.color.blue, 255));					

					glBindTexture(GL_TEXTURE_2D, bsTex);
					drawRect(vec2(offset.x + m.slider.loc.x, offset.y + m.slider.loc.y),
							 vec2(offset.x + m.slider.loc.x + m.slider.dim.x, offset.y + m.slider.loc.y + m.slider.dim.y));
	                    
					//draw the slider handle
                    glBindTexture(GL_TEXTURE_2D, hTex);
					if (m.slider.dim.y > m.slider.dim.x) {
                        drawRect(vec2(offset.x+m.slider.loc.x, offset.y+m.slider.loc.y + (m.slider.sliderLoc * 1.05)),
                            	 vec2(offset.x+m.slider.loc.x + sliderW, offset.y+m.slider.loc.y + (m.slider.sliderLoc * 1.05) + sliderH));

                        //draw the now combined slider text
                        putStringCentered(offset.x + m.slider.loc.x + (m.slider.dim.x/2), (offset.y + m.slider.loc.y + (m.slider.dim.y*1.05)) - ui::fontSize/2, outSV);
                    }else{
                        drawRect(vec2(offset.x+m.slider.loc.x+m.slider.sliderLoc, offset.y+m.slider.loc.y),
                                 vec2(offset.x+m.slider.loc.x + m.slider.sliderLoc + sliderW, offset.y+m.slider.loc.y + sliderH));

                        //draw the now combined slider text
                        putStringCentered(offset.x + m.slider.loc.x + (m.slider.dim.x/2), (offset.y + m.slider.loc.y + (m.slider.dim.y/2)) - ui::fontSize/2, outSV);
                    }
                    //test if mouse is inside of the slider's borders
                    if (mouse.x >= offset.x+m.slider.loc.x && mouse.x <= offset.x+m.slider.loc.x + m.slider.dim.x) {
                        if (mouse.y >= offset.y+m.slider.loc.y && mouse.y <= offset.y+m.slider.loc.y + m.slider.dim.y) {

                            //if it is we draw a white border around it
                            glBindTexture(GL_TEXTURE_2D, border);
							glUseProgram(textShader);

							glEnableVertexAttribArray(textShader_attribute_coord);
							glEnableVertexAttribArray(textShader_attribute_tex);
                            
							GLfloat box_border[] = {offset.x+m.slider.loc.x, 					offset.y+m.slider.loc.y, 				1.0,
                                					offset.x+m.slider.loc.x+m.slider.dim.x, 	offset.y+m.slider.loc.y,				1.0,
                                					offset.x+m.slider.loc.x+m.slider.dim.x, 	offset.y+m.slider.loc.y+m.slider.dim.y,	1.0,
                                					offset.x+m.slider.loc.x, 					offset.y+m.slider.loc.y+m.slider.dim.y,	1.0,
                                					offset.x+m.slider.loc.x, 					offset.y+m.slider.loc.y,				1.0};

							glVertexAttribPointer(textShader_attribute_coord, 3, GL_FLOAT, GL_FALSE, 0, box_border);
							glVertexAttribPointer(textShader_attribute_tex, 2, GL_FLOAT, GL_FALSE, 0, line_tex);
							glDrawArrays(GL_LINE_STRIP, 0, 5);	
                            if (m.slider.dim.y > m.slider.dim.x) {
                                //and a border around the slider handle
                                GLfloat handle_border[] = {offset.x+m.slider.loc.x, 		  static_cast<GLfloat>(offset.y+m.slider.loc.y + (m.slider.sliderLoc * 1.05)),				1.0,
                                						   offset.x+m.slider.loc.x + sliderW, static_cast<GLfloat>(offset.y+m.slider.loc.y + (m.slider.sliderLoc * 1.05)),				1.0,
                                						   offset.x+m.slider.loc.x + sliderW, static_cast<GLfloat>(offset.y+m.slider.loc.y + (m.slider.sliderLoc * 1.05) + sliderH),	1.0,
                                						   offset.x+m.slider.loc.x,           static_cast<GLfloat>(offset.y+m.slider.loc.y + (m.slider.sliderLoc * 1.05) + sliderH),	1.0,
                                						   offset.x+m.slider.loc.x,           static_cast<GLfloat>(offset.y+m.slider.loc.y + (m.slider.sliderLoc * 1.05)),				1.0};
								glVertexAttribPointer(textShader_attribute_coord, 3, GL_FLOAT, GL_FALSE, 0, handle_border);
								glVertexAttribPointer(textShader_attribute_tex, 2, GL_FLOAT, GL_FALSE, 0, line_tex);
								glDrawArrays(GL_LINE_STRIP, 0, 5);	
                            }else{
                                //and a border around the slider handle
                                GLfloat handle_border[] = {offset.x+m.slider.loc.x + m.slider.sliderLoc, offset.y+m.slider.loc.y,							1.0,
                                						   offset.x+m.slider.loc.x + (m.slider.sliderLoc + sliderW), offset.y+m.slider.loc.y,				1.0,
                                						   offset.x+m.slider.loc.x + (m.slider.sliderLoc + sliderW), offset.y+m.slider.loc.y+m.slider.dim.y,1.0,
                                						   offset.x+m.slider.loc.x + m.slider.sliderLoc, offset.y+m.slider.loc.y+m.slider.dim.y,			1.0,
                                						   offset.x+m.slider.loc.x + m.slider.sliderLoc, offset.y+m.slider.loc.y,							1.0};
								glVertexAttribPointer(textShader_attribute_coord, 3, GL_FLOAT, GL_FALSE, 0, handle_border);
								glVertexAttribPointer(textShader_attribute_tex, 2, GL_FLOAT, GL_FALSE, 0, line_tex);
								glDrawArrays(GL_LINE_STRIP, 0, 5);	
                            }


							glDisableVertexAttribArray(textShader_attribute_coord);
							glDisableVertexAttribArray(textShader_attribute_tex);

                            //if we are inside the slider and click it will set the slider to that point
                            if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
                                //change handle location
                                if (m.slider.dim.y > m.slider.dim.x) {
                                    *m.slider.var = (((mouse.y-offset.y) - m.slider.loc.y)/m.slider.dim.y)*100;
                                    //draw a white box over the handle
                                    glBindTexture(GL_TEXTURE_2D, border);
									drawRect(vec2(offset.x+m.slider.loc.x, offset.y+m.slider.loc.y + (m.slider.sliderLoc * 1.05)),
                                             vec2(offset.x+m.slider.loc.x + sliderW, offset.y+m.slider.loc.y + (m.slider.sliderLoc * 1.05) + sliderH));

                                }else{
                                    *m.slider.var = (((mouse.x-offset.x) - m.slider.loc.x)/m.slider.dim.x)*100;
                                    //draw a white box over the handle
                                    glBindTexture(GL_TEXTURE_2D, border);
									drawRect(vec2(offset.x+m.slider.loc.x + m.slider.sliderLoc, offset.y+m.slider.loc.y),
                                             vec2(offset.x+m.slider.loc.x + (m.slider.sliderLoc + sliderW), offset.y+m.slider.loc.y + m.slider.dim.y));
                                }
                            }

                            //makes sure handle can't go below or above min and max values
                            if (*m.slider.var >= m.slider.maxValue)*m.slider.var = m.slider.maxValue;
                            else if (*m.slider.var <= m.slider.minValue)*m.slider.var = m.slider.minValue;
                        }
                    }
                }
            }
            setFontSize(16);
        }


    }
}