/* ----------------------------------------------------------------------------
** The main file, home of the main loop.
** --------------------------------------------------------------------------*/
// ...
/* ----------------------------------------------------------------------------
** Includes section
** --------------------------------------------------------------------------*/

#include <brice.hpp>

// local library includes
#include <tinyxml2.h>
using namespace tinyxml2;

// local game includes
#include <common.hpp>
#include <config.hpp>
#include <entities.hpp>
#include <world.hpp>
#include <ui.hpp>
#include <gametime.hpp>

/* ----------------------------------------------------------------------------
** Variables section
** --------------------------------------------------------------------------*/

// the game's window title name
constexpr const char *GAME_NAME = "Independent Study v0.7 alpha - NOW WITH lights and snow and stuff";

// SDL's window object
SDL_Window *window = NULL;

// main loop runs based on this variable's value
bool gameRunning = false;

// world objects for the current world and the two that are adjacent
World *currentWorld        = NULL,
	  *currentWorldToLeft  = NULL,
	  *currentWorldToRight = NULL;

// an arena for fightin'
Arena *arena = nullptr;

// the currently used folder to grab XML files
std::string xmlFolder;

// the current menu
Menu *currentMenu;

// the player object
Player *player;

// shaders for rendering
GLuint fragShader;
GLuint shaderProgram;

/**
 * These are the source and index variables for our shader
 * used to draw text and ui elements
 */

GLuint textShader;
GLint textShader_attribute_coord;
GLint textShader_attribute_tex;
GLint textShader_uniform_texture;
GLint textShader_uniform_transform;
GLint textShader_uniform_color;

/**
 * These are the source and index variables for the world
 * shader which is used to draw the world items and shade them
 */

GLuint worldShader;
GLint worldShader_attribute_coord;
GLint worldShader_attribute_tex;
GLint worldShader_uniform_texture;
GLint worldShader_uniform_transform;
GLint worldShader_uniform_ortho;
GLint worldShader_uniform_color;

// keeps a simple palette of colors for single-color draws
GLuint colorIndex;

// the mouse's texture
GLuint mouseTex;

// the center of the screen
vec2 offset;

// handles all logic operations
void logic(void);

// handles all rendering operations
void render(void);

// takes care of *everything*
void mainLoop(void);

/*******************************************************************************
** MAIN ************************************************************************
********************************************************************************/

int main(int argc, char *argv[]){
	static SDL_GLContext mainGLContext = NULL;

	// handle command line arguments
	if (argc > 1) {
		std::vector<std::string> args (argc, "");
		for (int i = 1; i < argc; i++)
			args[i] = argv[i];

		for (const auto &s : args) {
			if (s == "--reset" || s == "-r")
				system("rm -f xml/*.dat");
		}
	}

	// attempt to initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
		UserError(std::string("SDL was not able to initialize! Error: ") + SDL_GetError());
	atexit(SDL_Quit);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

	// attempt to initialize SDL_image
	if (!(IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) & (IMG_INIT_PNG | IMG_INIT_JPG)))
		UserError(std::string("Could not init image libraries! Error: ") + IMG_GetError());
	atexit(IMG_Quit);

	// attempt to initialize SDL_mixer
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
		UserError(std::string("SDL_mixer could not initialize! Error: ") + Mix_GetError());
	Mix_AllocateChannels(8);
	atexit(Mix_Quit);

	// update values by reading the config file (config/settings.xml)
	game::config::read();

	// create the SDL window object
	window = SDL_CreateWindow(GAME_NAME,
							  SDL_WINDOWPOS_UNDEFINED,	// Spawn the window at random (undefined) x and y coordinates
							  SDL_WINDOWPOS_UNDEFINED,	//
							  game::SCREEN_WIDTH,
							  game::SCREEN_HEIGHT,
							  SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | (game::FULLSCREEN ? SDL_WINDOW_FULLSCREEN : 0)
							  );

    if (window == NULL)
		UserError(std::string("The window failed to generate! SDL_Error: ") + SDL_GetError());

    // create the OpenGL object that SDL provides
    if ((mainGLContext = SDL_GL_CreateContext(window)) == NULL)
		UserError(std::string("The OpenGL context failed to initialize! SDL_Error: ") + SDL_GetError());

	// initialize GLEW
#ifndef __WIN32__
	glewExperimental = GL_TRUE;
#endif

	GLenum err;
	if ((err = glewInit()) != GLEW_OK)
		UserError(std::string("GLEW was not able to initialize! Error: ") + reinterpret_cast<const char *>(glewGetErrorString(err)));

	// start the random number generator
	randInit(millis());

	// 'basic' OpenGL setup
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetSwapInterval(1); // v-sync
	SDL_ShowCursor(SDL_DISABLE); // hide the mouse
	glViewport(0, 0, game::SCREEN_WIDTH, game::SCREEN_HEIGHT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(1,1,1,1);

	// TODO
	Texture::initColorIndex();

	// initialize shaders
	std::cout << "Initializing shaders!\n";

	const GLchar *shaderSource = readFile("frig.frag");
	GLint bufferln = GL_FALSE;
	int logLength;

	fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragShader, 1, &shaderSource, NULL);
	glCompileShader(fragShader);

	glGetShaderiv(fragShader, GL_COMPILE_STATUS, &bufferln);
	glGetShaderiv(fragShader, GL_INFO_LOG_LENGTH, &logLength);

	std::vector<char> fragShaderError ((logLength > 1) ? logLength : 1);

	glGetShaderInfoLog(fragShader, logLength, NULL, &fragShaderError[0]);
	std::cout << &fragShaderError[0] << std::endl;

	if (bufferln == GL_FALSE)
		UserError("Error compiling shader");

	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, fragShader);
	glLinkProgram(shaderProgram);
	glValidateProgram(shaderProgram);

	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &bufferln);
    glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &logLength);
    std::vector<char> programError((logLength > 1) ? logLength : 1);
    glGetProgramInfoLog(shaderProgram, logLength, NULL, &programError[0]);
    std::cout << &programError[0] << std::endl;

	delete[] shaderSource;

	/**
	 *	Creating the text shader and its attributes/uniforms
	 */
	textShader = 					create_program("shaders/new.vert", "shaders/new.frag");
	textShader_attribute_coord = 	get_attrib(textShader, "coord2d");
	textShader_attribute_tex = 		get_attrib(textShader, "tex_coord");
	textShader_uniform_texture = 	get_uniform(textShader, "sampler");
	textShader_uniform_transform = 	get_uniform(textShader, "ortho");
    textShader_uniform_color = 		get_uniform(textShader, "tex_color");

	/**
	 *	Creating the world's shader and its attributes/uniforms
	 */
	worldShader = 					create_program("shaders/world.vert", "shaders/world.frag");
	worldShader_attribute_coord = 	get_attrib(worldShader, "coord2d");
	worldShader_attribute_tex = 	get_attrib(worldShader, "tex_coord");
	worldShader_uniform_texture = 	get_uniform(worldShader, "sampler");
	worldShader_uniform_transform = get_uniform(worldShader, "transform");
	worldShader_uniform_ortho = get_uniform(worldShader, "ortho");
	worldShader_uniform_color = 	get_uniform(worldShader, "tex_color");

	//glEnable(GL_MULTISAMPLE);

	// load up some fresh hot brice
	game::briceLoad();
	game::briceUpdate();

	// load sprites used in the inventory menu. See src/inventory.cpp
	initInventorySprites();

	// load mouse texture, and other inventory textures
	mouseTex = Texture::loadTexture("assets/mouse.png");

	// read in all XML file names in the folder
	std::vector<std::string> xmlFiles;
	if (xmlFolder.empty())
		xmlFolder = "xml/";
	if (getdir(std::string("./" + xmlFolder).c_str(), xmlFiles))
		UserError("Error reading XML files!!!");

	// alphabetically sort files
	strVectorSortAlpha(&xmlFiles);

	// load the first valid XML file for the world
	for (const auto &xf : xmlFiles) {
		if (xf[0] != '.' && strcmp(&xf[xf.size() - 3], "dat")){
			// read it in
			std::cout << "File to load: " << xf << '\n';
			currentWorld = loadWorldFromXML(xf);
			break;
		}
	}

	// make sure the world was made
	if (currentWorld == NULL)
		UserError("Plot twist: The world never existed...?");

	// spawn the player
	player = new Player();
	player->sspawn(0,100);
	ui::menu::init();
	currentWorld->bgmPlay(nullptr);

	// spawn the arena
	arena = new Arena();
	arena->setStyle("");
	arena->setBackground(WorldBGType::Forest);
	arena->setBGM("assets/music/embark.wav");

	// the main loop, in all of its gloriousness..
	gameRunning = true;
	std::thread([&]{
		while (gameRunning)
			mainLoop();
	}).detach();

	while (gameRunning)
		render();

	// put away the brice for later
	game::briceSave();

	// free library resources
    Mix_HaltMusic();
    Mix_CloseAudio();

    destroyInventory();
	ui::destroyFonts();
    Texture::freeTextures();

    SDL_GL_DeleteContext(mainGLContext);
    SDL_DestroyWindow(window);

	// close up the game stuff
	currentWorld->save();
	delete arena;
	//delete currentWorld;

    return 0; // Calls everything passed to atexit
}

/*
 * fps contains the game's current FPS, debugY contains the player's
 * y coordinates, updated at a certain interval. These are used in
 * the debug menu (see below).
 */

static unsigned int fps=0;
static float debugY=0;

void mainLoop(void){
	static unsigned int debugDiv=0;			// A divisor used to update the debug menu if it's open

	game::time::mainLoopHandler();

	if (currentMenu) {
		return;
	} else {
		// handle keypresses - currentWorld could change here
		ui::handleEvents();

		if (game::time::tickHasPassed())
			logic();

		currentWorld->update(player, game::time::getDeltaTime(), game::time::getTickCount());
		currentWorld->detect(player);

		if (++debugDiv == 20) {
			debugDiv=0;

			fps = 1000 / game::time::getDeltaTime();
			debugY = player->loc.y;
		}
	}

	SDL_Delay(1);
}

void render() {
	const auto SCREEN_WIDTH = game::SCREEN_WIDTH;
	const auto SCREEN_HEIGHT = game::SCREEN_HEIGHT;

	offset.x = player->loc.x + player->width / 2;

	// ortho x snapping
	if (currentWorld->getTheWidth() < (int)SCREEN_WIDTH)
		offset.x = 0;
	else if (offset.x - SCREEN_WIDTH / 2 < currentWorld->getTheWidth() * -0.5f)
		offset.x = ((currentWorld->getTheWidth() * -0.5f) + SCREEN_WIDTH / 2);
	else if (offset.x + SCREEN_WIDTH / 2 > currentWorld->getTheWidth() *  0.5f)
		offset.x = ((currentWorld->getTheWidth() *  0.5f) - SCREEN_WIDTH / 2);

	// ortho y snapping
	offset.y = std::max(player->loc.y + player->height / 2, SCREEN_HEIGHT / 2.0f);

	// "setup"
	glm::mat4 projection = glm::ortho(	static_cast<float>(floor(offset.x-SCREEN_WIDTH/2)), 	//left
										static_cast<float>(floor(offset.x+SCREEN_WIDTH/2)), 	//right
										static_cast<float>(floor(offset.y-SCREEN_HEIGHT/2)), 	//bottom
										static_cast<float>(floor(offset.y+SCREEN_HEIGHT/2)), 	//top
										10.0f,								//near
										-10.0f);							//far

	glm::mat4 view = glm::lookAt(glm::vec3(0,0,0.0f),  //pos
								 glm::vec3(0,0,-10.0f), //looking at
								 glm::vec3(0,1.0f,0)); //up vector

	glm::mat4 ortho = projection * view;

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	// TODO add depth
    glEnable(GL_DEPTH_TEST);

	glUseProgram(textShader);
	glUniformMatrix4fv(textShader_uniform_transform, 1, GL_FALSE, glm::value_ptr(ortho));
    glUniform4f(textShader_uniform_color, 1.0, 1.0, 1.0, 1.0);
    glUseProgram(worldShader);
	glUniformMatrix4fv(worldShader_uniform_ortho, 1, GL_FALSE, glm::value_ptr(ortho));
	glUniformMatrix4fv(worldShader_uniform_transform, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
	glUniform4f(worldShader_uniform_color, 1.0, 1.0, 1.0, 1.0);
	/**************************
	**** RENDER STUFF HERE ****
	**************************/

	/**
	 * Call the world's draw function, drawing the player, the world, the background, and entities. Also
	 * draw the player's inventory if it exists.
	 */
	//player->near = true; // allow player's name to be drawn
	currentWorld->draw(player);

	// draw the player's inventory
	player->inv->draw();

	// draw the fade overlay
	ui::drawFade();

	// draw ui elements
	ui::draw();

	/*
	 * Draw the debug overlay if it has been enabled.
	 */

	if(ui::debug){
		ui::putText(offset.x-SCREEN_WIDTH/2, (offset.y+SCREEN_HEIGHT/2)-ui::fontSize,
					"fps: %d\ngrounded:%d\nresolution: %ux%u\nentity cnt: %d\nloc: (%+.2f, %+.2f)\nticks: %u\nvolume: %f\nweather: %s",
					fps,
					player->ground,
					SCREEN_WIDTH,				// Window dimensions
					SCREEN_HEIGHT,				//
					currentWorld->entity.size(),// Size of entity array
					player->loc.x,				// The player's x coordinate
					debugY,						// The player's y coordinate
					game::time::getTickCount(),
					game::config::VOLUME_MASTER,
					currentWorld->getWeatherStr().c_str()
					);

		static GLuint tracerText = Texture::genColor(Color(100,100,255));

		uint es = currentWorld->entity.size();
		GLfloat tpoint[es * 2 * 5];
		GLfloat *tp = &tpoint[0];

		glUseProgram(textShader);
		glBindTexture(GL_TEXTURE_2D, tracerText);

		if (ui::posFlag) {
			for (auto &e : currentWorld->entity) {
				*(tp++) = player->loc.x + player->width / 2;
				*(tp++) = player->loc.y + player->height / 2;
				*(tp++) = -5.0;

				*(tp++) = 0.0;
				*(tp++) = 0.0;

				*(tp++) = e->loc.x + e->width / 2;
 				*(tp++) = e->loc.y + e->height / 2;
				*(tp++) = -5.0;

				*(tp++) = 1.0;
				*(tp++) = 1.0;
			}
		}
		
		glEnableVertexAttribArray(worldShader_attribute_coord);
		glEnableVertexAttribArray(worldShader_attribute_coord);
		
		glVertexAttribPointer(worldShader_attribute_coord, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), &tpoint[0]);
		glVertexAttribPointer(worldShader_attribute_tex, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), &tpoint[3]);
		glDrawArrays(GL_LINES, 0, es * 2);

		glDisableVertexAttribArray(worldShader_attribute_tex);
		glDisableVertexAttribArray(worldShader_attribute_tex);
		glUseProgram(0);
		
	}



	if (currentMenu)
		ui::menu::draw();

		glUseProgram(textShader);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mouseTex);
		glUniform1i(textShader_uniform_texture, 0);

		glEnableVertexAttribArray(textShader_attribute_tex);
		glEnableVertexAttribArray(textShader_attribute_coord);

		glDisable(GL_DEPTH_TEST);

		GLfloat mouseCoords[] = {
			ui::mouse.x			,ui::mouse.y, 	      1.0, //bottom left
			ui::mouse.x+15		,ui::mouse.y, 		  1.0, //bottom right
			ui::mouse.x+15		,ui::mouse.y-15,	  1.0, //top right

			ui::mouse.x+15		,ui::mouse.y-15, 	  1.0, //top right
			ui::mouse.x 		,ui::mouse.y-15, 	  1.0, //top left
			ui::mouse.x			,ui::mouse.y, 		  1.0, //bottom left
		};

		GLfloat mouseTex[] = {
			0.0f, 0.0f, //bottom left
			1.0f, 0.0f, //bottom right
			1.0f, 1.0f, //top right

			1.0f, 1.0f, //top right
			0.0f, 1.0f, //top left
			0.0f, 0.0f, //bottom left
		};

		glVertexAttribPointer(textShader_attribute_coord, 3, GL_FLOAT, GL_FALSE, 0, mouseCoords);
		glVertexAttribPointer(textShader_attribute_tex, 2, GL_FLOAT, GL_FALSE, 0, mouseTex);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(textShader_attribute_coord);
		glDisableVertexAttribArray(textShader_attribute_tex);

		SDL_GL_SwapWindow(window);
}

void logic(){
	static bool NPCSelected    = false;
	static bool ObjectSelected = false;

	// exit the game if the player falls out of the world
	if (player->loc.y < 0)
		gameRunning = false;

	if (player->inv->usingi) {
		for (auto &e : currentWorld->entity) {
			if (player->inv->usingi && !e->isHit() &&
				player->inv->detectCollision(vec2 { e->loc.x, e->loc.y }, vec2 { e->loc.x + e->width, e->loc.y + e->height})) {
				e->takeHit(25, 10);
				break;
			}
		}
		player->inv->usingi = false;
	}

	for (auto &e : currentWorld->entity) {
		if (e->isAlive() && ((e->type == NPCT) || (e->type == MERCHT) || (e->type == OBJECTT))) {
			if (e->type == OBJECTT && ObjectSelected) {
				e->near = false;
				continue;
			} else if (e->canMove) {
				if (!currentWorld->goWorldLeft(dynamic_cast<NPC *>(e)))
					currentWorld->goWorldRight(dynamic_cast<NPC *>(e));
				e->wander((rand() % 120 + 30));
				if (NPCSelected) {
					e->near = false;
					continue;
				}
			}

			if(e->isInside(ui::mouse) && player->isNear(*e)) {
				e->near = true;
				if (e->type == OBJECTT)
					ObjectSelected = true;
				else
					NPCSelected = true;

				if ((SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_RIGHT)) && !ui::dialogBoxExists) {
					if (ui::mouse.x < player->loc.x && player->right)
						player->left = true, player->right = false;
					else if(ui::mouse.x > player->loc.x && player->left)
						player->right = true, player->left = false;
					e->interact();
				}
			} else {
				e->near = false;
			}
		} else if (e->type == MOBT) {
			e->near = player->isNear(*e);
			e->wander();
		}
	}

	// calculate the world shading value
	worldShade = 50 * sin((game::time::getTickCount() + (DAY_CYCLE / 2)) / (DAY_CYCLE / PI));

	// update fades
	ui::fadeUpdate();

	// create weather particles if necessary
	auto weather = currentWorld->getWeatherId();
	if (weather == WorldWeather::Rain) {
		for (unsigned int r = (randGet() % 25) + 11; r--;) {
			currentWorld->addParticle(randGet() % currentWorld->getTheWidth() - (currentWorld->getTheWidth() / 2),
									  offset.y + game::SCREEN_HEIGHT / 2,
									  HLINES(1.25),										// width
									  HLINES(1.25),										// height
									  randGet() % 7 * .01 * (randGet() % 2 == 0 ? -1 : 1),	// vel.x
									  (4 + randGet() % 6) * .05,							// vel.y
									  { 0, 0, 255 },										// RGB color
									  2500,												// duration (ms)
									  (1 << 0) | (1 << 1)									// gravity and bounce
									  );
		}
	} else if (weather == WorldWeather::Snowy) {
		for (unsigned int r = (randGet() % 25) + 11; r--;) {
			currentWorld->addParticle(randGet() % currentWorld->getTheWidth() - (currentWorld->getTheWidth() / 2),
									  offset.y + game::SCREEN_HEIGHT / 2,
									  HLINES(1.25),										// width
									  HLINES(1.25),										// height
							.0001 + randGet() % 7 * .01 * (randGet() % 2 == 0 ? -1 : 1),	// vel.x
									  (4 + randGet() % 6) * -.03,							// vel.y
									  { 255, 255, 255 },									// RGB color
									  5000,												// duration (ms)
									  0													// no gravity, no bounce
									  );
		}
	}

	// increment game ticker
	game::time::tick();
	NPCSelected = false;
	ObjectSelected = false;
}
