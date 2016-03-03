#include <algorithm>

#include <world.h>
#include <ui.h>

/**
 * Defines how many HLINEs tall a blade of grass can be.
 */

#define GRASS_HEIGHT            4

/**
 * Defines the height of the floor in an IndoorWorld.
 */

#define INDOOR_FLOOR_HEIGHT     100


extern Player *player;


/**
 * Contains the current weather, used in many other places/files.
 */

WorldWeather weather = WorldWeather::Sunny;

const std::string bgPaths[2][9]={
    {"bg.png",					// Daytime background
     "bgn.png",					// Nighttime background
     "bgFarMountain.png",		// Furthest layer
     "forestTileFar.png",		// Furthest away Tree Layer
     "forestTileBack.png",		// Closer layer
     "forestTileMid.png",		// Near layer
     "forestTileFront.png",		// Closest layer
     "dirt.png",				// Dirt
     "grass.png"},				// Grass
    {"bgWoodTile.png",
     "bgWoodTile.png",
     "bgWoodTile.png",
     "bgWoodTile.png",
     "bgWoodTile.png",
     "bgWoodTile.png",
     "bgWoodTile.png",
     "bgWoodTile.png"}
};

const std::string buildPaths[] = {
    "townhall.png",
	"house1.png", 
    "house2.png", 
    "house1.png", 
    "house1.png", 
    "fountain1.png",
    "lampPost1.png",
	"brazzier.png"
};

/**
 * Constants used for layer drawing in World::draw(), releated to transparency.
 */

const float bgDraw[4][3]={
	{ 100, 240, 0.6  },
	{ 150, 250, 0.4  },
	{ 200, 255, 0.25 },
	{ 255, 255, 0.1  }
};

/**
 * Sets the desired theme for the world's background.
 * 
 * The images chosen for the background layers are selected depending on the
 * world's background type.
 */

void World::
setBackground( WorldBGType bgt )
{
    // load textures with a limit check
	switch ( (bgType = bgt) ) {
	case WorldBGType::Forest:
		bgTex = new Texturec( bgFiles );
		break;
        
	case WorldBGType::WoodHouse:
		bgTex = new Texturec( bgFilesIndoors );
		break;

    default:
        UserError( "Invalid world background type" );
        break;
	}
}

/**
 * Sets the world's style.
 * 
 * The world's style will determine what sprites are used for things like\
 * generic structures.
 */

void World::
setStyle( std::string pre )
{
    unsigned int i;
    
    // get folder prefix
	std::string prefix = pre.empty() ? "assets/style/classic/" : pre;
    
	for ( i = 0; i < arrAmt(buildPaths); i++ )
		sTexLoc.push_back( prefix + buildPaths[i] );
		
	prefix += "bg/";
    
	for ( i = 0; i < arrAmt(bgPaths[0]); i++ )
		bgFiles.push_back( prefix + bgPaths[0][i] );
		
	for ( i = 0; i < arrAmt(bgPaths[1]); i++ )
		bgFilesIndoors.push_back( prefix + bgPaths[1][i] );
}

/**
 * Creates a world object.
 * 
 * Note that all this does is nullify pointers, to prevent as much disaster as
 * possible. Functions like setBGM(), setStyle() and generate() should be called
 * before the World is actually put into use.
 */

World::
World( void )
{
    bgmObj = NULL;
    
	toLeft = NULL;
	toRight = NULL;
}

/**
 * The entity vector destroyer.
 * 
 * This function will free all memory used by all entities, and then empty the
 * vectors they were stored in.
 */

void World::
deleteEntities( void )
{
    // free mobs
	while ( !mob.empty() ) {
		delete mob.back();
		mob.pop_back();
	}

	merchant.clear();
	while(!npc.empty()){
		delete npc.back();
		npc.pop_back();
	}
    
    // free structures
	while ( !build.empty() ) {
		delete build.back();
		build.pop_back();
	}
    
    // free objects
	while ( !object.empty() ) {
		delete object.back();
		object.pop_back();
	}
    
    // clear entity array
	entity.clear();
    
    // free particles
	particles.clear();
    
    // clear light array
	light.clear();

    // free villages
	while ( !village.empty() ) {
		delete village.back();
		village.pop_back();
	}
}

/**
 * The world destructor.
 * 
 * This will free objects used by the world itself, then free the vectors of
 * entity-related objects.
 */

World::
~World( void )
{
    // sdl2_mixer's object
	if(bgmObj)
		Mix_FreeMusic(bgmObj);
    
	delete bgTex;
	
	delete[] toLeft;
	delete[] toRight;

	deleteEntities();
}

/**
 * Generates a world of the specified width.
 * 
 * This will mainly populate the WorldData array, mostly preparing the World
 * object for usage.
 */

void World::
generate( unsigned int width )
{	
    // iterator for `for` loops
	std::vector<WorldData>::iterator wditer;
    
    // see below for description
    float geninc = 0;
    
    // check for valid width
    if ( (int)width <= 0 )
        UserError("Invalid world dimensions");

    // allocate space for world
    worldData = std::vector<WorldData> (width + GROUND_HILLINESS, (WorldData) { false, {0,0}, 0, 0 });
    lineCount = worldData.size();
    
    // prepare for generation
    worldData.front().groundHeight = GROUND_HEIGHT_INITIAL;
    wditer = worldData.begin();
    
    // give every GROUND_HILLINESSth entry a groundHeight value
    for(unsigned int i = GROUND_HILLINESS; i < worldData.size(); i += GROUND_HILLINESS, wditer += GROUND_HILLINESS)
        worldData[i].groundHeight = (*wditer).groundHeight + (randGet() % 8 - 4);

    // create slopes from the points that were just defined, populate the rest of the WorldData structure
    
    for(wditer = worldData.begin(); wditer != worldData.end(); wditer++){
        if ((*wditer).groundHeight)
			// wditer + GROUND_HILLINESS can go out of bounds (invalid read)
            geninc = ( (*(wditer + GROUND_HILLINESS)).groundHeight - (*wditer).groundHeight ) / (float)GROUND_HILLINESS;
        else
            (*wditer).groundHeight = (*(wditer - 1)).groundHeight + geninc;

        (*wditer).groundColor    = randGet() % 32 / 8;
        (*wditer).grassUnpressed = true;
        (*wditer).grassHeight[0] = (randGet() % 16) / 3 + 2;
        (*wditer).grassHeight[1] = (randGet() % 16) / 3 + 2;

        // bound checks
        if ( (*wditer).groundHeight < GROUND_HEIGHT_MINIMUM )
            (*wditer).groundHeight = GROUND_HEIGHT_MINIMUM;
        else if ( (*wditer).groundHeight > GROUND_HEIGHT_MAXIMUM )
			(*wditer).groundHeight = GROUND_HEIGHT_MAXIMUM;
		
		if( (*wditer).groundHeight <= 0 )
			(*wditer).groundHeight = GROUND_HEIGHT_MINIMUM;

    }
    
    // define x-coordinate of world's leftmost 'line'
    worldStart = (width - GROUND_HILLINESS) * HLINE / 2 * -1;
	
    // create empty star array, should be filled here as well...
	star = std::vector<vec2> (100, (vec2) { 0, 0 } );
}

/**
 * Updates all entity and player coordinates with their velocities.
 * 
 * Also handles music fading, although that could probably be placed elsewhere.
 */

void World::
update( Player *p, unsigned int delta )
{
    // update player coords
	p->loc.y += p->vel.y			 * delta;
	p->loc.x +=(p->vel.x * p->speed) * delta;
	
	// update entity coords
	for ( auto &e : entity ) {
		e->loc.y += e->vel.y * delta;
		
        // dont let structures move?
		if ( e->type != STRUCTURET && e->canMove ) {
			e->loc.x += e->vel.x * delta;
			
            // update boolean directions
			if ( e->vel.x < 0 )
                e->left = true;
	   		else if ( e->vel.x > 0 )
                e->left = false;
		}
	}

    // iterate through particles
    particles.erase( std::remove_if( particles.begin(), particles.end(), [&delta](Particles part){ return part.kill( delta ); }), particles.end());
    for ( auto part = particles.begin(); part != particles.end(); part++ ) {
		if ( (*part).canMove ) {
			(*part).loc.y += (*part).vely * delta;
			(*part).loc.x += (*part).velx * delta;

			for ( auto &b : build ) {
				if ( b->bsubtype == FOUNTAIN ) {
					if ( (*part).loc.x >= b->loc.x && (*part).loc.x <= b->loc.x + b->width ) {
						if ( (*part).loc.y <= b->loc.y + b->height * .25)
							particles.erase( part );
					}
				}
			}
		}
	}
	
    // handle music fades
	if ( ui::dialogImportant )
		Mix_FadeOutMusic(2000);
	else if( !Mix_PlayingMusic() )
		Mix_FadeInMusic(bgmObj,-1,2000);
}

/**
 * Set the world's BGM.
 * 
 * This will load a sound file to be played while the player is in this world.
 * If no file is found, no music should play.
 */

void World::
setBGM( std::string path )
{
	if( !path.empty() )
		bgmObj = Mix_LoadMUS( (bgm = path).c_str() );
}

/**
 * Toggle play/stop of the background music.
 * 
 * If new music is to be played a crossfade will occur, otherwise... uhm.
 */

void World::
bgmPlay( World *prev ) const
{
	if ( prev ) {
		if ( bgm != prev->bgm ) {
            // new world, new music
			Mix_FadeOutMusic( 800 );
			Mix_PlayMusic( bgmObj, -1 );
		}
	} else {
        // first call
		Mix_FadeOutMusic( 800 );
		Mix_PlayMusic( bgmObj, -1 );
	}
}

/**
 * Variables used by World::draw().
 * @{
 */

extern vec2 offset;
extern unsigned int tickCount;

int worldShade = 0;

/**
 * @}
 */

/**
 * The world draw function.
 * 
 * This function will draw the background layers, entities, and player to the
 * screen.
 */

void World::
draw( Player *p )
{
    // iterators
    int i, iStart, iEnd;
    
    // shade value for draws -- may be unnecessary
	int shadeBackground = 0;
    
    // player's offset in worldData[]
	int pOffset;
    
    // world width in pixels
	int width = worldData.size() * HLINE;
	
	/*
     * Draw background images.
     */
	
	glEnable( GL_TEXTURE_2D );
	
	// the sunny wallpaper is faded with the night depending on tickCount
	
	bgTex->bind( 0 );
	safeSetColorA( 255, 255, 255, 255 - worldShade * 4 );
	
	glBegin( GL_QUADS );
		glTexCoord2i( 0, 0 ); glVertex2i(  worldStart, SCREEN_HEIGHT );
		glTexCoord2i( 1, 0 ); glVertex2i( -worldStart, SCREEN_HEIGHT );
		glTexCoord2i( 1, 1 ); glVertex2i( -worldStart, 0 );
		glTexCoord2i( 0, 1 ); glVertex2i(  worldStart, 0 );
	glEnd();

	bgTex->bindNext();
	safeSetColorA( 255, 255, 255, worldShade * 4 );
	
	glBegin( GL_QUADS );
		glTexCoord2i( 0, 0 ); glVertex2i(  worldStart, SCREEN_HEIGHT );
		glTexCoord2i( 1, 0 ); glVertex2i( -worldStart, SCREEN_HEIGHT );
		glTexCoord2i( 1, 1 ); glVertex2i( -worldStart, 0 );
		glTexCoord2i( 0, 1 ); glVertex2i(  worldStart, 0 );
	glEnd();
	
	glDisable( GL_TEXTURE_2D );
	
	// draw the stars if the time deems it appropriate
	
	if (((( weather == WorldWeather::Dark  ) & ( tickCount % DAY_CYCLE )) < DAY_CYCLE / 2)   ||
	    ((( weather == WorldWeather::Sunny ) & ( tickCount % DAY_CYCLE )) > DAY_CYCLE * .75) ){

		if (tickCount % DAY_CYCLE) {	// The above if statement doesn't check for exact midnight.
				
			safeSetColorA( 255, 255, 255, shadeBackground + getRand() % 30 - 15 );
			
			for ( i = 0; i < 100; i++ ) {
				glRectf(star[i].x + offset.x * .9,
						star[i].y,
						star[i].x + offset.x * .9 + HLINE,
						star[i].y + HLINE
						);
			}
		}
	}
	
	// draw remaining background items
	
	glEnable( GL_TEXTURE_2D );
	
	bgTex->bindNext();
	safeSetColorA( 150 - shadeBackground, 150 - shadeBackground, 150 - shadeBackground, 220 );
	
	glBegin( GL_QUADS );
		for ( i = 0; i <= (int)(worldData.size() * HLINE / 1920); i++ ) {
			glTexCoord2i( 0, 1 ); glVertex2i( width / 2 * -1 + (1920 * i      ) + offset.x * .85, GROUND_HEIGHT_MINIMUM        );
			glTexCoord2i( 1, 1 ); glVertex2i( width / 2 * -1 + (1920 * (i + 1)) + offset.x * .85, GROUND_HEIGHT_MINIMUM        );
			glTexCoord2i( 1, 0 ); glVertex2i( width / 2 * -1 + (1920 * (i + 1)) + offset.x * .85, GROUND_HEIGHT_MINIMUM + 1080 );
			glTexCoord2i( 0, 0 ); glVertex2i( width / 2 * -1 + (1920 * i      ) + offset.x * .85, GROUND_HEIGHT_MINIMUM + 1080 );
		}
	glEnd();
	
	for ( i = 4; i--; ) {
		bgTex->bindNext();
		safeSetColorA( bgDraw[i][0] - shadeBackground, bgDraw[i][0] - shadeBackground, bgDraw[i][0] - shadeBackground, bgDraw[i][1] );
	
		glBegin( GL_QUADS );
			for( int j = worldStart; j <= -worldStart; j += 600 ){
				glTexCoord2i( 0, 1 ); glVertex2i(  j        + offset.x * bgDraw[i][2], GROUND_HEIGHT_MINIMUM       );
				glTexCoord2i( 1, 1 ); glVertex2i( (j + 600) + offset.x * bgDraw[i][2], GROUND_HEIGHT_MINIMUM       );
				glTexCoord2i( 1, 0 ); glVertex2i( (j + 600) + offset.x * bgDraw[i][2], GROUND_HEIGHT_MINIMUM + 400 );
				glTexCoord2i( 0, 0 ); glVertex2i(  j        + offset.x * bgDraw[i][2], GROUND_HEIGHT_MINIMUM + 400 );
			}
		glEnd();
	}
	
	glDisable( GL_TEXTURE_2D );
	
	// draw black under backgrounds
	
	glColor3ub( 0, 0, 0 );
	glRectf( worldStart, GROUND_HEIGHT_MINIMUM, -worldStart, 0 );
	
	pOffset = (offset.x + p->width / 2 - worldStart) / HLINE;
	
    /*
     * Prepare for world ground drawing.
     */
    
	// only draw world within player vision
    
	if ((iStart = pOffset - (SCREEN_WIDTH / 2 / HLINE) - GROUND_HILLINESS) < 0)
		iStart = 0;

	if ((iEnd = pOffset + (SCREEN_WIDTH / 2 / HLINE) + GROUND_HILLINESS + HLINE) > (int)worldData.size())
		iEnd = worldData.size();
	else if (iEnd < GROUND_HILLINESS)
		iEnd = GROUND_HILLINESS;

	// draw particles and buildings

	std::for_each( particles.begin(), particles.end(), [](Particles part) { if ( part.behind ) part.draw(); });
	
	for ( auto &b : build )
		b->draw();

	// draw light elements?
	
	glEnable( GL_TEXTURE_2D );
    
	glActiveTexture( GL_TEXTURE0 );
	bgTex->bindNext();
	
	GLfloat pointArray[ light.size() + (int)p->light ][2];
	
	for ( i = 0; i < (int)light.size(); i++ ) {
		pointArray[i][0] = light[i].loc.x - offset.x;
		pointArray[i][1] = light[i].loc.y;
	}
	
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	
	glUseProgram( shaderProgram );
	glUniform1i( glGetUniformLocation( shaderProgram, "sampler"), 0 );
	glUniform1f( glGetUniformLocation( shaderProgram, "amb"    ), 1 );
	
	if ( p->light ) {
		pointArray[light.size() + 1][0] = (float)( p->loc.x + SCREEN_WIDTH / 2 );
		pointArray[light.size() + 1][1] = (float)( p->loc.y );
	}
	
	if ( light.size() + (int)p->light == 0)
		glUniform1i( glGetUniformLocation( shaderProgram, "numLight"), 0);
	else {
		glUniform1i ( glGetUniformLocation( shaderProgram, "numLight"     ), light.size() + (int)p->light );
		glUniform2fv( glGetUniformLocation( shaderProgram, "lightLocation"), light.size() + (int)p->light, (GLfloat *)&pointArray ); 
		glUniform3f ( glGetUniformLocation( shaderProgram, "lightColor"   ), 1.0f, 1.0f, 1.0f );
	}
	
    /*
     * Draw the dirt?
     */
    
	glBegin( GL_QUADS );
	
        // faulty
        /*glTexCoord2i(0 ,0);glVertex2i(pOffset - (SCREEN_WIDTH / 1.5),0);
        glTexCoord2i(64,0);glVertex2i(pOffset + (SCREEN_WIDTH / 1.5),0);
        glTexCoord2i(64,1);glVertex2i(pOffset + (SCREEN_WIDTH / 1.5),GROUND_HEIGHT_MINIMUM);
        glTexCoord2i(0 ,1);glVertex2i(pOffset - (SCREEN_WIDTH / 1.5),GROUND_HEIGHT_MINIMUM);*/

        for ( i = iStart; i < iEnd; i++ ) {
            if ( worldData[i].groundHeight <= 0 ) {
                worldData[i].groundHeight = GROUND_HEIGHT_MINIMUM - 1;
                glColor4ub( 0, 0, 0, 255 );
            } else
                safeSetColorA( 150, 150, 150, 255 );

            glTexCoord2i( 0, 0 ); glVertex2i(worldStart + i * HLINE         , worldData[i].groundHeight - GRASS_HEIGHT );
            glTexCoord2i( 1, 0 ); glVertex2i(worldStart + i * HLINE + HLINE , worldData[i].groundHeight - GRASS_HEIGHT );
            
            glTexCoord2i( 1, (int)(worldData[i].groundHeight / 64) + worldData[i].groundColor ); glVertex2i(worldStart + i * HLINE + HLINE, 0 );
            glTexCoord2i( 0, (int)(worldData[i].groundHeight / 64) + worldData[i].groundColor ); glVertex2i(worldStart + i * HLINE	      , 0 );
            
            if ( worldData[i].groundHeight == GROUND_HEIGHT_MINIMUM - 1 )
                worldData[i].groundHeight = 0;
        }
	
	glEnd();
    
	glUseProgram(0);
	glDisable(GL_TEXTURE_2D);
	
	/*
	 *	Draw the grass/the top of the ground.
	 */

	glEnable( GL_TEXTURE_2D );
    
	glActiveTexture( GL_TEXTURE0 );
	bgTex->bindNext();
	
	glUseProgram( shaderProgram );
	glUniform1i( glGetUniformLocation( shaderProgram, "sampler"), 0);

	float cgh[2];
	for ( i = iStart; i < iEnd - GROUND_HILLINESS; i++ ) {
		
		// load the current line's grass values
		if ( worldData[i].groundHeight )
			memcpy( cgh, worldData[i].grassHeight, 2 * sizeof( float ));
		else
			memset( cgh, 0 , 2 * sizeof( float ));
			
		// flatten the grass if the player is standing on it.
		if( !worldData[i].grassUnpressed ){
			cgh[0] /= 4;
			cgh[1] /= 4;
		}
		
		// actually draw the grass.
		
		safeSetColorA( 255, 255, 255, 255 );
		
		glBegin( GL_QUADS );
			glTexCoord2i( 0, 0 ); glVertex2i( worldStart + i * HLINE            , worldData[i].groundHeight + cgh[0] );
			glTexCoord2i( 1, 0 ); glVertex2i( worldStart + i * HLINE + HLINE / 2, worldData[i].groundHeight + cgh[0] );
			glTexCoord2i( 1, 1 ); glVertex2i( worldStart + i * HLINE + HLINE / 2, worldData[i].groundHeight - GRASS_HEIGHT );
			glTexCoord2i( 0, 1 ); glVertex2i( worldStart + i * HLINE		    , worldData[i].groundHeight - GRASS_HEIGHT );
			glTexCoord2i( 0, 0 ); glVertex2i( worldStart + i * HLINE + HLINE / 2, worldData[i].groundHeight + cgh[1] );
			glTexCoord2i( 1, 0 ); glVertex2i( worldStart + i * HLINE + HLINE    , worldData[i].groundHeight + cgh[1] );
			glTexCoord2i( 1, 1 ); glVertex2i( worldStart + i * HLINE + HLINE    , worldData[i].groundHeight - GRASS_HEIGHT );
			glTexCoord2i( 0, 1 ); glVertex2i( worldStart + i * HLINE + HLINE / 2, worldData[i].groundHeight - GRASS_HEIGHT );
		glEnd();
	}

	glUseProgram(0);
	glDisable(GL_TEXTURE_2D);

	/*
     * Draw remaining entities.
     */
	
	std::for_each( particles.begin(), particles.end(), [](Particles part) { if ( !part.behind ) part.draw(); });
	
	for ( auto &n : npc )
		n->draw();

	for ( auto &m : mob )
		m->draw();

	for ( auto &o : object )
		o->draw();
	
    /*
     * Handle grass-squishing.
     */
    
	// calculate the line that the player is on
	int ph = ( p->loc.x + p->width / 2 - worldStart ) / HLINE;

	// flatten grass under the player if the player is on the ground
	if ( p->ground ) {
		for ( i = 0; i < (int)(worldData.size() - GROUND_HILLINESS); i++ )
			worldData[i].grassUnpressed = !( i < ph + 6 && i > ph - 6 );
	} else {
		for ( i = 0; i < (int)(worldData.size() - GROUND_HILLINESS); i++ )
			worldData[i].grassUnpressed = true;
	}

	/*
     * Draw the player.
     */
     
	p->draw();
}

/**
 * Handles physics and such for a single entity.
 * 
 * This function is kept private, as World::detect() should be used instead to
 * handle stuffs for all entities at once.
 */

void World::
singleDetect( Entity *e )
{
	std::string killed;
	unsigned int i,j;
	int l;
	
	/*
	 *	Kill any dead entities.
	*/
	
	if ( !e->alive || e->health <= 0 ) {
		for ( i = 0; i < entity.size(); i++) {
			if ( entity[i] == e ){
				switch ( e->type ) {
				case STRUCTURET:
					killed = "structure";
					for(j=0;j<build.size();j++){
						if(build[j]==e){
							delete build[j];
							build.erase(build.begin()+j);
							break;
						}
					}
					break;
				case NPCT:
					killed = "NPC";
					for(j=0;j<npc.size();j++){
						if(npc[j]==e){
							delete npc[j];
							npc.erase(npc.begin()+j);
							break;
						}
					}
					break;
				case MOBT:
					killed = "mob";
					/*for(j=0;j<mob.size();j++){
						if(mob[j]==e){
							delete mob[j];
							mob.erase(mob.begin()+j);
							break;
						}
					}*/
					break;
				case OBJECTT:
					killed = "object";
					for(j=0;j<object.size();j++){
						if(object[j]==e){
							delete object[j];
							object.erase(object.begin()+j);
							break;
						}
					}
					break;
				default:
					break;
				}
				std::cout << "Killed a " << killed << "..." << std::endl;
				entity.erase(entity.begin()+i);
				return;
			}
		}
		std::cout<<"RIP "<<e->name<<"."<<std::endl;
		exit(0);
	}
	
	/*
	 *	Handle only living entities.
	*/
    
	if(e->alive){
	  
		if ( e->type == MOBT && Mobp(e)->subtype == MS_TRIGGER )
			return;
	  
		/*
		 *	Calculate the line that this entity is currently standing on.
		*/
		
		l=(e->loc.x + e->width / 2 - worldStart) / HLINE;
		if(l < 0) l=0;
		i = l;
		if(i > lineCount-1) i=lineCount-1;
		
		/*
		 *	If the entity is under the world/line, pop it back to the surface.
		*/
		
		if ( e->loc.y < worldData[i].groundHeight ) {

			e->loc.y= worldData[i].groundHeight - .001 * deltaTime;
			e->ground=true;
			e->vel.y=0;
		
		/*
		 *	Handle gravity if the entity is above the line.
		*/
		
		}else{
			
			if(e->type == STRUCTURET && e->loc.y > 2000){
				e->loc.y = worldData[i].groundHeight;
				e->vel.y = 0;
				e->ground = true;
				return;
			}else if(e->vel.y > -2)e->vel.y-=.003 * deltaTime;
					
		}
		
		/*
		 *	Insure that the entity doesn't fall off either edge of the world.
		*/

		if(e->loc.x < worldStart){												// Left bound
			
			e->vel.x=0;
			e->loc.x=(float)worldStart + HLINE / 2;
			
		}else if(e->loc.x + e->width + HLINE > worldStart + worldStart * -2){	// Right bound
			
			e->vel.x=0;
			e->loc.x=worldStart + worldStart * -2 - e->width - HLINE;
			
		}
	}
}

void World::
detect( Player *p )
{
	// handle the player
	std::thread( &World::singleDetect, this, p).detach();
		
    // handle other entities
	for ( auto &e : entity )
		std::thread(&World::singleDetect,this,e).detach();
        
    // handle particles
	for ( auto &part : particles ) {
		int l;
		unsigned int i;
		l=(part.loc.x + part.width / 2 - worldStart) / HLINE;
		if(l < 0) l=0;
		i = l;
		if(i > lineCount-1) i=lineCount-1;
		if(part.loc.y < worldData[i].groundHeight){
			part.loc.y = worldData[i].groundHeight;
			part.vely = 0;
			part.velx = 0;
			part.canMove = false;
		}else{
			if(part.gravity && part.vely > -2)
				part.vely-=.003 * deltaTime;
		}
	}
	for(auto &b : build){
		switch(b->bsubtype){
			case FOUNTAIN:
				for(int r = 0; r < (rand()%25)+10;r++){
					addParticle(	rand()%HLINE*3 + b->loc.x + b->width/2,
												b->loc.y + b->height, 
												HLINE*1.25,
												HLINE*1.25, 
												rand()%2 == 0?-(rand()%7)*.01:(rand()%7)*.01,
												((4+rand()%6)*.05), 
												{0,0,255}, 
												2500);

					particles.back().fountain = true;
				}
				break;
			case FIRE_PIT:
				for(int r = 0; r < (rand()%20)+10;r++){
					addParticle(rand()%(int)(b->width/2) + b->loc.x+b->width/4, b->loc.y+3*HLINE, HLINE, HLINE, rand()%2 == 0?-(rand()%3)*.01:(rand()%3)*.01,((4+rand()%6)*.005), {255,0,0}, 400);
					particles.back().gravity = false;
					particles.back().behind = true;
				}
				break;
			default: break;
		}
	}

	for(auto &v : village){
		if(p->loc.x > v->start.x && p->loc.x < v->end.x){
			if(!v->in){
				ui::passiveImportantText(5000,"Welcome to %s",v->name.c_str());
				v->in = true;
			}
		}else{
			v->in = false;
		}
	}

	/*if(hey->infront){
		hey = hey->infront;
		goto LOOOOP;
	}*/
}

void World::addStructure(BUILD_SUB sub, float x,float y, std::string tex, std::string inside){
	build.push_back(new Structures());
	build.back()->inWorld = this;
	build.back()->textureLoc = tex;
	
	build.back()->spawn(sub,x,y);
	
	build.back()->inside = inside;
		
	entity.push_back(build.back());
}

void World::addMob(int t,float x,float y){
	mob.push_back(new Mob(t));
	mob.back()->spawn(x,y);
	
	entity.push_back(mob.back());
}

void World::addMob(int t,float x,float y,void (*hey)(Mob *)){
	mob.push_back(new Mob(t));
	mob.back()->spawn(x,y);
	mob.back()->hey = hey;
	
	entity.push_back(mob.back());
}

void World::addNPC(float x,float y){
	npc.push_back(new NPC());
	npc.back()->spawn(x,y);
	
	entity.push_back(npc.back());
}

void World::addMerchant(float x, float y){
	merchant.push_back(new Merchant());
	merchant.back()->spawn(x,y);

	npc.push_back(merchant.back());
	entity.push_back(npc.back());
}

void World::addObject( std::string in, std::string p, float x, float y){
	object.push_back(new Object(in,p));
	object.back()->spawn(x,y);

	entity.push_back(object.back());
}

void World::
addParticle( float x, float y, float w, float h, float vx, float vy, Color color, int d )
{
	particles.emplace_back( x, y, w, h, vx, vy, color, d );
	particles.back().canMove = true;
}

void World::addLight(vec2 loc, Color color){
	if(light.size() < 64){
		light.emplace_back();
		light.back().loc = loc;
		light.back().color = color;
	}
}

char *World::setToLeft(const char *file){
	if(toLeft)
		delete[] toLeft;
	if(!file)
		return (toLeft = NULL);
	
	strcpy((toLeft = new char[strlen(file) + 1]),file);
	return toLeft;
}
char *World::setToRight(const char *file){
	if(toRight)
		delete[] toRight;
	if(!file)
		return (toRight = NULL);
	
	strcpy((toRight = new char[strlen(file) + 1]),file);
	return toRight;
}

World *World::
goWorldLeft( Player *p )
{
	World *tmp;
    
    // check if player is at world edge
	if(toLeft && p->loc.x < worldStart + HLINE * 15.0f){
        
        // load world (`toLeft` conditional confirms existance)
		tmp = loadWorldFromXML(toLeft);
		
        // adjust player location
		p->loc.x = tmp->worldStart - HLINE * -10.0f;
		p->loc.y = tmp->worldData[tmp->lineCount - 1].groundHeight;
		
		return tmp;
	}
	return this;
}		

World *World::goWorldRight(Player *p){
	World *tmp;
	
	if(toRight && p->loc.x + p->width > -worldStart - HLINE * 15){
		tmp = loadWorldFromXML(toRight);
		
		p->loc.x = tmp->worldStart + HLINE * 10;
		p->loc.y = GROUND_HEIGHT_MINIMUM;
		
		return tmp;
	}
	return this;
}

std::vector<std::string> inside;

World *World::
goInsideStructure( Player *p )
{
	World *tmp;
	char *current;
	if(inside.empty()){
		for(auto &b : build){
			if(p->loc.x            > b->loc.x            &&
			   p->loc.x + p->width < b->loc.x + b->width ){
				inside.push_back((std::string)(currentXML.c_str() + 4));
				
				tmp = loadWorldFromXML(b->inside.c_str());
				
				ui::toggleBlackFast();
				ui::waitForCover();
				ui::toggleBlackFast();
				
				return tmp;
			}
		}
	}else{
		strcpy((current = new char[strlen((const char *)(currentXML.c_str() + 4)) + 1]),(const char *)(currentXML.c_str() + 4));
		tmp = loadWorldFromXML(inside.back().c_str());
		for(auto &b : tmp->build){
			if(!strcmp(current,b->inside.c_str())){
				inside.pop_back();

				ui::toggleBlackFast();
				ui::waitForCover();
				
				p->loc.x = b->loc.x + (b->width / 2);
				
				ui::toggleBlackFast();
				
				return tmp;
			}
		}
		delete[] current;
	}
	return this;
}

void World::addHole(unsigned int start,unsigned int end){
	unsigned int i;
	for(i=start;i<end;i++){
		worldData[i].groundHeight = 0;
	}
}

int World::
getTheWidth( void ) const
{
	return worldStart * -2;
}

void World::save(void){
	std::string data;
	
	std::string save = (std::string)currentXML + ".dat";
	std::ofstream out (save,std::ios::out | std::ios::binary);
	
	std::cout<<"Saving to "<<save<<" ..."<<std::endl;

	for(auto &n : npc){
		data.append(std::to_string(n->dialogIndex) + "\n");
		data.append(std::to_string((int)n->loc.x) + "\n");
		data.append(std::to_string((int)n->loc.y) + "\n");
	}
	
	for(auto &b : build){
		data.append(std::to_string((int)b->loc.x) + "\n");
		data.append(std::to_string((int)b->loc.y) + "\n");
	}
	
	for(auto &m : mob){
		data.append(std::to_string((int)m->loc.x) + "\n");
		data.append(std::to_string((int)m->loc.y) + "\n");
		data.append(std::to_string((int)m->alive) + "\n");
	}
	
	data.append("dOnE\0");
	out.write(data.c_str(),data.size());
	
	out.close();
}

#include <sstream>

extern int  commonAIFunc(NPC *);
extern void commonTriggerFunc(Mob *);

void World::load(void){
	std::string save,data,line;
	const char *filedata;
	
	save = (std::string)currentXML + ".dat";
	filedata = readFile(save.c_str());
	data = filedata;
	std::istringstream iss (data);
	
	for(auto &n : npc){
		std::getline(iss,line);
		if(line == "dOnE")return;
		if((n->dialogIndex = std::stoi(line)) != 9999)
			n->addAIFunc(commonAIFunc,false);
		else n->clearAIFunc();
		
		std::getline(iss,line);
		if(line == "dOnE")return;
		n->loc.x = std::stoi(line);
		std::getline(iss,line);
		if(line == "dOnE")return;
		n->loc.y = std::stoi(line);
	}
	
	for(auto &b : build){
		std::getline(iss,line);
		if(line == "dOnE")return;
		b->loc.x = std::stoi(line);
		std::getline(iss,line);
		if(line == "dOnE")return;
		b->loc.y = std::stoi(line);
	}
	
	for(auto &m : mob){
		std::getline(iss,line);
		if(line == "dOnE")return;
		m->loc.x = std::stoi(line);
		std::getline(iss,line);
		if(line == "dOnE")return;
		m->loc.y = std::stoi(line);
		std::getline(iss,line);
		if(line == "dOnE")return;
		m->alive = std::stoi(line);
	}
	
	while(std::getline(iss,line)){
		if(line == "dOnE")
			break;
	}
	
	delete[] filedata;
}

IndoorWorld::IndoorWorld(void){
}

IndoorWorld::~IndoorWorld(void){
	delete bgTex;
	
	deleteEntities();
}

void IndoorWorld::generate(unsigned int width){		// Generates a flat area of width 'width'
	lineCount=width+GROUND_HILLINESS;			// Sets line count to the desired width plus GEN_INC to remove incorrect line calculations.
	if(lineCount<=0)abort();
	
	worldData = std::vector<WorldData> (lineCount, (WorldData) { false, {0,0}, INDOOR_FLOOR_HEIGHT, 0 });
	
	worldStart = (width - GROUND_HILLINESS) * HLINE / 2 * -1;
}

void IndoorWorld::draw(Player *p){
	unsigned int i,ie;
	//int j,x,v_offset;
	int x;
	
	/*
	 *	Draw the background.
	*/
	
	glEnable(GL_TEXTURE_2D);
	
	GLfloat pointArray[light.size()][2];
	for(uint w = 0; w < light.size(); w++){
		pointArray[w][0] = light[w].loc.x - offset.x;
		pointArray[w][1] = light[w].loc.y;
	}
	glUseProgram(shaderProgram);
	glUniform1i(glGetUniformLocation(shaderProgram, "sampler"), 0);
	glUniform1f(glGetUniformLocation(shaderProgram, "amb"), 0.0f);
	if(p->light){
		glUniform1i(glGetUniformLocation(shaderProgram, "numLight"), 1);
		glUniform2f(glGetUniformLocation(shaderProgram, "lightLocation"), p->loc.x - offset.x+SCREEN_WIDTH/2, p->loc.y);
		glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f,1.0f,1.0f);
	}else if(!light.size()){
		glUniform1i(glGetUniformLocation(shaderProgram, "numLight"), 0);
	}else{
		glUniform1i(glGetUniformLocation(shaderProgram, "numLight"), light.size());
		glUniform2fv(glGetUniformLocation(shaderProgram, "lightLocation"), light.size(), (GLfloat *)&pointArray); 
		glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f,1.0f,1.0f);
	}
	
	bgTex->bind(0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); //for the s direction
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); //for the t direction
	glColor4ub(255,255,255,255);
	
	glBegin(GL_QUADS);
			glTexCoord2i(0,1);							  glVertex2i( worldStart - SCREEN_WIDTH / 2,0);
			glTexCoord2i((-worldStart*2+SCREEN_WIDTH)/512,1);glVertex2i(-worldStart + SCREEN_WIDTH / 2,0);
			glTexCoord2i((-worldStart*2+SCREEN_WIDTH)/512,0);glVertex2i(-worldStart + SCREEN_WIDTH / 2,SCREEN_HEIGHT);
			glTexCoord2i(0,0);							  glVertex2i( worldStart - SCREEN_WIDTH / 2,SCREEN_HEIGHT);
	glEnd();
	
	glUseProgram(0);
	glDisable(GL_TEXTURE_2D);
	
	/*
	 *	Calculate the starting and ending points to draw the ground from.
	*/
	
	/*v_offset = (p->loc.x - x_start) / HLINE;
	j = v_offset - (SCREEN_WIDTH / 2 / HLINE) - GEN_INC;
	if(j < 0)j = 0;
	i = j;
	
	ie = v_offset + (SCREEN_WIDTH / 2 / HLINE) - GEN_INC;
	if(ie > lineCount)ie = lineCount;*/
	
	i = 0;
	ie = lineCount;
	
	/*
	 *	Draw the ground.
	*/
	glUseProgram(shaderProgram);
	glUniform1i(glGetUniformLocation(shaderProgram, "sampler"), 0);
	glBegin(GL_QUADS);
		for(;i < ie - GROUND_HILLINESS;i++){
			safeSetColor(150,100,50);
			
			x = worldStart + i * HLINE;
			glVertex2i(x		,worldData[i].groundHeight);
			glVertex2i(x + HLINE,worldData[i].groundHeight);
			glVertex2i(x + HLINE,worldData[i].groundHeight - 50);
			glVertex2i(x		,worldData[i].groundHeight - 50);
		}
	glEnd();
	glUseProgram(0);
	
	/*
	 *	Draw all entities.
	*/
	
	for ( auto &part : particles )
		part.draw();

	for ( auto &e : entity )
		e->draw();

	p->draw();
}

extern bool inBattle;

std::vector<World *> battleNest;
std::vector<vec2>    battleNestLoc;

Arena::Arena(World *leave,Player *p,Mob *m){
	generate(800);
	addMob(MS_DOOR,100,100);
	
	inBattle = true;
	mmob = m;
	//exit = leave;
	//pxy = p->loc;
	
	mob.push_back(m);
	entity.push_back(m);
	
	battleNest.push_back(leave);
	battleNestLoc.push_back(p->loc);
}

Arena::~Arena(void){
	delete bgTex;

	deleteEntities();
}

World *Arena::exitArena(Player *p){
	World *tmp;
	if(p->loc.x + p->width / 2 > mob[0]->loc.x				&&
	   p->loc.x + p->width / 2 < mob[0]->loc.x + HLINE * 12 ){
		tmp = battleNest.front();
		battleNest.erase(battleNest.begin());
		
		inBattle = !battleNest.empty();
		ui::toggleBlackFast();
		ui::waitForCover();
		
		p->loc = battleNestLoc.back();
		battleNestLoc.pop_back();
		//mmob->alive = false;
		return tmp;
	}else{
		return this;
	}
}

#include <tinyxml2.h>
using namespace tinyxml2;

std::string currentXML;

extern World *currentWorld;

World *loadWorldFromXML(std::string path){
	if ( !currentXML.empty() )
		currentWorld->save();

	return loadWorldFromXMLNoSave(path);
}

/**
 * Loads a world from the given XML file.
 */

World *
loadWorldFromXMLNoSave( std::string path ) {
	XMLDocument xml;
	XMLElement *wxml;
	XMLElement *vil;
	
	World *tmp;
	float spawnx, randx;
	bool dialog,Indoor;
	
	const char *ptr,*name;
		
	currentXML = (std::string)"xml/" + path;

	xml.LoadFile(currentXML.c_str());
	wxml = xml.FirstChildElement("World");

	if(wxml){
		wxml = wxml->FirstChildElement();
		vil = xml.FirstChildElement("World")->FirstChildElement("village");
		Indoor = false;
		tmp = new World();
	}else if((wxml = xml.FirstChildElement("IndoorWorld"))){
		wxml = wxml->FirstChildElement();
		vil = NULL;
		Indoor = true;
		tmp = new IndoorWorld();
	}
	
	while(wxml){
		name = wxml->Name();
		
		if(!strcmp(name,"link")){
			if((ptr = wxml->Attribute("left")))
				tmp->setToLeft(ptr);
			else if((ptr = wxml->Attribute("right")))
				tmp->setToRight(ptr);
			else
				abort();
		}else if(!strcmp(name,"style")){
			tmp->setStyle(wxml->StrAttribute("folder"));
			tmp->setBackground((WorldBGType)wxml->UnsignedAttribute("background"));
			tmp->setBGM(wxml->StrAttribute("bgm"));
		}else if(!strcmp(name,"generation")){
			if(!strcmp(wxml->Attribute("type"),"Random")){
				if(Indoor)
					((IndoorWorld *)tmp)->generate(wxml->UnsignedAttribute("width"));
				else {
					
					tmp->generate(wxml->UnsignedAttribute("width"));
				}
			}else if(Indoor)
				abort();
		}else if(!strcmp(name,"mob")){
			unsigned int type;
			type = wxml->UnsignedAttribute("type");
			if(wxml->QueryFloatAttribute("x",&spawnx) != XML_NO_ERROR)
				tmp->addMob(type,0,100);
			else
				tmp->addMob(type,spawnx,wxml->FloatAttribute("y"));
			if(wxml->QueryBoolAttribute("aggressive",&dialog) == XML_NO_ERROR)
				tmp->mob.back()->aggressive = dialog;
			
		}else if(!strcmp(name,"npc")){
			const char *npcname;

			if(wxml->QueryFloatAttribute("x",&spawnx) != XML_NO_ERROR)
				tmp->addNPC(0,100);
			else
				tmp->addNPC(spawnx,wxml->FloatAttribute("y"));
			
			
			if((npcname = wxml->Attribute("name"))){
				delete[] tmp->npc.back()->name;
				tmp->npc.back()->name = new char[strlen(npcname) + 1];
				strcpy(tmp->npc.back()->name,npcname);
			}
			
			dialog = false;
			if(wxml->QueryBoolAttribute("hasDialog",&dialog) == XML_NO_ERROR && dialog)
				tmp->npc.back()->addAIFunc(commonAIFunc,false);
			else tmp->npc.back()->dialogIndex = 9999;
			
		}else if(!strcmp(name,"structure")){
			tmp->addStructure((BUILD_SUB)wxml->UnsignedAttribute("type"),
							   wxml->QueryFloatAttribute("x",&spawnx) != XML_NO_ERROR ? 
							   			getRand() % tmp->getTheWidth() / 2.0f : 
							   			spawnx,
							   100,
							   wxml->StrAttribute("texture"),
							   wxml->StrAttribute("inside"));
		}else if(!strcmp(name,"trigger")){
			tmp->addMob(MS_TRIGGER,wxml->FloatAttribute("x"),0,commonTriggerFunc);
			tmp->mob.back()->heyid = wxml->Attribute("id");
		}

		wxml = wxml->NextSiblingElement();
	}

	Village *vptr;

	if(vil){
		tmp->village.push_back(new Village(vil->Attribute("name"), tmp));
		vptr = tmp->village.back();

		vil = vil->FirstChildElement();
	}

	while(vil){
		name = vil->Name();
		randx = 0;

		/**
		 * 	READS DATA ABOUT STRUCTURE CONTAINED IN VILLAGE
		 */
		 
		if(!strcmp(name,"structure")){
			tmp->addStructure((BUILD_SUB)vil->UnsignedAttribute("type"),
							   vil->QueryFloatAttribute("x", &spawnx) != XML_NO_ERROR ? randx : spawnx,
							   100,
							   vil->StrAttribute("texture"),
							   vil->StrAttribute("inside"));
		}else if(!strcmp(name, "stall")){
			if(!strcmp(vil->Attribute("type"),"market")){
				std::cout << "Market" << std::endl;
				tmp->addStructure((BUILD_SUB)70,
							   vil->QueryFloatAttribute("x", &spawnx) != XML_NO_ERROR ? 
							   randx : spawnx,
							   100,
							   vil->StrAttribute("texture"),
							   vil->StrAttribute("inside"));
				tmp->addMerchant(0,100);
				if(!strcmp(name,"buy")){
					std::cout << "Buying";
				}else if(!strcmp(name,"sell")){
					std::cout << "Selling";
				}
				strcpy(tmp->merchant.back()->name,"meme");

			}else if(!strcmp(vil->Attribute("type"),"trader")){
				std::cout << "Trader" << std::endl;
				tmp->addStructure((BUILD_SUB)71,
							   vil->QueryFloatAttribute("x", &spawnx) != XML_NO_ERROR ? 
							   randx : spawnx,
							   100,
							   vil->StrAttribute("texture"),
							   vil->StrAttribute("inside"));
			}
		}
		
		vptr->build.push_back(tmp->build.back());
		
		if(vptr->build.back()->loc.x < vptr->start.x){
			vptr->start.x = vptr->build.back()->loc.x;
		}
		
		if(vptr->build.back()->loc.x + vptr->build.back()->width > vptr->end.x){
			vptr->end.x = vptr->build.back()->loc.x + vptr->build.back()->width;
		}

		//go to the next element in the village block
		vil = vil->NextSiblingElement();
	}
	
	std::ifstream dat (((std::string)currentXML + ".dat").c_str());
	if(dat.good()){
		dat.close();
		tmp->load();
	}

	return tmp;
}

Village::Village(const char *meme, World *w){
	name = meme;
	start.x = w->getTheWidth() / 2.0f;
	end.x = -start.x;
	in = false;
}
