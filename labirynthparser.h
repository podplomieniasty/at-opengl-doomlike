#include <json/json.h>
#include <iostream>
#include <fstream>
#include <vector>

using json = nlohmann::json;

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

GameSettings fetchLabirynth(int lvNum);