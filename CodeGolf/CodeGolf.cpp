// CodeGolf.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define OLC_PGE_APPLICATION
#define _CRT_SECURE_NO_WARNINGS

#include "olcPixelGameEngine.h"
#include "olcPGEX_Graphics2D.h"
#include <iostream>

#define M_PI 3.14159265358979323846

class Hole {
public:
	float x = 142, y = 23;
};

class BallPosition {
public:
	float x = 152, y = 220;
};

class Ball {
public:
	float x = 152, y = 220, z = 1;
	float startingPositionX = 152, startingPositionY = 220, startingPositionZ = 1;
	float xVel = 0, yVel = 0, zVel = 0;

	int imageWidth = 4, imageHeight = 4;

	bool isStill = true;
	olc::Sprite* spr;
	olc::Sprite* ghostSpr;
};

enum GroundType {
	ground_rough,
	ground_fairway,
	ground_green,
	ground_water
};

class Golf : public olc::PixelGameEngine {
public:
	Golf() {
		sAppName = "PGE Golf";
	}

	bool OnUserCreate() {
		ballObj.spr = new olc::Sprite("ball_tiny.png");
		ballObj.ghostSpr = new olc::Sprite("ball_ghost.png");
		holeSpr = new olc::Sprite("hole.png");
		flagSpr = new olc::Sprite("flag.png");

		strokeList = std::vector<int>(18);
		holeList = std::vector<Hole>(18);
		ballPList = std::vector<BallPosition>(18);

		holeList[0] = { 142 , 23 };
		holeList[1] = { 289 , 25 };
		holeList[2] = { 277 , 32 };
		holeList[3] = { 251 , 81 };
		holeList[4] = { 50  , 48 };

		ballPList[0] = {152, 220};
		ballPList[1] = {38, 201};
		ballPList[2] = {42, 194};
		ballPList[3] = {28, 225};
		ballPList[4] = {252, 48};

		LoadMap(1);

		srand(time(NULL));

		aimAngle = -M_PI / 2;

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) {

		/*UPDATE*/

		if(gameStatus == status_aim){
			if (GetKey(olc::Key::LEFT).bHeld) {
				aimAngle -= fElapsedTime * aimSpeed;
			}
			else if (GetKey(olc::Key::RIGHT).bHeld) {
				aimAngle += fElapsedTime * aimSpeed;
			}
			/*else if (GetKey(olc::Key::UP).bHeld) {
				putPower += fElapsedTime * putSpeed;
			}
			else if (GetKey(olc::Key::DOWN).bHeld) {
				putPower -= fElapsedTime * putSpeed;
			}*/
			else if (GetKey(olc::Key::SPACE).bPressed) {
				curPower = minPower;
				gameStatus = status_power;
				backwardsPower = false;
			}
		}
		else if (gameStatus == status_power) {

			if(!backwardsPower)
				curPower += fElapsedTime * 10;
			else
				curPower -= fElapsedTime * 10;

			if (curPower > maxPower) {
				backwardsPower = true;
			}
			else if (curPower < minPower) {
				gameStatus = status_point;
			}
			else {
				if (GetKey(olc::Key::SPACE).bPressed) {
					gameStatus = status_point;

					curAngle = maxAngle;
				}
			}
		}
		else if (gameStatus == status_point) {

			curAngle -= fElapsedTime;

			if (curAngle < minAngle) {
				Putt();
			}
			else {
				if (GetKey(olc::Key::SPACE).bPressed) {
					Putt();
				}
			}

		}

		if (GetKey(olc::Key::P).bPressed) {
			curHole++;
			LoadMap(curHole + 1);
		}

		/*PHYSICS*/

		/*AIR DRAG*/
		if (ballObj.z > 1) {
			//ballObj.xVel *= 1 - (fElapsedTime * airDrag);
			ballObj.xVel += cos(windDirection) * windSpeed * ballObj.z * fElapsedTime;
			//ballObj.yVel *= 1 - (fElapsedTime * airDrag);
			ballObj.yVel += sin(windDirection) * windSpeed * ballObj.z * fElapsedTime;
		}
		else {
			ballObj.z = 1;

			ballObj.xVel *= 1 - (fElapsedTime * drag);
			ballObj.yVel *= 1 - (fElapsedTime * drag);

			GroundType curPosTile = GetMapAt(ballObj.x, ballObj.y);

			if (ballObj.x > holeList[curHole].x - 2.5 && ballObj.x < holeList[curHole].x + 2.5 &&
				ballObj.y > holeList[curHole].y - 2.5 && ballObj.y < holeList[curHole].y + 2.5) {
				curHole++;
				LoadMap(curHole + 1);
			}

			if (curPosTile == ground_water) {
				ballObj.x = ballObj.startingPositionX;
				ballObj.y = ballObj.startingPositionY;
				ballObj.xVel = 0;
				ballObj.yVel = 0;
			}
			else {
				float dragSpeed = 0;

				switch (curPosTile) {
				ground_rough:
					dragSpeed = roughDrag;
					break;
				ground_fairway:
					dragSpeed = fairwayDrag;
					break;
				ground_green:
					dragSpeed = greenDrag;
				}

				ballObj.xVel *= 1 - (dragSpeed * fElapsedTime);
				ballObj.yVel *= 1 - (dragSpeed * fElapsedTime);
			}
		}

		if(ballObj.z > 1)
			ballObj.zVel -= 0.5f * fElapsedTime;

		ballObj.x += ballObj.xVel * fElapsedTime;
		ballObj.y += ballObj.yVel * fElapsedTime;
		ballObj.z += ballObj.zVel * fElapsedTime;

		if (ballObj.xVel < 0.5f && ballObj.xVel > -0.5f && ballObj.yVel < 0.5f && ballObj.yVel > -0.5f && ballObj.z == 1.f && !ballObj.isStill) {
			ballObj.isStill = true;
			ballObj.xVel = 0;
			ballObj.yVel = 0;

			srand(time(NULL));
			windDirection = rand();
			windSpeed = rand() % 15;

			gameStatus = status_aim;

			ballObj.startingPositionX = ballObj.x;
			ballObj.startingPositionY = ballObj.y;
		}

		if (ballObj.z < 1) {
			ballObj.z = 1;
			ballObj.zVel = 0;
			/*ballObj.xVel = 0;
			ballObj.yVel = 0;*/
		}

		/*RENDER*/

		SetPixelMode(olc::Pixel::NORMAL);
		Clear(olc::CORNFLOWER_BLUE);

		DrawSprite(0, 0, bgSpr, 1);

		if (ballObj.isStill && gameStatus == status_aim) {
			DrawLine(ballObj.x, ballObj.y, ballObj.x + (cos(aimAngle) * aimLength * putPower), ballObj.y + (sin(aimAngle) * aimLength * putPower));
			DrawLine(ballObj.x, ballObj.y, ballObj.x + (cos(aimAngle) * aimLength * curPower), ballObj.y + (sin(aimAngle) * aimLength * curPower), olc::RED);
		}

		if (ballObj.x > holeList[curHole].x - 20 && ballObj.x < holeList[curHole].x + 20 &&
			ballObj.y > holeList[curHole].y - 20 && ballObj.y < holeList[curHole].y + 20){
			olc::GFX2D::Transform2D holeT2D;
			holeT2D.Translate(-2.5, -2.5);
			holeT2D.Translate(holeList[curHole].x, holeList[curHole].y);
			SetPixelMode(olc::Pixel::ALPHA);
			olc::GFX2D::DrawSprite(holeSpr, holeT2D);
			SetPixelMode(olc::Pixel::NORMAL);
		}
		else {
			olc::GFX2D::Transform2D holeT2D;
			holeT2D.Translate(-2.5f, -2.5f);
			holeT2D.Translate(holeList[curHole].x, holeList[curHole].y - 14);
			SetPixelMode(olc::Pixel::ALPHA);
			olc::GFX2D::DrawSprite(flagSpr, holeT2D);
			SetPixelMode(olc::Pixel::NORMAL);
		}

		DrawLine(18.5 - (cos(windDirection) * 7.5), 10.5 - (sin(windDirection) * 7.5), 18.5 + (cos(windDirection) * 7.5), 10.5 + (sin(windDirection) * 7.5));
		DrawCircle(18.5 + (cos(windDirection)) * 7.5, 10.5 + (sin(windDirection)) * 7.5, 1);
		DrawString(1, 17, std::to_string((int)windSpeed) + "kmph", olc::WHITE);

		switch (GetMapAt(ballObj.x, ballObj.y)) {
		case ground_rough:
			DrawString(1, mapHeight - 9, "ROUGH");
			break;
		case ground_fairway:
			DrawString(1, mapHeight - 9, "FAIRWAY");
			break;
		case ground_green:
			DrawString(1, mapHeight - 9, "GREEN");
			break;
		case ground_water:
			DrawString(1, mapHeight - 9, "WATER");
			break;
		}

		int powerIndicatorX = 182 - (curPower / maxPower) * 60;
		int angleIndicatorX = 200 - (curAngle / maxAngle) * 40;

		//DrawRect(121, 240 - 10, 80, 8);
		
		DrawLine(180, 240 - 5, powerIndicatorX, 240 - 5, olc::DARK_RED);
		DrawLine(180, 240 - 6, powerIndicatorX, 240 - 6, olc::DARK_RED);

		DrawLine(180, 240 - 5, angleIndicatorX, 240 - 5, olc::DARK_YELLOW);
		DrawLine(180, 240 - 6, angleIndicatorX, 240 - 6, olc::DARK_YELLOW);
		
		DrawLine(121, 240 - 7, 200, 240 - 7);
		DrawLine(121, 240 - 4, 200, 240 - 4);
		DrawLine(121, 240 - 8, 121, 240 - 3);
		DrawLine(200, 240 - 8, 200, 240 - 3);

		DrawLine(180, 240 - 9, 180, 238);
		DrawLine(160, 240 - 9, 160, 238);

		DrawLine(angleIndicatorX, 240 - 9, angleIndicatorX, 238, olc::YELLOW);
		DrawLine(powerIndicatorX, 240 - 9, powerIndicatorX, 238, olc::RED);

		if (GetKey(olc::Key::TAB).bHeld) {
			for (int i = 1; i <= 18; i++) {
				DrawString(230, (i * 8) + 5 * 8, "HOLE " + std::to_string(i) + ": ");
				DrawString(300, (i * 8) + 5 * 8, std::to_string(strokeList[i - 1]));
			}
		}


		DrawBall();

		return true;
	}

	void readBMPtoMap(std::string filename)
	{
		int i;
		FILE* f = fopen(filename.c_str(), "rb");

		if (f == NULL)
			throw "Argument Exception";

		unsigned char info[54];
		fread(info, sizeof(unsigned char), 54, f); // read the 54-byte header

		// extract image height and width from header
		int width = *(int*)&info[18];
		int height = *(int*)&info[22];

		int row_padded = (width * 3 + 3) & (~3);
		unsigned char* data = new unsigned char[row_padded];
		unsigned char tmp;

		map = new GroundType[width*height];

		int pos = 0;
		for (int i = 0; i < height; i++)
		{
			fread(data, sizeof(unsigned char), row_padded, f);
			for (int j = 0; j < width * 3; j += 3)
			{
				// Convert (B, G, R) to (R, G, B)
				tmp = data[j];
				data[j] = data[j + 2];
				data[j + 2] = tmp;

				//std::cout << (int)data[j] << ":" << (int)data[j + 1] << ":" << (int)data[j + 2] << "\n";

				if (data[j] == 255 && data[j + 1] == 255) {
					map[pos] = ground_rough;
				}
				else if (data[j] == 255) {
					map[pos] = ground_fairway;
				}
				else if (data[j + 1] == 255) {
					map[pos] = ground_green;
				}
				else if (data[j + 2] == 255) {
					map[pos] = ground_water;
				}
				pos++;
			}
		}
		delete[] data;
		fclose(f);
	}

	void LoadMap(int number) {
		if (number > maxHole + 1) {
			number = 1;
			curHole = 0;
		}

		if (bgSpr != NULL) 
			delete bgSpr;
		
		bgSpr = new olc::Sprite("DATA/Hole" + std::to_string(number) + ".png");

		if (map != NULL)
			delete[] map;

		readBMPtoMap("DATA/Hole" + std::to_string(number) + ".bmp");

		ballObj.x = ballPList[number - 1].x;
		ballObj.y = ballPList[number - 1].y;
		ballObj.xVel = 0;
		ballObj.yVel = 0;
		ballObj.zVel = 0;
		gameStatus = status_aim;

	}

	void DrawBall() {
		olc::GFX2D::Transform2D ghostPos;
		ghostPos.Translate(-ballObj.imageWidth / 2, -ballObj.imageHeight / 2);
		ghostPos.Translate(ballObj.x, ballObj.y);


		olc::GFX2D::Transform2D ballPos;
		ballPos.Translate(-ballObj.imageWidth/2, -ballObj.imageHeight/2);
		ballPos.Scale(ballObj.z, ballObj.z);
		ballPos.Translate(ballObj.x, ballObj.y - ((ballObj.z - 1) * zDifference));

		SetPixelMode(olc::Pixel::ALPHA);

		olc::GFX2D::DrawSprite(ballObj.ghostSpr, ghostPos);
		olc::GFX2D::DrawSprite(ballObj.spr, ballPos);
		SetPixelMode(olc::Pixel::NORMAL);
	}

	GroundType GetMapAt(int x, int y) {
		if (x > -1 && x < mapWidth && y > -1 && y < mapHeight) {
			return map[(int)(x) + (mapWidth * (mapHeight - y))];
		}
		else {
			return ground_rough;
		}
	}

	void Putt() {
		ballObj.zVel = (curPower/35) * sin(curAngle);
		ballObj.xVel = cos(curAngle) * curPower * cos(aimAngle);
		ballObj.yVel = cos(curAngle) * curPower * sin(aimAngle);
		ballObj.isStill = false;

		putPower = 35.f;

		curPower = minPower;
		curAngle = maxAngle;

		strokeList[curHole]++;

		gameStatus = status_standby;
	}

private:

	Ball ballObj;
	olc::Sprite* bgSpr = NULL;
	olc::Sprite* holeSpr = NULL;
	olc::Sprite* flagSpr = NULL;

	float maxPower = 40.f;
	float minPower = 1.f;
	float maxAngle = M_PI / 2;
	float minAngle = 0;
	float curPower = minPower;
	float curAngle = maxAngle;
	bool backwardsPower = false;

	float zDifference = 10.f;

	float aimAngle = 0.f;
	float aimLength = 1.f;
	float aimSpeed = 1.f;
	float putPower = 35.f;
	float putSpeed = 30.f;
	float putAngle = M_PI/4;

	float airDrag = 0.25f;
	float drag = 0.8f;
	float roughDrag = 1.5f;
	float fairwayDrag = 0.8f;
	float greenDrag = 0.45f;

	float windDirection = 0.3f;
	float windSpeed = 3.f;

	int mapWidth = 320, mapHeight = 240;

	GroundType* map = NULL;
	enum GameStatus {
		status_aim,
		status_power,
		status_point,
		status_standby
	} gameStatus = status_aim;

	std::vector<int> strokeList;
	std::vector<Hole> holeList;
	std::vector<BallPosition> ballPList;
	int curHole = 0;
	int maxHole = 4;
};

int main()
{

	Golf game;
	if (game.Construct(320, 240, 2, 2))
		game.Start();
	else
		std::wcout << L"Failed to construct console." << std::endl;

	return 0;
}