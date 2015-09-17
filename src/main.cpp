#include <common.h>
#include <cstdio>
#include <chrono>

#define TICKS_PER_SEC 20
#define MSEC_PER_TICK (1000/TICKS_PER_SEC)

SDL_Window    *window = NULL;
SDL_Surface   *renderSurface = NULL;
SDL_GLContext  mainGLContext = NULL;

bool gameRunning = true;
static float mx,my;

static unsigned int tickCount   = 0,
					prevTime    = 0,
					currentTime = 0,
					deltaTime   = 0;

int npcAmt = 0;
Entity *entPlay;	//The player base
Entity *entnpc[32];	//The NPC base
Player player;		//The actual player object
NPC npc[32];
Structures build;
World *currentWorld;//u-huh

World *spawn;

void logic();
void render();

unsigned int millis(void){
	std::chrono::system_clock::time_point now=std::chrono::system_clock::now();
	return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

int main(int argc, char *argv[]){
	//runs start-up procedures
    if(SDL_Init(SDL_INIT_VIDEO)){
		std::cout << "SDL was not able to initialize! Error: " << SDL_GetError() << std::endl;
		return -1;
	}
    atexit(SDL_Quit);
    if(!(IMG_Init(IMG_INIT_PNG|IMG_INIT_JPG)&(IMG_INIT_PNG|IMG_INIT_JPG))){
		std::cout<<"Could not init image libraries!\n"<<std::endl;
		return -1;
	}
	atexit(IMG_Quit);
	//Turn on double Buffering
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    //ANTIALIASING!!!
    //SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	//SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);
    //create the window
    window = SDL_CreateWindow("Independent Study v.0.2 alpha", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL
                              #ifdef FULLSCREEN
                              | SDL_WINDOW_FULLSCREEN
                              #endif // FULLSCREEN
                              );
    if(window==NULL){
		std::cout << "The window failed to generate! Error: " << SDL_GetError() << std::endl;
		std::cout << "Window address: "<<window<<std::endl;
        return -1;
    }
    //set OpenGL context
    mainGLContext = SDL_GL_CreateContext(window);
    if(mainGLContext == NULL){
		std::cout << "The OpenGL context failed to initialize! Error: " << SDL_GetError() << std::endl;
        return -1;
    }

	glViewport(0,0,SCREEN_WIDTH, SCREEN_HEIGHT);
	glClearColor(.3,.5,.8,0);
	glEnable(GL_BLEND);
	glEnable(GL_MULTISAMPLE);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	SDL_ShowCursor(SDL_DISABLE);
	
	/**************************
	****     GAMELOOP      ****
	**************************/

	ui::init("ttf/VCR_OSD_MONO_1.001.ttf");

	irand(time(NULL));
	entPlay = &player;
	entPlay->spawn(0, 0);

	build.spawn(-1, (grand()%20)-10, 0);
	//build.spawn(-1, 1.5, 0);


	// Generate the world
	World *w=NULL,*w2=NULL;
	w2=new World(4,w,NULL);
	w=new World(20,NULL,w2);
	
	spawn=currentWorld=w;
	currentWorld->addLayer(15);
	currentWorld->addLayer(10);
	// shh
	unsigned char jklasdf;
	for(jklasdf=0;jklasdf<npcAmt;jklasdf++){
		currentWorld->addEntity((void *)entnpc[jklasdf]);
	}
	
	currentTime=millis();
	
	while(gameRunning){
		prevTime = currentTime;
		currentTime = millis();
		deltaTime = currentTime - prevTime;

		player.loc.x += (player.vel.x * player.speed) * deltaTime;						//update the player's x based on 
		player.loc.y += player.vel.y * deltaTime;
		for(int i = 0; i < eAmt(entnpc); i++){
			if(npc[i].alive == true){
				npc[i].loc.y += npc[i].vel.y * deltaTime;
				npc[i].loc.x += npc[i].vel.x * deltaTime;
			}
	    }
	    
		render();
		
		if(prevTime + MSEC_PER_TICK >= millis()){						//the logic loop to run at a dedicated time
			logic();
			prevTime = millis();
		}
	}
	
	/**************************
	****  CLOSE PROGRAM    ****
	**************************/
	
    //closes the window and frees resources
    SDL_GL_DeleteContext(mainGLContext);
    SDL_DestroyWindow(window);
    return 0;
}

void render(){
	static float d,fps;
	static unsigned int div=0;
													//a matrix is a blank canvas for the computer to draw on, the matrices are stored in a "stack"
													//GL_PROJECTION has 2 matrices
													//GL_MODELVIEW has 32 matrices
	glMatrixMode(GL_PROJECTION); 					//set the matrix mode as projection so we can set the ortho size and the camera settings later on
	glPushMatrix(); 								//push the  matrix to the top of the matrix stack
	glLoadIdentity(); 								//replace the entire matrix stack with the updated GL_PROJECTION mode
	//set the the size of the screen
	if(player.loc.x-1<-1){
		glOrtho(-1,1, -1,1, -1,1);
	}else if(player.loc.x+1>-1+currentWorld->getWidth()){
		glOrtho(-3+currentWorld->getWidth(),-1+currentWorld->getWidth(),-1,1,-1,1);
	}else{
		glOrtho(-1 + player.loc.x, 1 + player.loc.x , -1, 1, -1,1);
	}
	glMatrixMode(GL_MODELVIEW); 					//set the matrix to modelview so we can draw objects
	glPushMatrix(); 								//push the  matrix to the top of the matrix stack
	glLoadIdentity(); 								//replace the entire matrix stack with the updated GL_MODELVIEW mode
	glPushMatrix();									//basically here we put a blank canvas (new matrix) on the screen to draw on
	glClear(GL_COLOR_BUFFER_BIT); 					//clear the matrix on the top of the stack

	/**************************
	**** RENDER STUFF HERE ****
	**************************/
	currentWorld->draw(); // layers dont scale x correctly...

	if((mx > player.loc.x && mx < player.loc.x + player.width) && (my > player.loc.y && my < player.loc.y + player.height)){
		glColor3ub(255,0,0);
	}else{
		glColor3ub(120,30,30);							//render the player
	}
	glRectf(player.loc.x, player.loc.y, player.loc.x + player.width, player.loc.y + player.height);

	
	glColor3ub(255,0,0);
	glRectf(build.loc.x, build.loc.y, build.loc.x + build.width, build.loc.y + build.height);
	///BWAHHHHHHHHHHHH

	ui::setFontSize(16);
	if(++div==20){
		div=0;
		d=deltaTime;
		fps=(1000/d);
	}
	//ui.putText(-.98 + player.loc.x, .88, "DT: %1.0f",d);
	ui::putText(player.loc.x,player.loc.y-(HLINE*10),"(%+1.3f,%+1.3f)",player.loc.x,player.loc.y);
	
	/**************************
	****  CLOSE THE LOOP   ****
	**************************/

	//DRAW MOUSE HERE!!!!!
	mx=(ui::mousex/(float)SCREEN_WIDTH)*2.0f-1.0f;
	my=((SCREEN_HEIGHT-ui::mousey)/(float)SCREEN_HEIGHT)*2.0f-1.0f;
	if(player.loc.x-1>-1 && player.loc.x-1<-3+currentWorld->getWidth()){
		if(ui::debug)
			ui::putText(-.98 + player.loc.x, .94, "FPS: %1.0f\nDT: %1.0f",fps, d);
		mx+=player.loc.x;
	}else if(player.loc.x-1>=-3+currentWorld->getWidth()){
		if(ui::debug)
			ui::putText(-.98 + -2+currentWorld->getWidth(), .94, "FPS: %1.0f\nDT: %1.0f",fps, d);
		mx =mx-1 + -1+currentWorld->getWidth();
	}else{
		if(ui::debug)
			ui::putText(-.98, .94, "FPS: %1.0f\nDT: %1.0f",fps, d);
	}

	glBegin(GL_TRIANGLES);

	glColor3ub(255,255,255);
	glVertex2f(mx,my);
	glVertex2f(mx + (HLINE*3.5),my - (HLINE*1));
	glVertex2f(mx + (HLINE*1),my - (HLINE*6));

	glEnd();

	glPopMatrix(); 									//take the matrix(s) off the stack to pass them to the renderer
	SDL_GL_SwapWindow(window); 						//give the stack to SDL to render it
}

void logic(){
	float gw;
	
	ui::handleEvents();								// Handle events

	if(player.right)player.vel.x=.00075;
	else if(player.left)player.vel.x=-.00075;
	else player.vel.x = 0;

	currentWorld->detect(&player.loc,&player.vel,player.width);
	gw=currentWorld->getWidth();
	if(player.loc.x+player.width>-1+gw){
		if(currentWorld->toRight){
			goWorldRight(currentWorld)
			player.loc.x=-1+HLINE;
		}else{
			player.loc.x=gw-1-player.width-HLINE;
		}
	}
	if(player.loc.x<-1){
		if(currentWorld->toLeft){
			goWorldLeft(currentWorld);
			player.loc.x=currentWorld->getWidth()-1-player.width-HLINE;
		}else{
			player.loc.x=-1+HLINE;
		}
	}
	
	currentWorld->detect(&build.loc,&build.vel,build.width);
	for(int i = 0; i < eAmt(entnpc); i++){
		if(npc[i].alive == true){
			currentWorld->detect(&npc[i].loc,&npc[i].vel,npc[i].width);
			entnpc[i]->wander((grand()%181 + 1), &npc[i].vel);
			if((mx > entnpc[i]->loc.x && mx < entnpc[i]->loc.x + entnpc[i]->width) && (my > entnpc[i]->loc.y && my < entnpc[i]->loc.y + entnpc[i]->height)&&(SDL_GetMouseState(NULL,NULL) & SDL_BUTTON(SDL_BUTTON_LEFT))){
				if(pow((entnpc[i]->loc.x - player.loc.x),2) + pow((entnpc[i]->loc.y - player.loc.y),2) < pow(.2,2)){
					entnpc[i]->interact();
					ui::putText(entnpc[i]->loc.x, entnpc[i]->loc.y - HLINE * 3, "HEY", NULL);
				}
			}
		}
	
	}
	tickCount++;
}	
