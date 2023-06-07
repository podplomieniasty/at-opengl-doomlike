#include <json/json.h>
#include <iostream>
#include <fstream>
#include <vector>


/**
	Zaimportowana biblioteka do obs³ugi jsona.
*/
using json = nlohmann::json;

/**
	Struktura przechowuj¹ca dane o aktualnym uk³adzie pomieszczeñ.
	Wa¿ne pozycje:
	int posX, posY  -  pozycje startowe gracza.
	int exitX, exitY  -  pozycje wyjœcia. -1 aby by³y losowo generowane.
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
	Funkcja obs³uguj¹ca pobranie poziomu o indeksie lvNum.
	@param[in] int lvNum  -  indeks poziomu. Poziomy indeksowane od 1.
	@return Nowy obiekt GameSettings z uzupe³nionymi danymi.
*/
GameSettings fetchLabirynth(int lvNum);