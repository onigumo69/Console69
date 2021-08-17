#pragma once
#include "Console69.h"

#include <string>
#include <algorithm>

class Space : public Console69
{
public:
	Space()
	{
		appName = L"Space";
	}

private:
	struct Entity
	{
		int size;
		float x;
		float y;
		float xv;
		float yv;
		float angle;
	};

	struct Vector2
	{
		Vector2() = default;
		Vector2(float x, float y)
			:
			x{ x }, y{ y }
		{}

		float x;
		float y;
	};

	Entity ship;
	const float speed = 5.0f;
	const float acceleration = 20.0f;
	bool dead = false;
	int score = 0;
	std::vector<Entity> rock;
	std::vector<Entity> bullets;

	std::vector<Vector2> shipModel;
	std::vector<Vector2> rockModel;

protected:
	virtual bool OnAwake() override
	{
		shipModel =
		{
			{  0.0f, -5.0f },
			{ -2.5f,  2.5f },
			{  2.5f,  2.5f }
		};

		// construct circle rock
		const int vertices = 20;
		for (int i = 0; i < vertices; ++i)
		{
			float noise = (float)rand() / (float)RAND_MAX * 0.4f + 0.8f;
			float randomX = noise * sinf(((float)i / (float)vertices) * 6.28318f);
			float randomY = noise * cosf(((float)i / (float)vertices) * 6.28318f);
			rockModel.push_back(Vector2(randomX, randomY));
		}

		ResetGame();

		return true;
	}

	virtual bool OnUpdate(float deltaTime) override
	{
		if (dead)
			ResetGame();

		// clear screen
		Fill(0, 0, GetScreenWidth(), GetScreenHeight(), Solid, BG_Black);

		// ship control
		if (keys[VK_LEFT].Hold)
			ship.angle -= speed * deltaTime;
		if (keys[VK_RIGHT].Hold)
			ship.angle += speed * deltaTime;
		if (keys[VK_UP].Hold)
		{
			ship.xv += sin(ship.angle) * acceleration * deltaTime;
			ship.yv += -cos(ship.angle) * acceleration * deltaTime;
		}

		// ship pos
		ship.x += ship.xv * deltaTime;
		ship.y += ship.yv * deltaTime;

		// check map boundary
		WrapCoordinates(ship.x, ship.y, ship.x, ship.y);

		// check ship collision
		for (auto& r : rock)
			if (IsPointInsideCircle(ship.x, ship.y, r.x, r.y, r.size))
				dead = true;

		// fire
		if (keys[VK_SPACE].Release)
			bullets.push_back({ 0, ship.x, ship.y, 50.0f * sinf(ship.angle),
				-50.0f * cosf(ship.angle), 100.0f });

		// draw rock
		for (auto& r : rock)
		{
			r.x += r.xv * deltaTime;
			r.y += r.yv * deltaTime;
			r.angle += 0.5f * deltaTime; // wiggle wiggle

			WrapCoordinates(r.x, r.y, r.x, r.y);

			DrawWireFrame(rockModel, r.x, r.y, r.angle, (float)r.size, FG_Cyan);
		}


		std::vector<Entity> collapsedRock;

		for (auto& b : bullets)
		{
			b.x += b.xv * deltaTime;
			b.y += b.yv * deltaTime;
			WrapCoordinates(b.x, b.y, b.x, b.y);
			b.angle -= 1.0f * deltaTime;

			for (auto& r : rock)
			{
				if (IsPointInsideCircle(b.x, b.y, r.x, r.y, r.size))
				{
					b.x = -100.0f;

					// collapse
					if (r.size > 4)
					{
						float angle1 = ((float)rand() / (float)RAND_MAX) * 6.283185f;
						float angle2 = ((float)rand() / (float)RAND_MAX) * 6.283185f;
						collapsedRock.push_back({ (int)r.size >> 1 , r.x, r.y, 10.0f * sinf(angle1), 10.0f * cosf(angle1), 0.0f });
						collapsedRock.push_back({ (int)r.size >> 1 , r.x, r.y, 10.0f * sinf(angle2), 10.0f * cosf(angle2), 0.0f });
					}

					r.x = -100.0f;
					score += 100;
				}
			}
		}

		for (auto& cr : collapsedRock)
		{
			rock.push_back(cr);
		}

		if (rock.size() > 0)
		{
			auto i = std::remove_if(rock.begin(), rock.end(),
				[&](Entity e) {return (e.x < 0); });

			if (i != rock.end())
				rock.erase(i);
		}

		if (rock.empty()) // next level
		{
			rock.clear();
			bullets.clear();

			// make sure don't spawn in ship's nearby
			rock.push_back(
				{
					(int)16, 30.0f * sinf(ship.angle - 3.14159f / 2.0f) + ship.x,
					30.0f * cosf(ship.angle - 3.14159f / 2.0f) + ship.y,
					10.0f * sinf(ship.angle), 10.0f * cosf(ship.angle), 0.0f
				}
			);

			rock.push_back(
				{
					(int)16, 30.0f * sinf(ship.angle - 3.14159f / 2.0f) + ship.x,
					30.0f * cosf(ship.angle + 3.14159f / 2.0f) + ship.y,
					10.0f * sinf(-ship.angle), 10.0f * cosf(-ship.angle), 0.0f
				}
			);
		}

		// remove offscreen bullets
		if (bullets.size() > 0)
		{
			auto i = std::remove_if(bullets.begin(), bullets.end(),
				[&](Entity e)
				{
					return (
						e.x < 1 || e.y < 1 ||
						e.x >= GetScreenWidth() - 1 ||
						e.y >= GetScreenHeight() - 1);
				}
			);

			if (i != bullets.end())
				bullets.erase(i);
		}

		// draw bullets
		for (auto& b : bullets)
			Draw(b.x, b.y);

		// draw ship
		DrawWireFrame(shipModel, ship.x, ship.y, ship.angle);

		// draw score
		score += 1000;
		DrawString(2, 2, L"Score: " + std::to_wstring(score));

		return true;
	}

	void ResetGame()
	{
		ship.x = GetScreenWidth() / 2.0f;
		ship.y = GetScreenHeight() / 2.0f;
		ship.xv = 0.0f;
		ship.yv = 0.0f;
		ship.angle = 0.0f;

		rock.clear();
		bullets.clear();

		rock.push_back({ (int)16, 20.0f, 20.0f, 8.0f, -6.0f, 0.0f });
		rock.push_back({ (int)16, 100.0f, 20.0f, -5.0f, 3.0f, 0.0f });

		dead = false;
		score = 0;
	}

	void WrapCoordinates(float ix, float iy, float& ox, float& oy)
	{
		ox = ix;
		oy = iy;
		const float width = (float)GetScreenWidth();
		const float height = (float)GetScreenHeight();

		if (ix < 0.0f)		ox = ix + width;
		if (ix >= width)	ox = ix - width;
		if (iy < 0.0f)		oy = iy + height;
		if (iy >= height)	oy = iy - height;
	}

	virtual void Draw(int x, int y, short cha = 0x2588, short col = 0x000F) override
	{
		float ox{};
		float oy{};
		WrapCoordinates(x, y, ox, oy);
		Console69::Draw(ox, oy, cha, col);
	}

	bool IsPointInsideCircle(float px, float py, float cx, float cy, float r)
	{
		const float distance = sqrt((px - cx) * (px - cx) + (py - cy) * (py - cy));
		return distance < r;
	}

	void DrawWireFrame(const std::vector<Vector2>& model, float x, float y, float r = 0.0f, float s = 1.0f, short col = FG_White)
	{
		std::vector<Vector2> transformed;
		const int vertices = model.size();
		transformed.resize(vertices);

		// rotate
		for (int i = 0; i < vertices; ++i)
		{
			transformed[i].x = model[i].x * cosf(r) - model[i].y * sinf(r);
			transformed[i].y = model[i].x * sinf(r) + model[i].y * cosf(r);
		}

		// scale
		for (int i = 0; i < vertices; ++i)
		{
			transformed[i].x = transformed[i].x * s;
			transformed[i].y = transformed[i].y * s;
		}

		// translate
		for (int i = 0; i < vertices; ++i)
		{
			transformed[i].x = transformed[i].x + x;
			transformed[i].y = transformed[i].y + y;
		}

		for (int i = 0; i < vertices; ++i)
		{
			int j = i + 1;
			DrawLine(
				transformed[i % vertices].x,
				transformed[i % vertices].y,
				transformed[j % vertices].x,
				transformed[j % vertices].y,
				Solid, col
			);
		}
	}
};