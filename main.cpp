#pragma comment(lib, "winmm.lib")

// ---- Biblioteki ----

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <GL/freeglut.h>
#include <math.h>
#include <vector>
#include <algorithm>

#include <Windows.h>
#include <mmsystem.h>

#include "labirynthparser.h"

// ---- Tekstury ----

#include "textures/Skybox_01.ppm"
#include "textures/Floor.ppm"
#include "textures/Walls.ppm"
#include "textures/Crossbow.ppm"

#include "textures/Amogus_01.ppm"
#include "textures/Chaser.ppm"
#include "textures/Sprites.ppm"

// ---- Splasharty ----

#include "splasharts/Splashart_menu_title.ppm"
#include "splasharts/Splashart_menu_levels.ppm"
#include "splasharts/Splashart_test.ppm"
#include "splasharts/Splashart_win.ppm"
#include "splasharts/Splashart_lose.ppm"

// ---- Sta�e ----

#define PI			3.1415926535	// warto�� PI
#define DEG			0.0174533		// stopie� w radianach

#define WIDTH		1024			// szeroko�� ekranu
#define HEIGHT		768				// wysoko�� ekranu
#define TITLE		"Raycasting - Bialas, Burzec i Kijowski"

#define MAP_HEIGHT	320	
#define mapS		64  // rozmiar kostki mapy

#define RAYS_NUM	240	// ilo�� promieni rysuj�cych

void gm_handleEndGame(bool gitGud = false);

// ---- Funkcje obliczaj�ce ----

float degToRad(float a) { return a * PI / 180.0; };
float fixAngle(float a) { if (a > 359) a -= 360; if (a < 0) a += 360; return a; };
float distance(float ax, float ay, float bx, float by, float angle) { return cos(degToRad(angle)) * (bx - ax) - sin(degToRad(angle)) * (by - ay); };



// ---- Enumy ----

enum SpriteType {

	ENEMY = 1,
	CHASER,
	LEVER,
	BULLET,
	COIN,
	CRATE,
	BARREL,
	BARREL_BOOM,
	ROCK,
	STONE,
	CHAIR,
	EXIT
};
enum ScreenType {
	SCR_TITLE = 1,
	SCR_MENU,
	SCR_GAME,
	SCR_LV_PASS,
	SCR_LV_FAIL
};
enum GameState {
	GM_SETUP = 0,
	GM_MENU_TITLE,
	GM_MENU_LEVELS,
	GM_GAME,
	GM_WIN,
	GM_LOST,
	GM_MAP
};



// ---- Struktury ----

typedef struct {

	int w, a, s, d, tab;// klawisze ze stanami on/off
} ButtonKeys;
typedef struct GameInfo {

	int score				= 0;
	bool isExitUnlocked		= false;
	bool gameOver			= false;
	bool ready				= false;
	ScreenType activeScreen = SCR_TITLE;
	GameState state			= GM_MENU_TITLE;
	int timer				= 0;
	bool canClick			= false;

}; GameInfo gameInfo;			// stany gry
typedef struct Sprite {

	int type;							// statyczny, przeciwnik, pickup -- przerobic na enum
	bool active				=	true;	// czy jest aktywny
	int map					=	0;		// kt�rej tekstury u�y�
	int x, y, z;						// pozycja
	int dx, dy;
	int scale				=	32;		// skala
	int hitbox				=	30;
	int* texture;
	bool toggled			= false;
	int liveTime			= -1;		// sekundy
	int spawnedAt			= glutGet(GLUT_ELAPSED_TIME);
	float detectionRange;
	float dist;

	float angle;
	float velocity;
	float step;

	Sprite(SpriteType type) {
		this->type = type;
		switch (type) {
			case ENEMY:
				texture = Amogus_01;
				z = 30;
				detectionRange = 120.0;
				velocity = 1;
				step = 1;
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
			case CRATE:
				texture = Sprites;
				z = 40;
				map = 4;
				scale = 64;
				hitbox = 50;
				break;
			case BARREL:
				texture = Sprites;
				z = 40;
				map = 8;
				scale = 48;
				hitbox = 30;
				break;
			case EXIT:
				texture = Sprites;
				z = 15;
				hitbox = 50;
				map = 10;
				active = false;
				break;
			case ROCK:
				texture = Sprites;
				z = 40;
				map = 7;
				scale = 48;
				break;
			case STONE:
				texture = Sprites;
				z = 40;
				map = 6;
				scale = 48;
				break;
			case CHAIR:
				texture = Sprites;
				z = 40;
				map = 5;
				scale = 40;
				break;
		}
	}
	void hide() {
		this->active = false;
	}
	void handleOnPlayerTouch() {
		switch (type) {

			case ENEMY:
				if (!toggled) {
					toggled = true;
					gameInfo.gameOver = true;
					gameInfo.canClick = false;
					gm_handleEndGame(true);
					PlaySound(L"audio/loss.wav", NULL, SND_ASYNC | SND_FILENAME);

				}
				break;
			case LEVER:
				if (!toggled) {
					PlaySound(L"audio/lever.wav", NULL, SND_ASYNC | SND_FILENAME);
					toggled = true;
					map = 1;
					gameInfo.isExitUnlocked = true;
				}
				break;
			case COIN:
				if (!toggled) {
					PlaySound(L"audio/coin.wav", NULL, SND_ASYNC | SND_FILENAME);
					gameInfo.score += 100;
					active = false;
					toggled = true;
				}
				break;
			case EXIT:
				if (active) {
					gm_handleEndGame(false);
				}
		}		
	}
};
typedef struct {

	float	x,		// pozycja X gracza
			y,		// pozycja Y gracza
			dx,		// delta X
			dy,		// delta Y
			angle;	// k�t po�o�enia gracza
} Player;



// ---- Zmienne globalne ----

ButtonKeys			keys;			// uk�ad stan�w klawiszy gracza
Player				player;				// gracz
int					rayDepth[RAYS_NUM] = { 0 };		// g��bia ka�dego promienia
float				frame1 = 0, frame2 = 0, fps = 0;	// klatki wykorzystane do renderowania
GameState			state;
GameSettings		settings;
std::vector<Sprite>* sprites;



// ---- Funkcje rysuj�ce ----

void gm_drawSprite() {

	// sprawdzenie, czy gracz jest w zasi�gu interakcji ze spritem

	for (int s = 0; s < sprites->size(); s++) {

		// sortowanie
		sprites->at(s).dist = distance(player.x, player.y, sprites->at(s).x, sprites->at(s).y, player.angle);

		// sprawdzenie czy dotkn�o si� spritea
		if (player.x < sprites->at(s).x + sprites->at(s).hitbox &&
			player.x > sprites->at(s).x - sprites->at(s).hitbox &&
			player.y < sprites->at(s).y + sprites->at(s).hitbox &&
			player.y > sprites->at(s).y - sprites->at(s).hitbox &&
			sprites->at(s).type != BULLET) {
			sprites->at(s).handleOnPlayerTouch();
		}

		// sprawdzenie czy mo�na odblokowa� wyj�cie
		if (sprites->at(s).type == EXIT) {

			if (gameInfo.isExitUnlocked && sprites->at(s).active == false) {
				sprites->at(s).active = true;
			}

		}

		// logika dla pocisk�w
		if (sprites->at(s).type == BULLET) {

			if (glutGet(GLUT_ELAPSED_TIME) - sprites->at(s).spawnedAt >= sprites->at(s).liveTime) {
				sprites->erase(sprites->begin() + s);
				return;
			}

			for (int e = 0; e < sprites->size(); e++) {
				if (sprites->at(e).type == ENEMY) {

					if (sprites->at(s).x < sprites->at(e).x + sprites->at(e).hitbox &&
						sprites->at(s).x > sprites->at(e).x - sprites->at(e).hitbox &&
						sprites->at(s).y < sprites->at(e).y + sprites->at(e).hitbox &&
						sprites->at(s).y > sprites->at(e).y - sprites->at(e).hitbox) {

						sprites->erase(sprites->begin() + e);
						e--;
						return;
					}
				}
				if (sprites->at(e).type == CRATE || sprites->at(e).type == BARREL) {
					if (sprites->at(s).x < sprites->at(e).x + sprites->at(e).hitbox &&
						sprites->at(s).x > sprites->at(e).x - sprites->at(e).hitbox &&
						sprites->at(s).y < sprites->at(e).y + sprites->at(e).hitbox &&
						sprites->at(s).y > sprites->at(e).y - sprites->at(e).hitbox) {

						Sprite c(COIN);
						c.x = sprites->at(e).x;
						c.y = sprites->at(e).y;
						sprites->push_back(c);

						sprites->erase(sprites->begin() + e);
						e--;
						PlaySound(L"audio/wood_break.wav", NULL, SND_ASYNC | SND_FILENAME);

						return;
					}
				}
			}

			float dx = cos(degToRad(sprites->at(s).angle));
			float dy = -sin(degToRad(sprites->at(s).angle));


			sprites->at(s).x += dx * sprites->at(s).velocity * fps;
			sprites->at(s).y += dy * sprites->at(s).velocity * fps;

		}

		if (sprites->at(s).type == ENEMY && gameInfo.state == GM_GAME) {

			if (player.x < sprites->at(s).x + sprites->at(s).detectionRange &&
				player.x > sprites->at(s).x - sprites->at(s).detectionRange &&
				player.y < sprites->at(s).y + sprites->at(s).detectionRange &&
				player.y > sprites->at(s).y - sprites->at(s).detectionRange) {

				float step = sprites->at(s).step;

				int hbx = (int)sprites->at(s).x >> 6; // pozycja w siatce mapy
				int hby = (int)sprites->at(s).y >> 6;

				int offset = sprites->at(s).hitbox;

				int hbx_add = ((int)sprites->at(s).x + offset) >> 6;
				int hbx_sub = ((int)sprites->at(s).x - offset) >> 6;

				int hby_add = ((int)sprites->at(s).y + offset) >> 6;
				int hby_sub = ((int)sprites->at(s).y - offset) >> 6;

				// kolizje dla sprite i poruszanie si�
				if (player.x < sprites->at(s).x && settings.walls[hby * settings.width + hbx_sub] == 0) {
					sprites->at(s).x -= step;
				}
				if (player.x > sprites->at(s).x && settings.walls[hby * settings.width + hbx_add] == 0) {
					sprites->at(s).x += step; 
				}
				if (player.y < sprites->at(s).y && settings.walls[hby_sub * settings.width + hbx] == 0) {
					sprites->at(s).y -= step;
				}
				if (player.y > sprites->at(s).y && settings.walls[hby_add * settings.width + hbx] == 0) {
					sprites->at(s).y += step;
				}
			}

		}

		float spriteX = sprites->at(s).x - player.x;	// odleg�o�� od gracza
		float spriteY = sprites->at(s).y - player.y;
		float spriteZ = sprites->at(s).z;

		float spriteCos = cos(degToRad(player.angle));	// obr�t wzgl�dem gracza
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

					int pixel = ((int)textureY * 32 + (int)textureX) * 3 + (sprites->at(s).map * 32 * 32 * 3);
					int red = sprites->at(s).texture[pixel + 0];
					int green = sprites->at(s).texture[pixel + 1];
					int blue = sprites->at(s).texture[pixel + 2];
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
	for (int s = sprites->size() - 1; s >= 0; s--) {
		if (sprites->at(s).active == false && sprites->at(s).type != EXIT) sprites->erase(sprites->begin() + s);
	}
	std::sort(sprites->begin(), sprites->end(), [](const Sprite& a, Sprite& b) {return a.dist > b.dist; });
}
void gm_drawMapIn2D() {

	// funkcja rysuj�ca map� w formacie 2D.
	/*
		x,  y	->	po�o�enie w uk�adzie mapy (x,y)
		xo, yo	->	offset x, y

		przerobi si� j� na rysowanie mapy
	*/

	int x, y, xo, yo;

	for (y = 0; y < settings.height; y++) {

		for (x = 0; x < settings.width; x++) {

			if (settings.walls[y * settings.width + x] != 0) {

				// �ciany oznaczone kolorem czarnym
				glColor3f(0, 0, 0);
			}
			else if (settings.walls[y * settings.width + x] == 0) {

				// puste pola oznaczone kolorem bia�ym
				glColor3f(1, 1, 1);
			}

			xo = x * mapS / 2;
			yo = y * mapS / 2;



			glBegin(GL_QUADS);
			glVertex2i(xo + 1, yo + 1);
			glVertex2i(xo + 1, yo + mapS - 1);
			glVertex2i(xo + mapS - 1, yo + mapS - 1);
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
	glVertex2i(player.x / 2, player.y / 2);
	glEnd();

	glLineWidth(3);
	glBegin(GL_LINES);
	glVertex2i(player.x / 2, player.y / 2);
	glVertex2i(player.x / 2 + player.dx * 20, player.y / 2 + player.dy * 20);
	glEnd();
}
void gm_castRays3D() {


	/*
		ray		->	aktualny promie�
		mpX		->	pozycja X promienia na mapie
		mpY		->	pozycja Y promienia na mapie
		mpP		->	pozycja w tablicy mapy
		dof		->	depth-of-field, w celu okre�lenia co renderowa� a co nie
		side	->	trafiony bok

		rayX	-> pozycja X punktu, gdzie promie� napotyka lini� pozioma/pionowa
		rayY	-> pozycja Y punktu, gdzie promie� ...
		rayA	-> k�t padania promienia
		xOff	-> offset kolejnego punktu rayX
		yOff	-> offset kolejnego punktu rayY

		disH	->	najkr�tszy dystans pomi�dzy graczem a kraw�dzi� mapy (poziom)
		disV	->	najkr�tszy dystans pomi�dzy graczem a kraw�dzi� mapy (pion)

		vx		->	finalny punkt padania promienia w pionie
		vy		->	-||-

		hmt		-> horizontal map texture?
		vmt		-> jak wy�ej

	*/
	//int ray, mpX, mpY, mpP, dof, side;

	int ray, mpX, mpY, mpP = 0, dof, side;
	float vx, vy, rayX, rayY, rayA, xOff, yOff, disV, disH;

	rayA = fixAngle(player.angle + 30);		// promie� z odst�pem 30deg

	for (ray = 0; ray < RAYS_NUM; ray++)
	{
		int vmt = 0,
			hmt = 0;

		//--- LINIE PIONOWE --- 

		dof = 0;
		side = 0;
		disV = 100000;	// bardzo du�a defaultowa odleg�o��

		float Tan = tan(degToRad(rayA));

		if (cos(degToRad(rayA)) > 0.001) {			// sprawd� lewo

			rayX = (((int)player.x >> 6) << 6) + 64;
			rayY = (player.x - rayX) * Tan + player.y;
			xOff = 64;
			yOff = -xOff * Tan;
		}

		else if (cos(degToRad(rayA)) < -0.001) {		// sprawd� prawo
			rayX = (((int)player.x >> 6) << 6) - 0.0001;
			rayY = (player.x - rayX) * Tan + player.y;
			xOff = -64;
			yOff = -xOff * Tan;
		}

		// r�wnoleg�e do pionu
		else {
			rayX = player.x;
			rayY = player.y;
			dof = settings.height;
		}

		while (dof < settings.height)
		{
			mpX = (int)(rayX) >> 6;
			mpY = (int)(rayY) >> 6;
			mpP = mpY * settings.width + mpX;

			// sprawdzenie, czy nachodzimy na �cian�
			if (mpP > 0 && mpP < settings.width * settings.height && settings.walls[mpP] > 0) {
				vmt = settings.walls[mpP] - 1;
				dof = settings.height;
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

		if (sin(degToRad(rayA)) > 0.001) { 			// sprawdzenie g�ra
			rayY = (((int)player.y >> 6) << 6) - 0.0001;
			rayX = (player.y - rayY) * Tan + player.x;
			yOff = -64;
			xOff = -yOff * Tan;
		}

		else if (sin(degToRad(rayA)) < -0.001) { 		// sprawdzenie d�
			rayY = (((int)player.y >> 6) << 6) + 64;
			rayX = (player.y - rayY) * Tan + player.x;
			yOff = 64;
			xOff = -yOff * Tan;
		}

		// r�wnoleg�e do poziomu
		else {
			rayX = player.x;
			rayY = player.y;
			dof = settings.width;
		}

		while (dof < settings.width)
		{
			mpX = (int)(rayX) >> 6;
			mpY = (int)(rayY) >> 6;
			mpP = mpY * settings.width + mpX;
			if (mpP > 0 && mpP < settings.width * settings.height && settings.walls[mpP] > 0) {
				hmt = settings.walls[mpP] - 1;
				dof = settings.width;
				disH = cos(degToRad(rayA)) * (rayX - player.x) - sin(degToRad(rayA)) * (rayY - player.y);
			}
			else {
				rayX += xOff;
				rayY += yOff;
				dof += 1;
			}
		}

		float shade = 1;	// do cieniowania niekt�rych �cian
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
		int lineH = (mapS * HEIGHT) / (disH);	// wysoko�� rysowanej linii

		float ty_step = 32.0 / (float)lineH;
		float ty_off = 0;

		if (lineH > HEIGHT) {
			ty_off = (lineH - HEIGHT) / 2.0;
			lineH = HEIGHT;

		}
		int lineOff = HEIGHT / 2 - (lineH >> 1);


		rayDepth[ray] = disH; // zapisanie g��bi promienia

		// rysowanie �cian
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
			int pixel = ((int)textureY * 32 + (int)textureX) * 3 + (hmt * 32 * 32 * 3);
			int red = Walls[pixel + 0] * shade;
			int green = Walls[pixel + 1] * shade;
			int blue = Walls[pixel + 2] * shade;

			glPointSize(8);
			glColor3ub(red, green, blue);
			glBegin(GL_POINTS);
			glVertex2i(ray * 8, y + lineOff);
			glEnd();
			textureY += ty_step;
		}


		// rysowanie pod�ogi
		for (int y = lineOff + lineH; y < HEIGHT; y++) {

			float dy = y - (HEIGHT / 2.0);
			float deg = degToRad(rayA);
			float raFix = cos(degToRad(fixAngle(player.angle - rayA)));

			int magicNum = 190 * 32 * 2;

			textureX = player.x / 2 + cos(deg) * magicNum / dy / raFix;
			textureY = player.y / 2 - sin(deg) * magicNum / dy / raFix;

			int mp = settings.floor[(int)(textureY / 32.0) * settings.width + (int)(textureX / 32.0)] * 32 * 32;

			int pixel = (((int)(textureY) & 31) * 32 + ((int)textureX & 31)) * 3 + mp * 3;
			int red = Floor[pixel + 0] * 0.7;
			int green = Floor[pixel + 1] * 0.7;
			int blue = Floor[pixel + 2] * 0.7;

			glPointSize(8);
			glColor3ub(red, green, blue);
			glBegin(GL_POINTS);
			glVertex2i(ray * 8, y);
			glEnd();

		}
		rayA = fixAngle(rayA - 0.5); // kolejny promie�
	}
}
void gm_drawHud() {

	int crossbowWidth = 128;
	int crossbowHeight = 64;
	int pointSize = 5;

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
				glVertex2i(WIDTH - crossbowWidth * pointSize + x * pointSize, HEIGHT - crossbowHeight * pointSize + y * pointSize + pointSize);
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
	for (int y = 0; y < skyboxHeight / 1.5; y++) {
		for (int x = 0; x < skyboxWidth; x++) {

			int xOffset = (int)player.angle * 2 - x;
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
void gm_drawScreen(ScreenType screenType) {

	int* S;
	int pointSize;
	int width, height;
	switch (screenType) {

	case SCR_TITLE:
		S = Splashart_menu_title;
		pointSize = 4;
		width = 256;
		height = 192;
		break;
	case SCR_MENU:
		S = Splashart_menu_levels;
		pointSize = 4;
		width = 256;
		height = 192;
		break;
	case SCR_LV_PASS:
		S = Splashart_win;
		pointSize = 4;
		width = 256;
		height = 192;
		break;
	case SCR_LV_FAIL:
		S = Splashart_lose;
		pointSize = 4;
		width = 256;
		height = 192;
		break;
	default:
		S = Splashart_test;
		pointSize = 1;
		width = 1024;
		height = 768;
		break;
	}

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {

			int pixel = (y * width + x) * 3;
			int red = S[pixel + 0];
			int green = S[pixel + 1];
			int blue = S[pixel + 2];
			glPointSize(pointSize);
			glColor3ub(red, green, blue);
			glBegin(GL_POINTS);
			glVertex2i(x * pointSize, y * pointSize);
			glEnd();
		}
	}
}



// ---- Funkcje gry ----

void gm_handleClickSetState(GameState state) {
	gameInfo.state = state;
}
void gm_placeSprite(SpriteType type, int gridX, int gridY, int offset = 0) {

	Sprite s(type);
	s.x = gridX * mapS + mapS / 2;
	s.y = gridY * mapS + mapS / 2;
	sprites->push_back(s);
}
void gm_placeExit() {

	int x = -1, y = -1;
	if (settings.exitX != -1 && settings.exitY != 1) {
		x = settings.exitX;
		y = settings.exitY;
	}
	else {
		int pos = -1;
		while (pos != 0 && pos != (settings.posY * settings.width + settings.posX)) {
			x = rand() % settings.width;
			if (x == 0 || x == settings.width - 1) continue;
			y = rand() % settings.width;
			if (y == 0 || y == settings.height - 1) continue;
			pos = settings.walls[y * settings.width + x];
		}
	}
	gm_placeSprite(EXIT, x, y);
	settings.floor[y * settings.width + x] = 6;
}
void gm_placePlayer(int posX, int posY) {
	player.x = (posX * mapS + mapS / 2) + 1.0f;//settings.posX;
	player.y = (posY * mapS + mapS / 2);//settings.posY;
	player.angle = fixAngle(0);
	player.dx = cos(degToRad(player.angle));
	player.dy = -sin(degToRad(player.angle));
}
void gm_randomPlaceEntity(SpriteType type, int offset = 0) {

	int pos = -1, x = -1, y = -1;
	while (pos != 0 && pos != (settings.posY * settings.width + settings.posX)) {
		x = rand() % settings.width;
		if (x == 0 || x == settings.width - 1) continue;
		y = rand() % settings.width;
		if (y == 0 || y == settings.height - 1) continue;
		pos = settings.walls[y * settings.width + x];
	}
	
	gm_placeSprite(type, x, y);
}



// ---- Funkcje ustawie� ----

void gm_init() {

	glClearColor(0.3, 0.3, 0.3, 0);
	sprites = new std::vector<Sprite>;
	gameInfo.state = GM_SETUP;
}
void gm_setupLabirynth(int lvNum) {

	settings = fetchLabirynth(lvNum);

	gm_placeExit();
	gm_placePlayer(settings.posX, settings.posY);

	int enemies = 0;
	int barrels = 2;
	int crates = 2;
	int coins = 5;
	int levers = 1;
	int rocks = 3;
	int stones = 3;

	if (lvNum == 1) {	// labirynt

		// utw�rz 2 przeciwnik�w
		enemies = 2;
		barrels = 3;
		crates = 1;
		coins = 3;
		levers = 1;
		rocks = 2;
		stones = 4;
	}
	else if (lvNum == 2) {

		gm_placeSprite(CRATE,  3, 1);
		gm_placeSprite(BARREL, 2, 1);
		gm_placeSprite(BARREL, 3, 3);
		gm_placeSprite(CHAIR,  1, 2);

		gm_placeSprite(ENEMY, 6, 6);

	}
	else if (lvNum == 3) {	// testowa plansza

		gm_placeSprite(ENEMY, 5, 5);
	}
	
	for (int i = 0; i < enemies; i++) {
		gm_randomPlaceEntity(ENEMY);
	}
	for (int i = 0; i < barrels; i++) {
		gm_randomPlaceEntity(BARREL);
	}
	for (int i = 0; i < crates; i++) {
		gm_randomPlaceEntity(CRATE);
	}
	for (int i = 0; i < coins; i++) {
		gm_randomPlaceEntity(COIN);
	}
	for (int i = 0; i < levers; i++) {
		gm_randomPlaceEntity(LEVER);
	}
	for (int i = 0; i < rocks; i++) {
		gm_randomPlaceEntity(ROCK);
	}
	for (int i = 0; i < stones; i++) {
		gm_randomPlaceEntity(STONE);
	}

	gm_handleClickSetState(GM_GAME);
}
void gm_displayResize(int width, int height) {

	glutReshapeWindow(WIDTH, HEIGHT);
}
void gm_displayFunc() {

	// tutaj s� konfiguracje odno�nie
	// renderowanego okienka w openglu

	frame2 = glutGet(GLUT_ELAPSED_TIME);
	fps = (frame2 - frame1);
	frame1 = glutGet(GLUT_ELAPSED_TIME);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (gameInfo.state == GM_SETUP) {	// inicjalizacja

		gameInfo.timer = 0;
		gameInfo.state = GM_MENU_TITLE;
		sprites = new std::vector<Sprite>[1];
		gameInfo.score = 0;
		PlaySound(L"audio/DUN_DUN_DUN.wav", NULL, SND_ASYNC | SND_FILENAME);

	}
	if (gameInfo.state == GM_MENU_TITLE) {	// okno g��wne

		gm_drawScreen(gameInfo.activeScreen);

		glColor3f(1.0f, 1.0f, 1.0f);
		glRasterPos2i(WIDTH / 8, HEIGHT / 3);
	}
	if (gameInfo.state == GM_MENU_LEVELS) {

		gm_drawScreen(gameInfo.activeScreen);
	}

	if (gameInfo.state == GM_MAP) {
		gameInfo.canClick = false;
		gm_drawMapIn2D();
		gm_drawPlayer();
	}

	if (gameInfo.state == GM_WIN) {

		gm_drawScreen(gameInfo.activeScreen);
		glColor3f(1.0f, 1.0f, 1.0f);
		glRasterPos2f(100, 200);
		glutBitmapString(GLUT_BITMAP_TIMES_ROMAN_24, (unsigned char*)("Liczba zdobytych punktow: " + gameInfo.score));

	}
	if (gameInfo.state == GM_LOST) {
		gm_drawScreen(gameInfo.activeScreen);
	}

	if (gameInfo.state == GM_GAME) {	// p�tla gry

		gameInfo.canClick = true;

		if (keys.a == 1) {
			player.angle += 0.2 * fps;;
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

			if (settings.walls[ipy * settings.width + ipx_sub_off] == 0) {
				player.x -= player.dx * 0.2 * fps;
			}
			if (settings.walls[ipy_sub_off * settings.width + ipx] == 0) {
				player.y -= player.dy * 0.2 * fps;
			}
		}
		if (keys.w == 1) {

			if (settings.walls[ipy * settings.width + ipx_add_off] == 0) {
				player.x += player.dx * 0.2 * fps;
			}
			if (settings.walls[ipy_add_off * settings.width + ipx] == 0) {
				player.y += player.dy * 0.2 * fps;
			}

		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		gm_drawSkybox();
		gm_castRays3D();
		gm_drawSprite();
		gm_drawHud();

	}
	glutPostRedisplay();
	glutSwapBuffers();
}
void gm_handleEndGame(bool gitGud) {

	gameInfo.canClick = true;
	gameInfo.isExitUnlocked = false;
	if (gitGud) {
		gameInfo.activeScreen = SCR_LV_FAIL;
		gameInfo.state = GM_LOST;
	}
	else {
		gameInfo.activeScreen = SCR_LV_PASS;
		gameInfo.state = GM_WIN;
		PlaySound(L"audio/DUN_DUN_DUN.wav", NULL, SND_ASYNC | SND_FILENAME);
	}
}



// ---- Funkcje przycisk�w ----

void gm_handleButtonDown(unsigned char key, int x, int y) {

	const int spacebar = 32;
	if (key == spacebar && gameInfo.state == GM_MENU_TITLE) {
		gameInfo.state = GM_MENU_LEVELS;
		gameInfo.activeScreen = SCR_MENU;
	}


	if (key == 'a')		keys.a = 1;
	if (key == 'd')		keys.d = 1;
	if (key == 'w')		keys.w = 1;
	if (key == 's')		keys.s = 1;
	if (key == 'm' && gameInfo.state == GM_GAME) 	gameInfo.state = GM_MAP;
	if ((gameInfo.state == GM_LOST || gameInfo.state == GM_WIN) && key == spacebar) {
		gameInfo.activeScreen = SCR_TITLE;
		gameInfo.state = GM_SETUP;
	}
	glutPostRedisplay();
}
void gm_handleButtonUp(unsigned char key, int x, int y) {

	if (key == 'a')		keys.a = 0;
	if (key == 'd')		keys.d = 0;
	if (key == 'w')		keys.w = 0;
	if (key == 's')		keys.s = 0;
	if (key == 'm' && gameInfo.state == GM_MAP)	gameInfo.state = GM_GAME;

	glutPostRedisplay();
}
void gm_handleClick(int button, int state, int x, int y) {

	if (gameInfo.state == GM_MENU_LEVELS) {

		if (x >= 60 && x <= 297 &&
			y >= 334 && y <= 358) {

			gm_setupLabirynth(1);	// labirynt
		}

		if (x >= 58 && x <= 184 &&
			y >= 394 && y <= 417) {

			gm_setupLabirynth(2);	
		}

		if (x >= 59  && x <= 512 &&
			y >= 453 && y <= 477) {

			gm_setupLabirynth(3);
		}
	}

	if (gameInfo.canClick == true && state == GLUT_DOWN && button == GLUT_LEFT_BUTTON && state != GLUT_UP) {
		PlaySound(L"audio/shoot.wav", NULL, SND_ASYNC | SND_FILENAME);
		Sprite bullet(BULLET);
		bullet.velocity = 0.5;
		bullet.angle = player.angle;
		bullet.liveTime = 0.5 * 1000; // 0.5 sekundy
		bullet.x = player.x;
		bullet.y = player.y;
		bullet.spawnedAt = glutGet(GLUT_ELAPSED_TIME);
		sprites->push_back(bullet);
	}
}



int main(int argc, char* argv[]) {

	srand(time(NULL));

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