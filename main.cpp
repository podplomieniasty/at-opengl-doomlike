#pragma comment(lib, "winmm.lib")

// ---- Biblioteki ----

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <gl/freeglut.h>
#include <math.h>
#include <vector>

#include <Windows.h>
#include <mmsystem.h>

// ---- Tekstury ----

#include "textures/Skybox_01.ppm"
#include "textures/Floor.ppm"
#include "textures/Walls.ppm"
#include "textures/Crossbow.ppm"

#include "textures/Amogus_01.ppm"
#include "textures/Sprites.ppm"


// ---- Splasharty ----

#include "splasharts/Splashart_test.ppm"

// ---- Sta³e ----

#define PI			3.1415926535

#define P2			PI/2			
#define P3			3*PI/2
#define DEG			0.0174533	// stopieñ w radianach

#define WIDTH		1024
#define HEIGHT		768
#define TITLE		"Raycasting - Bialas, Burzec i Kijowski"

#define MAP_HEIGHT	320

#define mapX		10	// szerokoœæ mapy
#define mapY		10	// wysokoœæ mapy
#define mapS		64  // rozmiar kostki mapy

#define RAYS_NUM 144



// ---- Dodatkowe funkcje ----

float degToRad(float a) { return a * PI / 180.0; };
float fixAngle(float a) { if (a > 359) a -= 360; if (a < 0) a += 360; return a; };
float distance(float ax, float ay, float bx, float by, float angle) { return cos(degToRad(angle)) * (bx - ax) - sin(degToRad(angle)) * (by - ay); };



// ---- Enumy ----

enum SpriteType {

	SPRITE_STATIC = 1,
	SPRITE_KEY,
	SPRITE_ENEMY,
	LEVER,
	BULLET,
	COIN
};

enum ScreenType {
	SCR_TITLE = 1,
	SCR_GAME,
	SCR_LV_PASS,
	SCR_LV_FAIL
};



// ---- Struktury ----

typedef struct {

	int w, a, s, d;// klawisze ze stanami on/off
} ButtonKeys;

typedef struct GameInfo {

	int score = 0;
	bool isExitUnlocked = false;
	ScreenType activeScreen = SCR_TITLE;
}; GameInfo gameInfo;			// stany gry

typedef struct Sprite {

	int type;				// statyczny, przeciwnik, pickup -- przerobic na enum
	bool active	=	true;	// czy jest aktywny
	int map		=	0;		// której tekstury u¿yæ
	int x, y, z;			// pozycja
	int dx, dy;
	int scale	=	32;		// skala
	int hitbox	=	30;
	int* texture;
	bool toggled = false;
	int liveTime = -1;		// sekundy
	int spawnedAt = GLUT_ELAPSED_TIME;

	float angle;
	float velocity;

	Sprite(SpriteType type) {
		this->type = type;
		switch (type) {
			case SPRITE_STATIC:
				texture = Amogus_01;
				z = 30;
				break;
			case COIN:
				texture = Sprites;
				z = 15;
				hitbox = 20;
				map = 3;
				break;
			case LEVER:
				texture = Sprites;
				z = 40;
				map = 0;
				scale = 64 * 0.75;
				break;
			case BULLET:
				texture = Sprites;
				z = 15;
				hitbox = 30;
				map = 2;
				break;
		}
	}
	void hide() {
		this->active = false;
	}
	void handleOnPlayerTouch() {
		switch (type) {

			case SPRITE_STATIC:
				active = false;
				break;
			case LEVER:
				if (!toggled) {
					toggled = true;
					map = 1;
					gameInfo.isExitUnlocked = true;
				}
				break;
			case COIN:
				gameInfo.score += 100;
				break;
			

		}		
	}
};

typedef struct {

	float	x,		// pozycja X gracza
			y,		// pozycja Y gracza
			dx,	// delta X
			dy,	// delta Y
			angle;		// k¹t po³o¿enia gracza
} Player;



// ---- Zmienne globalne ----

ButtonKeys keys;			// uk³ad stanów klawiszy gracza
Player player;				// gracz
int rayDepth[RAYS_NUM];		// g³êbia ka¿dego promienia
float frame1, frame2, fps;	// klatki wykorzystane do renderowania

std::vector<Sprite> sprites[2];

int map_walls[] = {

	1,2,3,2,1,3,1,2,1,1,
	1,0,0,0,0,0,0,0,0,3,
	1,0,0,0,0,0,0,0,0,2,
	1,0,0,0,3,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,2,
	1,0,0,0,3,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,2,
	1,0,0,0,0,0,0,0,0,1,
	1,1,1,1,1,1,1,1,1,1,
};	// bloki œcian mapy

int map_floor[] = {

	1,1,1,1,1,1,1,1,1,1,
	1,1,2,3,4,3,2,1,2,1,
	1,1,0,0,0,0,0,0,0,1,
	1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,

};	// pod³oga

int map_ceiling[] = {

	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
};	// sufit



// ---- GRACZ ----

void gm_handleButtonDown(unsigned char key, int x, int y) {

	if (key == 'a') keys.a = 1;
	if (key == 'd') keys.d = 1;
	if (key == 'w') keys.w = 1;
	if (key == 's') keys.s = 1;
	glutPostRedisplay();
}

void gm_handleButtonUp(unsigned char key, int x, int y) {

	if (key == 'a') keys.a = 0;
	if (key == 'd') keys.d = 0;
	if (key == 'w') keys.w = 0;
	if (key == 's') keys.s = 0;
	glutPostRedisplay();
}

void gm_handleClick(int button, int state, int x, int y) {

	if (state == GLUT_DOWN && button == GLUT_LEFT_BUTTON) {
		Sprite bullet(BULLET);
		bullet.velocity = 0.5;
		bullet.angle = player.angle;
		bullet.liveTime = 2 * 1000; // 2 sekundy
		bullet.x = player.x;
		bullet.y = player.y;
		bullet.spawnedAt = glutGet(GLUT_ELAPSED_TIME);
		sprites->push_back(bullet);
	}
}

void gm_drawSprite() {

	// sprawdzenie, czy gracz jest w zasiêgu interakcji ze spritem
	

	for (int s = 0; s < sprites->size(); s++) {

		// touchie touchie
		if (player.x < sprites->at(s).x + sprites->at(s).hitbox &&
			player.x > sprites->at(s).x - sprites->at(s).hitbox &&
			player.y < sprites->at(s).y + sprites->at(s).hitbox &&
			player.y > sprites->at(s).y - sprites->at(s).hitbox &&
			sprites->at(s).type != BULLET) {
			sprites->at(s).handleOnPlayerTouch();
		}

		if (sprites->at(s).type == BULLET) {

			if (glutGet(GLUT_ELAPSED_TIME) - sprites->at(s).spawnedAt >= sprites->at(s).liveTime) {
				sprites->erase(sprites->begin() + s);
				return;
			}

			float dx = cos(degToRad(sprites->at(s).angle));
			float dy = -sin(degToRad(sprites->at(s).angle));


			sprites->at(s).x += dx * sprites->at(s).velocity * fps;
			sprites->at(s).y += dy * sprites->at(s).velocity * fps;

		}

		float spriteX = sprites->at(s).x - player.x;	// odleg³oœæ od gracza
		float spriteY = sprites->at(s).y - player.y;
		float spriteZ = sprites->at(s).z;

		float spriteCos = cos(degToRad(player.angle));	// obrót wzglêdem gracza
		float spriteSin = sin(degToRad(player.angle));

		float a = spriteY * spriteCos + spriteX * spriteSin;
		float b = spriteX * spriteCos - spriteY * spriteSin;
		spriteX = a; spriteY = b;		// pozycja w przestrzeni

		int width = WIDTH / 8;		// ??????
		int height = HEIGHT / 8;	// idk, ale tak musi byc

		float FOV = 108.0;

		spriteX = (spriteX * FOV / spriteY) + width / 2;
		spriteY = (spriteZ * FOV / spriteY) + height / 2;

		int scale = sprites->at(s).scale * height / b;
		if (scale < 0)		scale = 0;
		if (scale > width)	scale = width;

		int x, y;

		float textureX = 0, textureY = 31;
		float textureXStep = 31.5 / (float)scale, textureYStep = 32.0 / (float)scale;	// wysokosc tekstury przez skale

		for (x = spriteX - scale / 2; x < spriteX + scale / 2; x++) {
			textureY = 31;
			for (y = 0; y < scale; y++) {
				if (sprites->at(s).active == 1 && x > 0 && x < 120 && b < rayDepth[x]) {

					int pixel	= ((int)textureY * 32 + (int)textureX) * 3 + (sprites->at(s).map * 32 * 32 * 3);
					int red		= sprites->at(s).texture[pixel + 0];
					int green	= sprites->at(s).texture[pixel + 1];
					int blue	= sprites->at(s).texture[pixel + 2];
					if (!(red == 255 && green == 0 && blue == 255)) {
						glPointSize(8);
						glColor3ub(red, green, blue);
						glBegin(GL_POINTS);
						glVertex2i(x * 8, spriteY * 8 - y * 8);
						glEnd();
					}
					textureY -= textureYStep;
					if (textureY < 0) textureY = 0;
				}
			}
			textureX += textureXStep;
		}
	}
}

// ---- Mapa 2D ----

void gm_drawMapIn2D() {

	// funkcja rysuj¹ca mapê w formacie 2D.
	/* 
		x,  y	->	po³o¿enie w uk³adzie mapy (x,y)
		xo, yo	->	offset x, y

		przerobi siê j¹ na rysowanie mapy
	*/

	int x, y, xo, yo;

	for (y = 0; y < mapY; y++) {

		for (x = 0; x < mapX; x++) {

			if (map_walls[y * mapX + x] == 1) {

				// œciany oznaczone kolorem czarnym
				glColor3f(0, 0, 0);	
			}
			else if (map_walls[y * mapX + x] == 0) {

				// puste pola oznaczone kolorem bia³ym
				glColor3f(1, 1, 1);
			}

			xo = x * mapS;
			yo = y * mapS;

			glBegin(GL_QUADS);
			glVertex2i(xo + 1, yo + 1);
			glVertex2i(xo + 1, yo + mapS - 1);
			glVertex2i(xo + mapS - 1, yo + mapS - 1 );
			glVertex2i(xo + mapS - 1, yo + 1);
			glEnd();
		}
	}
}

void gm_drawPlayer() {

	// rysuje znacznik gracza na
	// mapie 2D

	glColor3f(1, 0, 0);
	glPointSize(8);
	glBegin(GL_POINTS);
	glVertex2i(player.x, player.y);
	glEnd();

	glLineWidth(3);
	glBegin(GL_LINES);
	glVertex2i(player.x, player.y);
	glVertex2i(player.x + player.dx * 20, player.y + player.dy * 20);
	glEnd();
}

float gm_distance(float ax, float ay, float bx, float by, float ang) {

	return sqrt((bx - ax)*(bx - ax) + (by-ay)*(by-ay));
}

void gm_castRays3D() {

	
	/*
		ray		->	aktualny promieñ
		mpX		->	pozycja X promienia na mapie 
		mpY		->	pozycja Y promienia na mapie
		mpP		->	pozycja w tablicy mapy
		dof		->	depth-of-field, w celu okreœlenia co renderowaæ a co nie
		side	->	trafiony bok

		rayX	-> pozycja X punktu, gdzie promieñ napotyka liniê pozioma/pionowa
		rayY	-> pozycja Y punktu, gdzie promieñ ...
		rayA	-> k¹t padania promienia
		xOff	-> offset kolejnego punktu rayX
		yOff	-> offset kolejnego punktu rayY

		disH	->	najkrótszy dystans pomiêdzy graczem a krawêdzi¹ mapy (poziom)
		disV	->	najkrótszy dystans pomiêdzy graczem a krawêdzi¹ mapy (pion)

		vx		->	finalny punkt padania promienia w pionie
		vy		->	-||-

		hmt		-> horizontal map texture?
		vmt		-> jak wy¿ej

	*/
	//int ray, mpX, mpY, mpP, dof, side;

	int ray, mpX, mpY, mpP = 0, dof, side;
	float vx, vy, rayX, rayY, rayA, xOff, yOff, disV, disH;

	rayA = fixAngle(player.angle + 30);		// promieñ z odstêpem 30deg

	for (ray = 0; ray < RAYS_NUM; ray++)
	{
		int vmt = 0,
			hmt = 0;

		//--- LINIE PIONOWE --- 

		dof = 0; 
		side = 0; 
		disV = 100000;	// bardzo du¿a defaultowa odleg³oœæ

		float Tan = tan(degToRad(rayA));

		if (cos(degToRad(rayA)) > 0.001) {			// sprawdŸ lewo

			rayX = (((int)player.x >> 6) << 6) + 64;      
			rayY = (player.x - rayX) * Tan + player.y; 
			xOff = 64; 
			yOff = -xOff * Tan; 
		}

		else if (cos(degToRad(rayA)) < -0.001) {		// sprawdŸ prawo
			rayX = (((int)player.x >> 6) << 6) - 0.0001; 
			rayY = (player.x - rayX) * Tan + player.y; 
			xOff = -64; 
			yOff = -xOff * Tan; }

		// równoleg³e do pionu
		else { 
			rayX = player.x; 
			rayY = player.y; 
			dof = mapY; 
		}

		while (dof < mapY)
		{
			mpX = (int)(rayX) >> 6; 
			mpY = (int)(rayY) >> 6; 
			mpP = mpY * mapX + mpX;

			// sprawdzenie, czy nachodzimy na œcianê
			if (mpP > 0 && mpP < mapX * mapY && map_walls[mpP] > 0) {
				vmt = map_walls[mpP] - 1;
				dof = mapX; 
				disV = cos(degToRad(rayA)) * (rayX - player.x) - sin(degToRad(rayA)) * (rayY - player.y);
			}         
			else {
				rayX += xOff; 
				rayY += yOff; 
				dof += 1; 
			}
		}

		vx = rayX; 
		vy = rayY;

		//--- POZIOME LINIE ---

		dof = 0; disH = 100000;
		Tan = 1.0 / Tan;

		if (sin(degToRad(rayA)) > 0.001) { 			// sprawdzenie góra
			rayY = (((int)player.y >> 6) << 6) - 0.0001; 
			rayX = (player.y - rayY) * Tan + player.x; 
			yOff = -64; 
			xOff = -yOff * Tan; 
		}

		else if (sin(degToRad(rayA)) < -0.001) { 		// sprawdzenie dó³
			rayY = (((int)player.y >> 6) << 6) + 64;      
			rayX = (player.y - rayY) * Tan + player.x; 
			yOff = 64; 
			xOff = -yOff * Tan; 
		}

		// równoleg³e do poziomu
		else { 
			rayX = player.x; 
			rayY = player.y; 
			dof = mapX; 
		} 

		while (dof < mapX)
		{
			mpX = (int)(rayX) >> 6; 
			mpY = (int)(rayY) >> 6; 
			mpP = mpY * mapX + mpX;
			if (mpP > 0 && mpP < mapX * mapY && map_walls[mpP] > 0) {
				hmt = map_walls[mpP] - 1;
				dof = mapX; 
				disH = cos(degToRad(rayA)) * (rayX - player.x) - sin(degToRad(rayA)) * (rayY - player.y);
			}        
			else { 
				rayX += xOff; 
				rayY += yOff; 
				dof += 1; 
			} 
		}

		float shade = 1;	// do cieniowania niektórych œcian
		glColor3f(0, 0.8, 0);
		if (disV < disH) {
			hmt = vmt;
			shade = 0.5;
			rayX = vx; 
			rayY = vy; 
			disH = disV; 
			glColor3f(0, 0.6, 0);
		}

		int ca = fixAngle(player.angle - rayA); 
		disH = disH * cos(degToRad(ca));		// finalny dystans
		int lineH = (mapS * HEIGHT) / (disH);	// wysokoœæ rysowanej linii

		float ty_step = 32.0 / (float)lineH;	
		float ty_off = 0;

		if (lineH > HEIGHT) {
			ty_off = (lineH - HEIGHT)/2.0;
			lineH = HEIGHT;

		}
		int lineOff = HEIGHT/2 - (lineH >> 1);


		rayDepth[ray] = disH; // zapisanie g³êbi promienia

		// rysowanie œcian
		float textureY = ty_off * ty_step; // hmt * 32;
		float textureX;
		if (shade == 1) {
			textureX = (int)(rayX / 2.0) % 32;
			if (rayA > 180) textureX = 31 - textureX;	// odbicie tekstury
		}
		else {
			textureX = (int)(rayY / 2.0) % 32;
			if (rayA > 90 && rayA < 180) textureX = 31 - textureX;
		}

		for (int y = 0; y < lineH; y++) {
			int pixel	=	((int)textureY * 32 + (int)textureX) * 3 + (hmt * 32 * 32);
			int red		=	Walls[pixel + 0] * shade;
			int green	=	Walls[pixel + 1] * shade;
			int blue	=	Walls[pixel + 2] * shade;
			glPointSize(8);
			glColor3ub(red, green, blue);
			glBegin(GL_POINTS);
			glVertex2i(ray * 8, y + lineOff);
			glEnd();
			textureY += ty_step;
		}


		// rysowanie pod³ogi
		for (int y = lineOff + lineH; y < HEIGHT; y++) {

			float dy = y - (HEIGHT / 2.0);
			float deg = degToRad(rayA);
			float raFix = cos(degToRad(fixAngle(player.angle - rayA)));

			float magicNum = 158 * 32 * 2.5;

			textureX = player.x / 2 + cos(deg) * magicNum / dy / raFix;
			textureY = player.y / 2 - sin(deg) * magicNum / dy / raFix;

			int mp = map_floor[(int)(textureY / 32.0) * mapX + (int)(textureX / 32.0)] * 32 * 32;

			int pixel = (((int)(textureY) & 31)*32 + ((int)textureX & 31)) * 3 + mp * 3;
			int red = Floor[pixel + 0] * 0.7;
			int green = Floor[pixel + 1] * 0.7;
			int blue = Floor[pixel + 2] * 0.7;

			glPointSize(8);
			glColor3ub(red, green, blue);
			glBegin(GL_POINTS);
			glVertex2i(ray * 8, y);
			glEnd();

		}

		rayA = fixAngle(rayA - 0.5); // kolejny promieñ
	}
}


// ---- Ekran ----

void gm_init() {

	glClearColor(0.3, 0.3, 0.3, 0);


	player.x = 300; player.y = 300; player.angle = 0;
	player.dx = cos(degToRad(player.angle));
	player.dy = -sin(degToRad(player.angle));

	Sprite s(SPRITE_STATIC);
	s.x = 2 * 64;
	s.y = 5 * 64;
	sprites->push_back(
		s
	);
	Sprite c(COIN);
	c.x = 2 * 64;
	c.y = 2 * 64;
	
	Sprite lev(LEVER);
	lev.x = 1.5 * 64; lev.y = 1.5 * 64;
	sprites->push_back(c);
	sprites->push_back(lev);
}

void gm_displayResize(int width, int height) {

	glutReshapeWindow(WIDTH, HEIGHT);
}


void gm_screen(ScreenType screenType) {

	int* S;
	switch (screenType) {

		case SCR_TITLE:	S = Splashart_test; break;
		default:		S = Splashart_test; break;
	}
	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {

			int pixel	= (y * WIDTH + x) * 3;
			int red = S[pixel + 0];
			int green = S[pixel + 1];
			int blue = S[pixel + 2];
			glPointSize(1);
			glColor3ub(red, green, blue);
			glBegin(GL_POINTS);
			glVertex2i(x, y);
			glEnd();
		}
	}
}

void gm_drawHud() {

	int crossbowWidth = 254;
	int crossbowHeight = 118;
	int pointSize = 2;


	for (int y = 0; y < crossbowHeight; y++) {
		for (int x = 0; x < crossbowWidth; x++) {

			int pixel = (y * crossbowWidth + x) * 3;
			int red = Crossbow[pixel + 0];
			int green = Crossbow[pixel + 1];
			int blue = Crossbow[pixel + 2];
			if (!(red == 255 && green == 0 && blue == 255)) {

				glPointSize(pointSize);
				glColor3ub(red, green, blue);
				glBegin(GL_POINTS);
				glVertex2i(WIDTH - crossbowWidth * pointSize + x * pointSize, HEIGHT - crossbowHeight * pointSize + y * pointSize);
				glEnd();
			}
		}
	}
}

void gm_drawSkybox() {

	// aktualny obrazek ma wymiary
	// 120x80.
	int skyboxWidth = 120;
	int skyboxHeight = 80;
	int* texture = Skybox_01;
	for (int y = 0; y < skyboxHeight/1.5; y++) {
		for (int x = 0; x < skyboxWidth; x++) {

			int xOffset = (int)player.angle*2 - x;
			if (xOffset < 0) xOffset += skyboxWidth;
			xOffset %= skyboxWidth;


			int pixel = (y * skyboxWidth + xOffset) * 3;
			int red = texture[pixel + 0];
			int green = texture[pixel + 1];
			int blue = texture[pixel + 2];

			glPointSize(9);
			glColor3ub(red, green, blue);
			glBegin(GL_POINTS);
			glVertex2i(x * 9, y * 9);
			glEnd();
		}
	}
}

int gameState = 0, timer = 0;

void gm_displayFunc() {

	// tutaj s¹ konfiguracje odnoœnie
	// renderowanego okienka w openglu

	frame2 = glutGet(GLUT_ELAPSED_TIME);
	fps = (frame2 - frame1);
	frame1 = glutGet(GLUT_ELAPSED_TIME);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (gameState == 0) {	// inicjalizacja

		gm_init();
		timer = 0;
		gameState = 1;
		PlaySound(L"audio/DUN_DUN_DUN.wav", NULL, SND_ASYNC | SND_FILENAME);

	}
	if (gameState == 1) {	// okno g³ówne

		gm_screen(SCR_TITLE);
		timer += 1 * fps;
		if (timer > 2500) {
			timer = 0;
			gameState = 2;
		}
	}
	if (gameState == 2) {	// pêtla gry

		if (keys.a == 1) {
			player.angle += 0.2 * fps;
			player.angle = fixAngle(player.angle);
			player.dx = cos(degToRad(player.angle));
			player.dy = -sin(degToRad(player.angle));
		}
		if (keys.d == 1) {
			player.angle -= 0.2 * fps;
			player.angle = fixAngle(player.angle);
			player.dx = cos(degToRad(player.angle));
			player.dy = -sin(degToRad(player.angle));
		}

		// hitbox
		int xoff = 0, yoff = 0;
		xoff = player.dx < 0 ? -20 : 20;
		yoff = player.dy < 0 ? -20 : 20;

		// pozycja w siatce
		int ipx = player.x / 64.0;
		int ipx_add_off = (player.x + xoff) / 64.0;
		int ipx_sub_off = (player.x - xoff) / 64.0;

		int ipy = player.y / 64.0;
		int ipy_add_off = (player.y + yoff) / 64.0;
		int ipy_sub_off = (player.y - yoff) / 64.0;

		if (keys.s == 1) {
			//px -= pdx* 0.2 * fps;
			//py -= pdy * 0.2 * fps;
			if (map_walls[ipy * mapX + ipx_sub_off] == 0) {
				player.x -= player.dx * 0.2 * fps;
			}
			if (map_walls[ipy_sub_off * mapX + ipx] == 0) {
				player.y -= player.dy * 0.2 * fps;
			}
		}
		if (keys.w == 1) {

			if (map_walls[ipy * mapX + ipx_add_off] == 0) {
				player.x += player.dx * 0.2 * fps;
			}
			if (map_walls[ipy_add_off * mapX + ipx] == 0) {
				player.y += player.dy * 0.2 * fps;
			}

		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// wyœwietlanie 2D
		//gm_drawMapIn2D();
		//gm_drawPlayer();

		gm_drawSkybox();
		gm_castRays3D();
		gm_drawSprite();
		gm_drawHud();

	}

	
	
	//gm_screen(SCR_TITLE);
	glutPostRedisplay();
	glutSwapBuffers();
}

int main(int argc, char* argv[]) {

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(WIDTH, HEIGHT);
	glutCreateWindow(TITLE);
	gluOrtho2D(0, WIDTH, HEIGHT, 0);
	gm_init();

	glutDisplayFunc(gm_displayFunc);
	glutReshapeFunc(gm_displayResize);

	glutKeyboardFunc(gm_handleButtonDown);
	glutKeyboardUpFunc(gm_handleButtonUp);
	glutMouseFunc(gm_handleClick);

	glutMainLoop();
	

	return 0;
}