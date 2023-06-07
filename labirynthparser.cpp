#include "labirynthparser.h"

GameSettings fetchLabirynth(int lvNum) {
	std::string filename = "levels/levels.json";

	std::ifstream file(filename);
	json data = json::parse(file);

	GameSettings settings;

	if (lvNum <= 0) {
		std::cout << "Nie ma takiego poziomu!";
		return settings;
	}

	settings.level = lvNum;
	settings.width = data["levels"][lvNum-1]["width"];
	settings.height = data["levels"][lvNum - 1]["height"];
	settings.posX = data["levels"][lvNum - 1]["startX"];
	settings.posY = data["levels"][lvNum - 1]["startY"];

	size_t size = settings.width * settings.height;

	int* walls = new int[size];
	int* floor = new int[size];

	for (int x = 0; x < size; x++) {
		
		int a = data["levels"][lvNum - 1]["walls"][x];
		int b = data["levels"][lvNum - 1]["floor"][x];
		walls[x] = a;
		floor[x] = b;
	}
	
	settings.walls = walls;
	settings.floor = floor;

	settings.enemies = data["levels"][lvNum - 1]["enemies"];
	settings.chasers = data["levels"][lvNum - 1]["chasers"];
	settings.exitX = data["levels"][lvNum - 1]["exitX"];
	settings.exitY = data["levels"][lvNum - 1]["exitY"];

	return settings;
}
