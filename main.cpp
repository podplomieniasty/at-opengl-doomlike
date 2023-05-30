// ---- Biblioteki ----

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <math.h>



// ---- Tekstury ----

#include "textures/Bricks_01.ppm"
#include "textures/Planks_01.ppm"
#include "textures/Skybox_01.ppm"


// ---- Sta³e ----

#define PI		3.1415926535

#define P2		PI/2			
#define P3		3*PI/2
#define DEG		0.0174533	// stopieñ w radianach

#define WIDTH	1024
#define HEIGHT	768
#define TITLE	"Raycasting - Bialas, Burzec i Kijowski"

#define MAP_HEIGHT	320

#define mapX		10	// szerokoœæ mapy
#define mapY		10	// wysokoœæ mapy
#define mapS		64  // rozmiar kostki mapy



// ---- Dodatkowe funkcje ----

float degToRad(float a) { return a * PI / 180.0; };
float fixAngle(float a) { if (a > 359) a -= 360; if (a < 0) a += 360; return a; };
float distance(float ax, float ay, float bx, float by, float angle) { return cos(degToRad(angle)) * (bx - ax) - sin(degToRad(angle)) * (by - ay); };



// ---- Struktury ----

typedef struct {

	int w, a, s, d;// klawisze ze stanami on/off

} ButtonKeys;



// ---- Zmienne globalne ----

float	px,			// pozycja X gracza
		py,			// pozycja Y gracza
		pdx,		// delta X
		pdy,		// delta Y
		pa;			// k¹t po³o¿enia gracza

ButtonKeys keys;	// uk³ad stanów klawiszy gracza

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
	1,0,1,0,0,0,0,0,0,1,
	1,0,1,1,1,0,0,0,0,1,
	1,0,0,0,1,0,0,0,0,1,
	1,0,1,0,0,0,0,0,0,1,
	1,0,1,1,1,1,1,1,0,1,
	1,0,0,0,0,1,0,0,0,1,
	1,0,1,0,0,0,0,0,1,1,
	1,0,1,0,0,0,1,0,0,1,
	1,1,1,1,1,1,1,1,1,1,
};	// bloki œcian mapy

int map_floor[] = {

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

	int r, mx, my, mp, dof, side; 
	float vx, vy, rx, ry, ra, xo, yo, disV, disH;

	ra = fixAngle(pa + 30);		// promieñ z odstêpem 30deg

	for (r = 0; r < 120; r++)
	{
		int vmt = 0, hmt = 0;

		//--- LINIE PIONOWE --- 
		dof = 0; side = 0; disV = 100000;
		float Tan = tan(degToRad(ra));

		// sprawdzenie lewo
		if (cos(degToRad(ra)) > 0.001) { 
			rx = (((int)px >> 6) << 6) + 64;      
			ry = (px - rx) * Tan + py; 
			xo = 64; 
			yo = -xo * Tan; 
		}

		// sprawdzenie prawo
		else if (cos(degToRad(ra)) < -0.001) { 
			rx = (((int)px >> 6) << 6) - 0.0001; 
			ry = (px - rx) * Tan + py; 
			xo = -64; 
			yo = -xo * Tan; }

		// równoleg³e do pionu
		else { 
			rx = px; 
			ry = py; 
			dof = 8; 
		}

		while (dof < 8)
		{
			mx = (int)(rx) >> 6; 
			my = (int)(ry) >> 6; 
			mp = my * mapX + mx;

			// sprawdzenie, czy nachodzimy na œcianê
			if (mp > 0 && mp < mapX * mapY && map_walls[mp] > 0) {
				vmt = map_walls[mp] - 1;
				dof = 8; 
				disV = cos(degToRad(ra)) * (rx - px) - sin(degToRad(ra)) * (ry - py);
			}         
			else {
				rx += xo; 
				ry += yo; 
				dof += 1; 
			}
		}

		vx = rx; 
		vy = ry;

		//--- POZIOME LINIE ---

		dof = 0; disH = 100000;
		Tan = 1.0 / Tan;

		// sprawdzenie góra
		if (sin(degToRad(ra)) > 0.001) { 
			ry = (((int)py >> 6) << 6) - 0.0001; 
			rx = (py - ry) * Tan + px; 
			yo = -64; 
			xo = -yo * Tan; 
		}

		// sprawdzenie dó³
		else if (sin(degToRad(ra)) < -0.001) { 
			ry = (((int)py >> 6) << 6) + 64;      
			rx = (py - ry) * Tan + px; 
			yo = 64; 
			xo = -yo * Tan; 
		}

		// równoleg³e do poziomu
		else { 
			rx = px; 
			ry = py; 
			dof = 8; 
		} 

		while (dof < 8)
		{
			mx = (int)(rx) >> 6; 
			my = (int)(ry) >> 6; 
			mp = my * mapX + mx;
			if (mp > 0 && mp < mapX * mapY && map_walls[mp] > 0) {
				hmt = map_walls[mp] - 1;
				dof = 8; 
				disH = cos(degToRad(ra)) * (rx - px) - sin(degToRad(ra)) * (ry - py);
			}        
			else { 
				rx += xo; 
				ry += yo; 
				dof += 1; 
			} 
		}

		float shade = 1;
		glColor3f(0, 0.8, 0);
		if (disV < disH) {
			hmt = vmt;
			shade = 0.5;
			rx = vx; 
			ry = vy; 
			disH = disV; 
			glColor3f(0, 0.6, 0);
		}
		//glLineWidth(2); glBegin(GL_LINES); glVertex2i(px, py); glVertex2i(rx, ry); glEnd(); // rysowanie promienia

		int ca = fixAngle(pa - ra); 
		disH = disH * cos(degToRad(ca));
		int lineH = (mapS * HEIGHT) / (disH);

		float ty_step = 32.0 / (float)lineH;
		float ty_off = 0;

		if (lineH > HEIGHT) {
			ty_off = (lineH - HEIGHT)/2.0;
			lineH = HEIGHT;

		}
		int lineOff = HEIGHT/2 - (lineH >> 1);

		// rysowanie œcian

		float textureY = ty_off * ty_step; // hmt * 32;
		float textureX;
		if (shade == 1) {
			textureX = (int)(rx / 2.0) % 32;
			if (ra > 180) textureX = 31 - textureX;
		}
		else {
			textureX = (int)(ry / 2.0) % 32;
			if (ra > 90 && ra < 180) textureX = 31 - textureX;
		}

		int* texture;
		if (map_walls[mp] == 1)			texture = Bricks_01;
		else if (map_walls[mp] == 2)		texture = Bricks_01;
		else							texture = Bricks_01;

		for (int y = 0; y < lineH; y++) {
			int pixel	=	((int)textureY * 32 + (int)textureX) * 3;
			int red		=	texture[pixel + 0] * shade;
			int green	=	texture[pixel + 1] * shade;
			int blue	=	texture[pixel + 2] * shade;
			glPointSize(8);
			glColor3ub(red, green, blue);
			glBegin(GL_POINTS);
			glVertex2i(r * 8, y + lineOff);
			glEnd();
			textureY += ty_step;
		}


		// rysowanie pod³ogi

		if (map_floor[mp] == 0)	texture = Planks_01;
		else					texture = Bricks_01;

		for (int y = lineOff + lineH; y < HEIGHT; y++) {

			

			float dy = y - (HEIGHT / 2.0);
			float deg = degToRad(ra);
			float raFix = cos(degToRad(fixAngle(pa - ra)));
			int magicNum = 158 * 32*2.5;
			textureX = px / 2 + cos(deg) * magicNum / dy / raFix;
			textureY = py / 2 - sin(deg) * magicNum / dy / raFix;

			int mp = map_floor[(int)(textureY / 32.0)] * mapX + (int)(textureX / 32.0);

			int pixel = (((int)(textureY) & 31)*32 + ((int)textureX & 31)) * 3;
			int red = texture[pixel + 0];
			int green = texture[pixel + 1];
			int blue = texture[pixel + 2];
			glPointSize(8);
			glColor3ub(red, green, blue);
			glBegin(GL_POINTS);
			glVertex2i(r * 8, y);
			glEnd();
		}

		

		ra = fixAngle(ra - 0.5); // kolejny promieñ
	}
}










// ---- Ekran ----

void gm_displayResize(int width, int height) {

	glutReshapeWindow(WIDTH, HEIGHT);
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

void gm_displayFunc() {

	// tutaj s¹ konfiguracje odnoœnie
	// renderowanego okienka w openglu

	frame2 = glutGet(GLUT_ELAPSED_TIME);
	fps = (frame2 - frame1);
	frame1 = glutGet(GLUT_ELAPSED_TIME);


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
		if (map_walls [ipy * mapX + ipx_sub_off] == 0) {
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
	glutPostRedisplay();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// wyœwietlanie 2D
	//gm_drawMapIn2D();
	//gm_drawPlayer();

	gm_drawSkybox();
	gm_castRays3D();
	


	glutSwapBuffers();
}

void gm_init() {

	glClearColor(0.3, 0.3, 0.3, 0);
	gluOrtho2D(0, WIDTH, HEIGHT, 0);

	px = 300; py = 300; pa = 90;
	pdx = cos(degToRad(pa));
	pdy = -sin(degToRad(pa));
}

int main(int argc, char* argv[]) {

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(WIDTH, HEIGHT);
	glutCreateWindow(TITLE);
	gm_init();

	glutDisplayFunc(gm_displayFunc);
	glutReshapeFunc(gm_displayResize);

	glutKeyboardFunc(gm_handleButtonDown);
	glutKeyboardUpFunc(gm_handleButtonUp);

	glutMainLoop();
	

	return 0;
}