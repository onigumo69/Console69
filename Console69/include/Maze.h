#pragma once
#include "Console69.h"

#include <stack>

class Maze : public Console69
{
public:
	Maze()
	{
		appName = L"Maze";
	}

private:
	int width;
	int height;
	int* maze;

	enum Cell
	{
		Cell_Path_N = 0x01,
		Cell_Path_E = 0x02,
		Cell_Path_S = 0x04,
		Cell_Path_W = 0x08,
		Cell_Visited = 0x10,
	};

	int visitedCells;
	std::stack<std::pair<int, int>> stack;
	int pathWidth;

protected:
	virtual bool OnAwake() override
	{
		width = 40;
		height = 25;
		maze = new int[width * height];
		memset(maze, 0x00, width * height * sizeof(int));
		pathWidth = 3;

		int x = rand() % width;
		int y = rand() % height;
		stack.push(std::make_pair(x, y));
		maze[x + y * width] = Cell_Visited;
		visitedCells = 1;

		return true;
	}

	virtual bool OnUpdate(float deltaTime) override
	{
		Sleep(10);

		auto offset = [&](int x, int y)
		{
			return (stack.top().first + x) + (stack.top().second + y) * width;
		};

		// wikipedia research Kappa
		if (visitedCells < width * height)
		{
			std::vector<int> neighbours;

			// N
			if (stack.top().second > 0 && (maze[offset(0, -1)] & Cell_Visited) == 0)
				neighbours.push_back(0);
			// E
			if (stack.top().first < width - 1 && (maze[offset(1, 0)] & Cell_Visited) == 0)
				neighbours.push_back(1);
			// S
			if (stack.top().second < height - 1 && (maze[offset(0, 1)] & Cell_Visited) == 0)
				neighbours.push_back(2);
			// W
			if (stack.top().first > 0 && (maze[offset(-1, 0)] & Cell_Visited) == 0)
				neighbours.push_back(3);

			if (!neighbours.empty())
			{
				int next_cell_dir = neighbours[rand() % neighbours.size()];

				// create a path between the neighbour and the current cell
				switch (next_cell_dir)
				{
				case 0: // N
					maze[offset(0, -1)] |= Cell_Visited | Cell_Path_S;
					maze[offset(0, 0)] |= Cell_Path_N;
					stack.push(std::make_pair((stack.top().first + 0), (stack.top().second - 1)));
					break;

				case 1: // E
					maze[offset(+1, 0)] |= Cell_Visited | Cell_Path_W;
					maze[offset(0, 0)] |= Cell_Path_E;
					stack.push(std::make_pair((stack.top().first + 1), (stack.top().second + 0)));
					break;

				case 2: // S
					maze[offset(0, +1)] |= Cell_Visited | Cell_Path_N;
					maze[offset(0, 0)] |= Cell_Path_S;
					stack.push(std::make_pair((stack.top().first + 0), (stack.top().second + 1)));
					break;

				case 3: // W
					maze[offset(-1, 0)] |= Cell_Visited | Cell_Path_E;
					maze[offset(0, 0)] |= Cell_Path_W;
					stack.push(std::make_pair((stack.top().first - 1), (stack.top().second + 0)));
					break;

				}

				++visitedCells;
			}
			else
			{
				// no available neighbours so backtrack
				stack.pop();
			}
		}

		// draw
		Fill(0, 0, GetScreenWidth(), GetScreenHeight(), L' ');

		for (int x = 0; x < width; ++x)
		{
			for (int y = 0; y < height; ++y)
			{
				for (int py = 0; py < pathWidth; ++py)
				{
					for (int px = 0; px < pathWidth; ++px)
					{
						if (maze[x + y * width] & Cell_Visited)
							Draw(x * (pathWidth + 1) + px, y * (pathWidth + 1) + py, Solid, FG_White);
						else
							Draw(x * (pathWidth + 1) + px, y * (pathWidth + 1) + py, Solid, FG_Blue);
					}
				}

				for (int p = 0; p < pathWidth; ++p)
				{
					if (maze[x + y * width] & Cell_Path_S)
						Draw(x * (pathWidth + 1) + p, y * (pathWidth + 1) + pathWidth);

					if (maze[x + y * width] & Cell_Path_E)
						Draw(x * (pathWidth + 1) + pathWidth, y * (pathWidth + 1) + p);
				}
			}
		}

		for (int py = 0; py < pathWidth; ++py)
			for (int px = 0; px < pathWidth; ++px)
				Draw(stack.top().first * (pathWidth + 1) + px, stack.top().second * (pathWidth + 1) + py, Solid, FG_Green);


		return true;
	}
};