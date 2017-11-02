#include <Gamebuino-Meta.h>

const byte ok[] = {8,7,0x2,0x4,0x88,0x48,0x50,0x30,0x20}; // from bub
const byte ko[] = {8,7,0x82,0x44,0x28,0x10,0x28,0x44,0x82}; // from bub
extern const byte *gamemaps[];
const byte *gamemap;
byte curlevel = 0;
int mapX;
int mapY;
byte mapHeight;
byte mapWidth;
byte numBlocks = 0;
byte playerX;
byte playerY;
byte liftBlock;
bool doLift = false;
bool lookLeft = true;
extern const uint8_t sprites_raw[];
Image sprites = Image(sprites_raw, 6, 0);
extern const uint8_t wall_up_right_raw[];
extern const uint8_t wall_up_left_raw[];
extern const uint8_t wall_down_right_raw[];
extern const uint8_t wall_down_left_raw[];
Image wall_up_right = Image(wall_up_right_raw, 5, 0);
Image wall_up_left = Image(wall_up_left_raw, 5, 0);
Image wall_down_right = Image(wall_down_right_raw, 5, 0);
Image wall_down_left = Image(wall_down_left_raw, 5, 0);
extern const uint8_t dude_raw[];
Image dude = Image(dude_raw, 4, 0);

byte getTile(byte x, byte y) {
	return gamemap[y*mapWidth + x];
}
bool canMove(byte cx, byte cy);
void refreshLevelMenu(byte curPick);
class Block {
	byte x, y;
	bool islift;
	public:
		Block(byte bx,byte by) {
			x = bx;
			y = by;
			islift = false;
		}
		void draw() {
			if (islift) { // we let other stuff handle this as we will be on player
				return;
			}
			int drawX = x*8-mapX;
			int drawY = y*8-mapY;
			if (drawX > 86 || drawY > 68 || drawX < -8 || drawY < -8) {
				return;
			}
			sprites.setFrame(1);
			gb.display.drawImage(drawX, drawY, sprites);
		}
		bool is(byte ix, byte iy) {
			return !islift && ix==x && iy==y;
		}
		void lift() {
			islift = true;
		}
		void put(byte nx, byte ny){
			for (; canMove(nx,ny+1); ny++);
			x = nx;
			y = ny;
			islift = false;
		}
};
Block *blocks[70];
const byte camera_x_offset = 4;
const byte camera_y_offset = 4;
void loadLevel() {
	gamemap = gamemaps[curlevel];
	mapWidth = gamemap[0];
	mapHeight = gamemap[1];
	gamemap += 2;
	for (byte i = 0; i < numBlocks; i++) {
		delete blocks[i];
	}
	numBlocks = 0;
	doLift = false;
	lookLeft = true;
	for (byte y = 0; y < mapHeight; y++) {
		for (byte x = 0; x < mapWidth; x++) {
			byte tile = getTile(x, y);
			if (tile == 9) {
				mapX = 8*(x - 4) - camera_x_offset; // it works, OK!?!?!?!
				mapY = 8*(y - 3) - camera_y_offset;
				playerX = x;
				playerY = y;
			} else if (tile == 1) {
				blocks[numBlocks++] = new Block(x, y);
			}
		}
	}
}
void drawWorld() {
	bool wallMatrix[3][3];
	wall_up_left.setFrame(0);
	wall_up_right.setFrame(0);
	wall_down_left.setFrame(0);
	wall_down_right.setFrame(0);
	// first lets draw the background
	for (byte y = 0; y < 9; y++) {
		for (byte x = 0; x < 11; x++) {
			int drawX = x*8 - camera_x_offset;
			int drawY = y*8 - camera_y_offset;
			gb.display.drawImage(drawX, drawY, wall_up_left);
			gb.display.drawImage(drawX + 4, drawY, wall_up_right);
			gb.display.drawImage(drawX, drawY + 4, wall_down_left);
			gb.display.drawImage(drawX + 4, drawY + 4, wall_down_right);
		}
	}
	bool gotDoor = false;
	byte doorX = 0;
	byte doorY = 0;
	bool rowDoorShadow = true;
	for (byte y = 0; y < mapHeight; y++){
		for (byte x = 0; x < mapWidth; x++){
			int drawX = x*8 - mapX;
			int drawY = y*8 - mapY;
			byte tile = getTile(x, y);
			if (tile == 2) {
				gotDoor = true;
				doorX = x;
				doorY = y;
			}
			if (tile == 3 && gotDoor && (x - doorX == y - doorY)) {
				rowDoorShadow = false;
			}
			if (drawX > 86 || drawY > 68 || drawX < -8 || drawY < -8) {
				continue;
			}
			if (tile == 1 || tile == 9) {
				tile = 0; // we draw background instead
			}
			if (tile == 0 && gotDoor && rowDoorShadow && x > doorX && y >= doorY) {
				if (x - doorX - 1 == y - doorY) {
					tile = 4;
				} else if (x - doorX == y - doorY) {
					tile = 5;
				}
			}
			if (tile == 3) {
				for (byte yy = 0; yy < 3; yy++) {
					for (byte xx = 0; xx < 3; xx++) {
						if (x + xx < 1 || y + yy < 1 || x + xx - 1 >= mapWidth || y + yy - 1 >= mapHeight) {
							wallMatrix[xx][yy] = true;
							continue;
						}
						wallMatrix[xx][yy] = getTile(x + xx - 1, y + yy - 1) == 3;
					}
				}
				byte down_right = 0;
				byte down_left = 0;
				byte up_right = 0;
				byte up_left = 0;
				if (!wallMatrix[0][1]) {
					down_left++;
					up_left++;
				}
				if (!wallMatrix[2][1]) {
					down_right++;
					up_right++;
				}
				if (!wallMatrix[1][0]) {
					up_left += 2;
					up_right += 2;
				}
				if (!wallMatrix[1][2]) {
					down_left += 2;
					down_right += 2;
				}
				if (!down_left && !wallMatrix[0][2]) {
					down_left = 4;
				}
				if (!down_right && !wallMatrix[2][2]) {
					down_right = 4;
				}
				if (!up_left && !wallMatrix[0][0]) {
					up_left = 4;
				}
				if (!up_right && !wallMatrix[2][0]) {
					up_right = 4;
				}
				wall_up_left.setFrame(up_left);
				wall_up_right.setFrame(up_right);
				wall_down_left.setFrame(down_left);
				wall_down_right.setFrame(down_right);
				gb.display.drawImage(drawX, drawY, wall_up_left);
				gb.display.drawImage(drawX + 4, drawY, wall_up_right);
				gb.display.drawImage(drawX, drawY + 4, wall_down_left);
				gb.display.drawImage(drawX + 4, drawY + 4, wall_down_right);
			} else {
				sprites.setFrame(tile);
				gb.display.drawImage(drawX, drawY, sprites);
			}
		}
	}
	for (byte i = 0; i < numBlocks; i++) {
		blocks[i]->draw();
	}
	int drawX = playerX*8 - mapX;
	int drawY = playerY*8 - mapY;
	if (!(drawX > 86 || drawY > 68 || drawX < -8 || drawY < -8)) {
		dude.setFrame(0 + (lookLeft?1:0) + (doLift?2:0));
		gb.display.drawImage(drawX, drawY, dude);
	}
	drawY -= 8;
	if (doLift && !(drawX > 86 || drawY > 68 || drawX < -8 || drawY < -8)) {
		sprites.setFrame(1);
		gb.display.drawImage(drawX, drawY, sprites);
	}
}
bool canMove(byte cx, byte cy) {
	byte tile = getTile(cx,cy);
	if (tile == 3) {
		return false;
	}
	for (byte i = 0; i < numBlocks; i++) {
		if (blocks[i]->is(cx, cy)) {
			return false;
		}
	}
	return true;
}
void moveWorld() {
	drawWorld();
	while(1) {
		if (!gb.update()) {
			continue;
		}
		if(gb.buttons.released(BUTTON_B)){
			return;
		}
		if(gb.buttons.pressed(BUTTON_LEFT)){
			mapX -= 8;
		}
		if(gb.buttons.pressed(BUTTON_RIGHT)){
			mapX += 8;
		}
		if(gb.buttons.pressed(BUTTON_UP)){
			mapY -= 8;
		}
		if(gb.buttons.pressed(BUTTON_DOWN)){
			mapY += 8;
		}
		drawWorld();
	}
}
int32_t moves;
void climb() {
	byte tmpX = playerX + (lookLeft?-1:1);
	if (!canMove(tmpX, playerY) && canMove(tmpX, playerY-1) && canMove(playerX, playerY-1)  && (!doLift || (canMove(tmpX, playerY-2) && canMove(playerX, playerY-2)))){
		playerY--;
		playerX = tmpX;
		mapY -= 8;
		mapX = mapX + (lookLeft?-8:8);
		gb.sound.playTick();
		moves++;
	}
}
int32_t playMap() {
	moves = 0;
	while(1) {
		if (!gb.update()) {
			continue;
		}
		if (gb.buttons.pressed(BUTTON_LEFT)) {
			if (canMove(playerX-1, playerY)) {
				if (doLift && !canMove(playerX-1, playerY-1)) {
					blocks[liftBlock]->put(playerX, playerY-1);
					doLift = false;
				}
				playerX--;
				mapX -= 8;
				gb.sound.playTick();
				moves++;
			} else if (lookLeft) {
				climb();
			}
			lookLeft = true;
		}
		if (gb.buttons.pressed(BUTTON_RIGHT)) {
			if (canMove(playerX+1, playerY)) {
				if (doLift && !canMove(playerX+1, playerY-1)) {
					blocks[liftBlock]->put(playerX, playerY-1);
					doLift = false;
				}
				playerX++;
				mapX += 8;
				gb.sound.playTick();
				moves++;
			} else if (!lookLeft) {
				climb();
			}
			lookLeft = false;
		}
		if (gb.buttons.pressed(BUTTON_A)) {
			if (doLift) {
				byte tmpX = playerX + (lookLeft?-1:1);
				if (canMove(tmpX, playerY-1)) {
					blocks[liftBlock]->put(tmpX, playerY-1);
					doLift = false;
					gb.sound.playOK();
					moves++;
				}
			} else {
				byte tmpX = playerX + (lookLeft?-1:1);
				if (canMove(tmpX, playerY-1) && canMove(playerX, playerY-1)) {
					for (byte i = 0; i < numBlocks; i++) {
						if (blocks[i]->is(lookLeft?playerX-1:playerX+1,playerY)) {
							doLift = true;
							blocks[i]->lift();
							liftBlock = i;
							gb.sound.playOK();
							moves++;
							break;
						}
					}
				}
			}
		}
		if (gb.buttons.pressed(BUTTON_UP)) {
			climb();
		}
		if (gb.buttons.pressed(BUTTON_B)) {
			int oldMapX = mapX;
			int oldMapY = mapY;
			moveWorld();
			mapX = oldMapX;
			mapY = oldMapY;
		}
		if (gb.buttons.pressed(BUTTON_C)) {
			gb.sound.playCancel();
			return 0;
		}
		if (getTile(playerX, playerY) == 2) {
			return moves;
		}
		while (canMove(playerX, playerY+1)) {
			mapY += 8;
			playerY++;
			if (getTile(playerX, playerY) == 2) {
				return moves;
			}
		}
		drawWorld();
	}
}

const uint8_t levelMenuMap[] = {
	10,8,
	3,3,3,3,3,3,3,3,3,3,
	3,0,0,0,0,0,0,0,0,3,
	3,3,3,3,3,3,3,3,3,3,
	3,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,3,
	3,0,0,0,0,0,0,0,0,3,
	3,3,3,3,3,3,3,3,3,3,
};
void drawLevelMenu(byte curPick) {
	drawWorld();
	gb.display.setCursor(20, 9);
	gb.display.print("Level Menu");
	
	gb.display.setCursor(13, 30);
	gb.display.print("Level  \x11");
	if (curPick < 10) {
		gb.display.print(" ");
	}
	gb.display.print(curPick);
	gb.display.print("\x10");
	int32_t _moves = gb.save.get(curPick - 1);
	
	gb.display.drawBitmap(61, 29, _moves>0?ok:ko);
	if (_moves > 0) {
		gb.display.setCursor(13, 42);
		gb.display.print("Moves  ");
		gb.display.print(_moves);
	}
}
void chooseLevel() {
	// create the fake map
	gamemap = levelMenuMap;
	mapWidth = *(gamemap++);
	mapHeight = *(gamemap++);
	for (byte i = 0; i < numBlocks; i++) {
		delete blocks[i];
	}
	numBlocks = 0;
	mapX = mapY = 0;
	playerX = 7;
	playerY = 6;
	doLift = true;
	lookLeft = true;
	
	byte curPick = curlevel+1;
	while(1) {
		if (!gb.update()) {
			continue;
		}
		drawLevelMenu(curPick);
		if (gb.buttons.pressed(BUTTON_RIGHT)) {
			curPick++;
			if (curPick > 11) {
				curPick = 11;
			} else {
				gb.sound.playTick();
			}
		}
		if (gb.buttons.pressed(BUTTON_LEFT)) {
			curPick--;
			if (curPick < 1) {
				curPick = 1;
			} else {
				gb.sound.playTick();
			}
		}
		if (gb.buttons.pressed(BUTTON_A)) {
			gb.sound.playOK();
			break;
		}
	}
	curlevel = curPick-1;
}
void setup() {
	gb.begin();
}
void loop() {
	chooseLevel();
	while(1) {
		loadLevel();
		int32_t _moves = playMap();
		if (_moves > 0) {
			int32_t oldMoves = gb.save.get(curlevel);
			if(_moves < oldMoves || oldMoves <= 0){
				gb.save.set(curlevel, _moves);
			}
			if (curlevel < 11) {
				curlevel++;
				continue;
			}
		}
		break;
	}
}
