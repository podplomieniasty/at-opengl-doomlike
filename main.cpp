// ---- Biblioteki ----

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <math.h>


// ---- Tekstury ----

#include "textures/Skybox_01.ppm"

#include "textures/Textures.ppm"

#include "textures/Amogus_01.ppm"


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

#define RAYS_NUM 120



// ---- Dodatkowe funkcje ----

float degToRad(float a) { return a * PI / 180.0; };
float fixAngle(float a) { if (a > 359) a -= 360; if (a < 0) a += 360; return a; };
float distance(float ax, float ay, float bx, float by, float angle) { return cos(degToRad(angle)) * (bx - ax) - sin(degToRad(angle)) * (by - ay); };



// ---- Struktury ----

typedef struct {

	int w, a, s, d;// klawisze ze stanami on/off

} ButtonKeys;

typedef struct {

	int type;		// statyczny, przeciwnik, pickup -- przerobic na enum
	bool alive;		// czy jest aktywny
	int map;		// której tekstury u¿yæ
	int x, y, z;	// pozycja
} Sprite;

int rayDepth[RAYS_NUM];

enum SpriteType {

	SPRITE_STATIC = 1,
	SPRITE_KEY,
	SPRITE_ENEMY,
};




// ---- Zmienne globalne ----

float	px,			// pozycja X gracza
		py,			// pozycja Y gracza
		pdx,		// delta X
		pdy,		// delta Y
		pa;			// k¹t po³o¿enia gracza

ButtonKeys keys;	// uk³ad stanów klawiszy gracza
Sprite sprites[4];

float frame1, frame2, fps;	// klatki wykorzystane do renderowania

int map[] = {

	1,1,1,1,1,1,1,1,1,1,
	1,0,1,0,0,0,0,0,0,1,
	1,0,1,0,0,0,0,0,0,1,
	1,0,0,0,1,0,0,0,0,1,
	1,0,0,0,0,0,1,1,0,1,
	1,0,0,2,0,0,1,0,0,1,
	1,0,0,0,0,0,1,1,0,1,
	1,0,0,0,0,0,0,1,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,1,1,1,1,1,1,1,1,
};

int map_walls[] = {

	1,1,1,1,1,1,1,1,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,1,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,1,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,1,1,1,1,1,1,1,1,
};	// bloki œcian mapy

int map_floor[] = {

	1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,
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

void gm_drawSprite() {

	// sprawdzenie, czy gracz jest w zasiêgu interakcji ze spritem
	if (px < sprites[0].x + 30 && px > sprites[0].x - 30 && py < sprites[0].y + 30 && py > sprites[0].y - 30) {
		sprites[0].alive = false;
	}

	float spriteX = sprites[0].x - px;	// odleg³oœæ od gracza
	float spriteY = sprites[0].y - py;
	float spriteZ = sprites[0].z;

	float spriteCos = cos(degToRad(pa));	// obrót wzglêdem gracza
	float spriteSin = sin(degToRad(pa));

	float a = spriteY * spriteCos + spriteX * spriteSin;
	float b = spriteX * spriteCos - spriteY * spriteSin;
	spriteX = a; spriteY = b;		// pozycja w przestrzeni

	int width = WIDTH / 8;		// ??????
	int height = HEIGHT / 8;	// idk, ale tak musi byc

	float FOV = 108.0;

	spriteX = (spriteX * FOV / spriteY) + width / 2;
	spriteY = (spriteZ * FOV / spriteY) + height / 2;

	int scale = 32 * height / b;
	if (scale < 0)		scale = 0;
	if (scale > width)	scale = width;

	int x, y;

	float textureX = 0, textureY = 31;
	float textureXStep = 31.5 / (float)scale, textureYStep = 32.0/(float)scale;	// wysokosc tekstury przez skale

	for (x = spriteX - scale / 2; x < spriteX + scale / 2; x++) {
		textureY = 31;
		for (y = 0; y < scale; y++) {
			if (sprites[0].alive == 1 && x > 0 && x < 120 && b < rayDepth[x]) {

				int pixel = ((int)textureY * 32 + (int)textureX) * 3;
				int red		=	Amogus_01[pixel + 0];
				int green	=	Amogus_01[pixel + 1];
				int blue	=	Amogus_01[pixel + 2];
				if (!(red == 255 && green == 0 && blue == 255)) {
					glPointSize(8);
					glColor3ub(red, green, blue);
					glBegin(GL_POINTS);
					glVertex2i(x * 8, spriteY*8 - y*8);
					glEnd();
				}
				textureY -= textureYStep;
				if (textureY < 0) textureY = 0;
			}
		}
		textureX += textureXStep;
	}

	
}

// ---- Mapa 2D ----

void gm_drawMapIn2D() {

	// funkcja rysuj¹ca mapê w formacie 2D.
	/* 
		x,  y	->	po³o¿enie w uk³adzie mapy (x,y)
		xo, yo	->	offset x, y
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
	glVertex2i(px, py);
	glEnd();

	glLineWidth(3);
	glBegin(GL_LINES);
	glVertex2i(px, py);
	glVertex2i(px + pdx * 20, py + pdy * 20);
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

	rayA = fixAngle(pa + 30);		// promieñ z odstêpem 30deg

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

			rayX = (((int)px >> 6) << 6) + 64;      
			rayY = (px - rayX) * Tan + py; 
			xOff = 64; 
			yOff = -xOff * Tan; 
		}

		else if (cos(degToRad(rayA)) < -0.001) {		// sprawdŸ prawo
			rayX = (((int)px >> 6) << 6) - 0.0001; 
			rayY = (px - rayX) * Tan + py; 
			xOff = -64; 
			yOff = -xOff * Tan; }

		// równoleg³e do pionu
		else { 
			rayX = px; 
			rayY = py; 
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
				disV = cos(degToRad(rayA)) * (rayX - px) - sin(degToRad(rayA)) * (rayY - py);
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
			rayY = (((int)py >> 6) << 6) - 0.0001; 
			rayX = (py - rayY) * Tan + px; 
			yOff = -64; 
			xOff = -yOff * Tan; 
		}

		else if (sin(degToRad(rayA)) < -0.001) { 		// sprawdzenie dó³
			rayY = (((int)py >> 6) << 6) + 64;      
			rayX = (py - rayY) * Tan + px; 
			yOff = 64; 
			xOff = -yOff * Tan; 
		}

		// równoleg³e do poziomu
		else { 
			rayX = px; 
			rayY = py; 
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
				disH = cos(degToRad(rayA)) * (rayX - px) - sin(degToRad(rayA)) * (rayY - py);
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
		//glLineWidth(2); glBegin(GL_LINES); glVertex2i(px, py); glVertex2i(rx, ry); glEnd(); // rysowanie promienia

		int ca = fixAngle(pa - rayA); 
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
			int red		=	Textures[pixel + 0] * shade;
			int green	=	Textures[pixel + 1] * shade;
			int blue	=	Textures[pixel + 2] * shade;
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
			float raFix = cos(degToRad(fixAngle(pa - rayA)));

			float magicNum = 158 * 32 * 2.5;

			textureX = px / 2 + cos(deg) * magicNum / dy / raFix;
			textureY = py / 2 - sin(deg) * magicNum / dy / raFix;

			int mp = map_floor[(int)(textureY / 32.0) * mapX + (int)(textureX / 32.0)] * 32 * 32;

			int pixel = (((int)(textureY) & 31)*32 + ((int)textureX & 31)) * 3 + mp * 3;
			int red = Textures[pixel + 0] * 0.7;
			int green = Textures[pixel + 1] * 0.7;
			int blue = Textures[pixel + 2] * 0.7;

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

	px = 300; py = 300; pa = 0;
	pdx = cos(degToRad(pa));
	pdy = -sin(degToRad(pa));

	sprites[0].type = SPRITE_STATIC;
	sprites[0].alive = true;
	sprites[0].map = 0;
	sprites[0].x = 1.5 * 64;
	sprites[0].y = 5 * 64;
	sprites[0].z = 30;
}

void gm_displayResize(int width, int height) {

	glutReshapeWindow(WIDTH, HEIGHT);
}

enum ScreenType {
	SCR_TITLE = 1,
	SCR_GAME,
	SCR_LV_PASS,
	SCR_LV_FAIL
};

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

void gm_drawSkybox() {

	// aktualny obrazek ma wymiary
	// 120x80.
	int skyboxWidth = 120;
	int skyboxHeight = 80;
	int* texture = Skybox_01;
	for (int y = 0; y < skyboxHeight/1.5; y++) {
		for (int x = 0; x < skyboxWidth; x++) {

			int xOffset = (int)pa*2 - x;
			if (xOffset < 0) xOffset += skyboxWidth;
			xOffset %= skyboxWidth;


			int pixel = (y * skyboxWidth + xOffset) * 3;
			int red = texture[pixel + 0];
			int green = texture[pixel + 1];
			int blue = texture[pixel + 2];

			glPointSize(8);
			glColor3ub(red, green, blue);
			glBegin(GL_POINTS);
			glVertex2i(x * 8, y * 8);
			glEnd();
		}
	}
}

int gameState = 2, timer = 0;

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
	}
	if (gameState == 1) {	// okno g³ówne

		gm_screen(SCR_TITLE);
		timer += 1 * fps;
		if (timer > 3000) {
			timer = 0;
			gameState = 2;
		}
	}
	if (gameState == 2) {	// pêtla gry

		if (keys.a == 1) {
			pa += 0.2 * fps;
			pa = fixAngle(pa);
			pdx = cos(degToRad(pa));
			pdy = -sin(degToRad(pa));
		}
		if (keys.d == 1) {
			pa -= 0.2 * fps;
			pa = fixAngle(pa);
			pdx = cos(degToRad(pa));
			pdy = -sin(degToRad(pa));
		}

		// hitbox
		int xoff = 0, yoff = 0;
		xoff = pdx < 0 ? -20 : 20;
		yoff = pdy < 0 ? -20 : 20;

		// pozycja w siatce
		int ipx = px / 64.0;
		int ipx_add_off = (px + xoff) / 64.0;
		int ipx_sub_off = (px - xoff) / 64.0;

		int ipy = py / 64.0;
		int ipy_add_off = (py + yoff) / 64.0;
		int ipy_sub_off = (py - yoff) / 64.0;

		if (keys.s == 1) {
			//px -= pdx* 0.2 * fps;
			//py -= pdy * 0.2 * fps;
			if (map_walls[ipy * mapX + ipx_sub_off] == 0) {
				px -= pdx * 0.2 * fps;
			}
			if (map_walls[ipy_sub_off * mapX + ipx] == 0) {
				py -= pdy * 0.2 * fps;
			}
		}
		if (keys.w == 1) {

			if (map_walls[ipy * mapX + ipx_add_off] == 0) {
				px += pdx * 0.2 * fps;
			}
			if (map_walls[ipy_add_off * mapX + ipx] == 0) {
				py += pdy * 0.2 * fps;
			}

		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// wyœwietlanie 2D
		//gm_drawMapIn2D();
		//gm_drawPlayer();

		gm_drawSkybox();
		gm_castRays3D();
		gm_drawSprite();

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

	glutMainLoop();
	

	return 0;
}