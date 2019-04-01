#include "pch.h"
#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <chrono>
using namespace std;

#include <stdio.h>
#include <Windows.h>

using namespace std;

// Map size and render
int nScreenWidth = 120;
int nScreenHeight = 40;
int nMapHeight = 16; // Map display height
int nMapWidth = 16; // Map display width

// Player Movement / Position and Raycasting
float fPlayerX = 14.7f; // Floating point for the Player's X position.
float fPlayerY = 5.09f; // Floating point for the Player's Y position.
float fPlayerA = 0.0f; // Floating point for the Angle the Player's looking at.
float fFOV = 3.14159f / 4.0f; // Player's field of view
float fDepth = 16.0f; // Maximum depth to search for a wall
float fSpeed = 4.0f; // Player move speed

int main()
{
	// Create Screen Buffer
	wchar_t *screen = new wchar_t[nScreenWidth * nScreenHeight];
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;

	// Create the map
	wstring map;
	map += L"################";
	map += L"#..............#";
	map += L"#..##########..#";
	map += L"#......##......#";
	map += L"#......##......#";
	map += L"#......##......#";
	map += L"#.......#......#";
	map += L"###.....#......#";
	map += L"##.............#";
	map += L"#.#....####..###";
	map += L"#.#............#";
	map += L"#.#............#";
	map += L"#.#............#";
	map += L"#.####.#########";
	map += L"#..............#";
	map += L"################";

	auto tp1 = chrono::system_clock::now();
	auto tp2 = chrono::system_clock::now();


	// Game Loop
	while (true)
	{
		// Get time differential per frame to calculate modification
		// to the movement speed for a constant movement
		tp2 = chrono::system_clock::now();
		chrono::duration<float> elapsedTime = tp2 - tp1;
		tp1 = tp2;
		float fElapsedTime = elapsedTime.count();

		// Controls
		// Handle CCW Rotation
		if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
			fPlayerA -= (fSpeed * 0.5f) * fElapsedTime; // Rotate the player to the left

		if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
			fPlayerA += (fSpeed * 0.5f) * fElapsedTime; // Rotate the player to the right

		if (GetAsyncKeyState((unsigned short)'W') & 0x8000) {
			fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime; // Move the player forwards on the X
			fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime; // Move the player forwards on the Y

			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#') {
				fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime;
				fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime;
			}
		}

		if (GetAsyncKeyState((unsigned short)'S') & 0x8000) {
			fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime; // Move the player backwards on the X
			fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime; // Move the player backwards on the Y

			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#') {
				fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime;
				fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime;
			}
		}

		for (int x = 0; x < nScreenWidth; x++) {
			// For each column, claculate the projected ray angle into world space
			float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV;

			float fStepSize = 0.1f; // increment size for the ray casting
			float fDistanceToWall = 0.0f; // Gets the distance from the player to the wall
			bool bHitWall = false; // Saves if the ray cast has hit a wall
			bool bBoundary = false; // Saves if we've hit a boundary

			float fEyeX = sinf(fRayAngle); // Unit vector for ray in player space
			float fEyeY = cosf(fRayAngle);

			// Checks for a wall hit
			while (!bHitWall && fDistanceToWall < fDepth)
			{
				fDistanceToWall += fStepSize;

				int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
				int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

				// Test if ray is out of bounds
				if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight) {
					// Hit wall to true and set distance to be the maximum depth
					bHitWall = true;
					fDistanceToWall = fDepth;
				}
				else {
					// Ray is inbounds so test to see if the ray cell is a wall block
					if (map[nTestY * nMapWidth + nTestX] == '#') {
						bHitWall = true; // Hit wall to true

						vector<pair<float, float>> p; // distance, dot

						for (int tx = 0; tx < 2; tx++) {
							for (int ty = 0; ty < 2; ty++) {
								float vy = (float)nTestY + ty - fPlayerY;
								float vx = (float)nTestX + tx - fPlayerX;
								float d = sqrt(vx * vx + vy * vy);
								float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
								p.push_back(make_pair(d, dot));
							}
						}

						// Sort Pairs from closest to farthest
						sort(p.begin(), p.end(), [](const pair<float, float> &left, const pair<float, float> &right)
						{return left.first < right.first; });

						float fBound = 0.005f;
						if (acos(p.at(0).second) < fBound)
							bBoundary = true;
						if (acos(p.at(1).second) < fBound)
							bBoundary = true;
						if (acos(p.at(2).second) < fBound)
							bBoundary = true;
					}
				}
			}

			// Calculate distance to ceiling and floor to emulate a 3d ambient
			int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
			int nFloor = nScreenHeight - nCeiling;

			short nShade = ' '; // Will store the value of what char to print based on the distance

			// Run the array to check if we're drawing a wall a ceiling or the floor
			for (int y = 0; y < nScreenHeight; y++) {
				if (y < nCeiling) {
					// Shade the ceiling based on distance
					float c = 1.0f + (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
					if (c < 0.25)
						nShade = '#';
					else if (c < 0.5)
						nShade = 'x';
					else if (c < 0.75)
						nShade = '.';
					else if (c < 0.9)
						nShade = '-';
					else
						nShade = ' ';

					screen[y * nScreenWidth + x] = nShade;
				}
				else if (y > nCeiling && y <= nFloor) {
					// Shade the walls based on the distance
					if (fDistanceToWall <= fDepth / 4.0f)
						nShade = 0x2588; // Very Close
					else if (fDistanceToWall < fDepth / 3.0f)
						nShade = 0x2593;
					else if (fDistanceToWall < fDepth / 2.0f)
						nShade = 0x2592;
					else if (fDistanceToWall < fDepth)
						nShade = 0x2591;
					else
						nShade = ' '; // To far away

					if (bBoundary)
						nShade = ' '; // Black out the wall on the corners

					screen[y * nScreenWidth + x] = nShade;
				}
				else {
					// Shade the floor also based on distance
					float f = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
					if (f < 0.25)
						nShade = '#';
					else if (f < 0.5)
						nShade = 'x';
					else if (f < 0.75)
						nShade = '.';
					else if (f < 0.9)
						nShade = '-';
					else
						nShade = ' ';
						
					screen[y * nScreenWidth + x] = nShade;
				}
					
			}
		}

		// Display stats
		swprintf_s(screen, 50, L"X=%3.2f, Y=%3.2f, A=%3.2f, FPS=%3.2f ", fPlayerX, fPlayerY, fPlayerA, (1.0f / fElapsedTime));

		// Display minimap
		for (int nx = 0; nx < nMapWidth; nx++) {
			for (int ny = 0; ny < nMapWidth; ny++) {
				screen[(ny + 1) * nScreenWidth + nx] = map[ny * nMapWidth + nx];
			}
		}

		// Display a P on the map where the player is currently standing
		screen[((int)fPlayerY + 1) * nScreenWidth + (int)fPlayerX] = 'P';

		screen[nScreenWidth * nScreenHeight - 1] = '\0'; // Sets the last char of the array to stop writing
		// Allows for us to always start writing from the top left cornor of the console.
		WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
	}

	return 0;
}