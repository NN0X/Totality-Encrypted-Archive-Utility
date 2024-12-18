#include <iostream>

// for testing purposes
#include <fstream>

//

#include "archive.h"

int main()
{
	TEA archive(".", "archive");

	if (archive.add("file1.txt", "file1", false, false, 0, 0, false, {}))
	{
		std::cout << "File added successfully!" << std::endl;
	}
	else
	{
		std::cout << "Failed to add file!" << std::endl;
	}

	// wait
	std::cin.get();
	if (archive.save())
	{
		std::cout << "Archive saved successfully!" << std::endl;
	}
	else
	{
		std::cout << "Failed to save archive!" << std::endl;
	}

	// wait
	std::cin.get();

	TEA archive2(".", "archive");
	if (archive2.load())
	{
		std::cout << "Archive loaded successfully!" << std::endl;
	}
	else
	{
		std::cout << "Failed to load archive!" << std::endl;
	}

	archive2.info();

	// wait
	std::cin.get();

	return 0;
}
