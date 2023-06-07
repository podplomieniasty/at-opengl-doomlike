#include <json/json.h>
#include <iostream>
#include <fstream>
#include <vector>


/**
	Zaimportowana biblioteka do obs�ugi jsona.
*/
using json = nlohmann::json;

/**
	Struktura przechowuj�ca dane o aktualnym uk�adzie pomieszcze�.
	Wa�ne pozycje:
	int posX, posY  -  pozycje startowe gracza.
	int exitX, exitY  -  pozycje wyj�cia. -1 aby by�y losowo generowane.
*/
typedef struct GameSettings {

	int level = 0;
	int width = 0;
	int height = 0;
	int posX = 0;
	int posY = 0;
	int* walls = nullptr;
	int* floor = nullptr;
	int enemies = 0;
	int chasers = 0;
	int exitX = 0;
	int exitY = 0;
};

/**
	Funkcja obs�uguj�ca pobranie poziomu o indeksie lvNum.
	@param[in] int lvNum  -  indeks poziomu. Poziomy indeksowane od 1.
	@return Nowy obiekt GameSettings z uzupe�nionymi danymi.
*/
GameSettings fetchLabirynth(int lvNum);