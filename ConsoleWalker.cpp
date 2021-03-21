#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>

#include <Windows.h>

const int screenWidth = 120;
const int screenHeight = 40;

float playerX = 8.0f, playerY = 8.0f;
float playerAngle = 0;

float FOV = 3.14159f / 4.0;
float depth = 16.0f;

int mapHeight = 16;
int mapWidth = 16;

int main() {

	wchar_t* screen = new wchar_t[screenHeight * screenWidth];
	HANDLE console = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(console);
	DWORD bytesWritten = 0;

	std::wstring map;

	map += L"################";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"################";

	auto time1 = std::chrono::system_clock::now();
	auto time2 = std::chrono::system_clock::now();

	//Main game loop
	while (true) {

		time2 = std::chrono::system_clock::now();
		std::chrono::duration<float> elapsedTime = time2 - time1;
		time1 = time2;
		float ElapsedTime = elapsedTime.count();

		if (GetAsyncKeyState((unsigned short)'A') & 0x8000) {
			playerAngle -= (0.9) * ElapsedTime;
		}
		if (GetAsyncKeyState((unsigned short)'D') & 0x8000) {
			playerAngle += (0.9) * ElapsedTime;
		}

		if (GetAsyncKeyState((unsigned short)'W') & 0x8000) {
			playerX += std::sinf(playerAngle) * 5.0f * ElapsedTime;
			playerY += std::cosf(playerAngle) * 5.0f * ElapsedTime;

			if (map[(int)playerY * mapWidth + (int)playerX] == '#') {
				playerX -= std::sinf(playerAngle) * 5.0f * ElapsedTime;
				playerY -= std::cosf(playerAngle) * 5.0f * ElapsedTime;
			}
		}
		if (GetAsyncKeyState((unsigned short)'S') & 0x8000) {
			playerX -= std::sinf(playerAngle) * 5.0f * ElapsedTime;
			playerY -= std::cosf(playerAngle) * 5.0f * ElapsedTime;

			if (map[(int)playerY * mapWidth + (int)playerX] == '#') {
				playerX += std::sinf(playerAngle) * 5.0f * ElapsedTime;
				playerY += std::cosf(playerAngle) * 5.0f * ElapsedTime;
			}
		}

		for (int x = 0; x < screenWidth; x++) {
			//Calculate ray angle projected into world
			float rayAngle = (playerAngle - FOV / 2.0f) + ((float)x / (float)screenWidth) * FOV;


			float distanceToWall = 0;
			bool hitWall = false;
			bool boundary = false;

			float eyeX = std::sinf(rayAngle); //Unit vector for ray in player space
			float eyeY = std::cosf(rayAngle);

			while (!hitWall && distanceToWall < depth) {
				distanceToWall += 0.1f;

				int testX = (int)(playerX + eyeX * distanceToWall);
				int testY = (int)(playerY + eyeY * distanceToWall);

				//Check if ray is out of bounds
				//we'll set the maximum distance
				if (testX < 0 || testX >= mapWidth || testY < 0 || testY >= mapHeight) {
					hitWall = true;
					distanceToWall = depth;
				}
				else {
					//Ray is in bounds and is 'hitting' a wall
					if (map[testY * mapWidth + testX] == '#') {
						hitWall = true;

						std::vector<std::pair<float, float>> p;	//distance, dot product

						for (int tx = 0; tx < 2; tx++) {
							for (int ty = 0; ty < 2; ty++) {

								float vector_y = (float)testY + ty - playerY;
								float vector_x = (float)testX + tx - playerX;
								//magnitude of vector
								float d = sqrt(vector_x * vector_x + vector_y * vector_y);
								//dot product
								float dot = (eyeX * vector_x / d) + (eyeY * vector_y / d);
								p.push_back(std::make_pair(d, dot));
							}

							std::sort(p.begin(), p.end(), [](const std::pair<float, float>&left,const std::pair<float, float>& right) {return left.first < right.first; });

							float bound = 0.01f;

							//vectors are sorted, so..
							if (std::acos(p.at(0).second) < bound) boundary = true;
							if (std::acos(p.at(1).second) < bound) boundary = true;

						}
					}
				}
			}

			//Calculating Distance to ceiling and floor
			int ceiling = (float)(screenHeight / 2.0f) - screenHeight / ((float)distanceToWall);
			int floor = screenHeight - ceiling;

			short shade = ' ';

			//look at extended ascii-table

			if (distanceToWall <= depth / 4.0f)		shade = 0x2588; // Very close
			else if (distanceToWall < depth / 3.0f)		shade = 0x2593;
			else if (distanceToWall < depth / 2.0f)		shade = 0x2592;
			else if (distanceToWall < depth)		shade = 0x2591;
			else						shade = ' ';		// too far

			if (boundary)	shade = ' ';

			for (int y = 0; y < screenHeight; y++) {

				if (y <= ceiling) {
					screen[y * screenWidth + x] = ' ';
				}
				else if (y > ceiling && y <= floor) {
					screen[y * screenWidth + x] = shade;
				}
				else {
					//shading floor based on distance
					float b = 1.0f - (((float)y - screenHeight / 2.0f) / ((float)screenHeight / 2.0f));
					if (b < 0.25f)		shade = '#';
					else if (b < 0.5f)	shade = 'x';
					else if (b < 0.75f)	shade = '.';
					else if (b < 0.9f)	shade = '-';
					else			shade = ' ';

					screen[y * screenWidth + x] = shade;
				}
			}
		}

		//Display stats
		swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f, FPS=%3.2f", playerX, playerY, playerAngle, 1.0f/ElapsedTime);

		//Display map
		for (int nx = 0; nx < mapWidth; nx++) {
			for (int ny = 0; ny < mapWidth; ny++) {
				screen[(ny + 1) * screenWidth + nx] = map[ny * mapWidth + nx];
			}
		}

		screen[(int)playerY * screenWidth + (int)playerX] = 'P';

		screen[screenHeight * screenWidth - 1] = '\0';
		WriteConsoleOutputCharacter(console, screen, screenHeight * screenWidth, { 0,0 }, &bytesWritten);
	}

}
