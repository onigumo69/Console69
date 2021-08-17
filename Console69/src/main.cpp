#include "Console69.h"

#include "Maze.h"
#include "Space.h"
#include "World.h"

int main()
{
	//Maze demo;
	//Space demo;
	World demo;




	demo.Initialize(256, 240, 4, 4);
	demo.Start();

	return 0;
}