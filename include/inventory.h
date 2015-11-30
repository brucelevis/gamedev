#ifndef INVENTORY_H
#define INVENTORY_H

#include <common.h>
#include <string.h>

#define DEBUG

#define ID 			Item(
#define NAME 		,
#define TYPE 		,
#define WIDTH 		,
#define HEIGHT 		,
#define STACKSIZE 	,
#define TEX 		,
#define ENI 		),
#define STOP		)

/*
 * A list of all item IDs.
*/

enum ITEM_ID {
	DEBUG_ITEM = 69,
	TEST_ITEM = 1,
	PLAYER_BAG = 2,
	FLASHLIGHT = 3,
	SWORD_WOOD = 4
};

enum ITEM_TYPE{
	TOOL = 1,
	SWORD = 2,
	RANGED = 3,
	EQUIP = 4,
	FOOD = 5
};

class Item{
protected:
public:
	friend class Inventory;
	friend unsigned int initInventorySprites(void);
	ITEM_ID id;				// ID of the item
	char *name;
	ITEM_TYPE type;			// What category the item falls under
	float width;
	float height;
	int maxStackSize;
	char* textureLoc;
	Texturec *tex;
	int count;
	Item(ITEM_ID i, char* n, ITEM_TYPE t, float w, float h, int m, char* tl):
		id(i), type(t), width(w), height(h), maxStackSize(m){
		count = 0;

		name 		= (char*)calloc(strlen(n ),sizeof(char));
		textureLoc 	= (char*)calloc(strlen(tl),sizeof(char));

		strcpy(name,n);
		strcpy(textureLoc,tl);

		tex= new Texturec(1,textureLoc);
	}
	GLuint rtex(){
		return tex->image[0];
	}
};

struct item_t{
	int count;
	ITEM_ID id;
} __attribute__((packed));


class Inventory {
private:
	unsigned int size;		// Size of 'item' array
	item_t *inv;
	int os = 0;
	//struct item_t *item;	// An array of the items contained in this inventory.
public:
	unsigned int sel;
	bool invOpen = false;
	bool invOpening = false;

	Inventory(unsigned int s);	// Creates an inventory of size 's'
	~Inventory(void);			// Free's 'item'
	
	int addItem(ITEM_ID id,unsigned char count);	// Add 'count' items with an id of 'id' to the inventory
	int takeItem(ITEM_ID id,unsigned char count);	// Take 'count' items with an id of 'id' from the inventory
	int useItem(void);
	
	bool tossd;
	int itemToss(void);
	
	void setSelection(unsigned int s);
	
	void draw(void);	// Draws a text list of items in this inventory (should only be called for the player for now)
	
};

void itemUse(void *p);

#endif // INVENTORY_H
