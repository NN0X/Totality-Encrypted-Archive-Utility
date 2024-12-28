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

	archive.info();
	archive.list();

	archive.setMetadata("metadata");

	if (archive.save())
	{
		std::cout << "Archive saved successfully!" << std::endl;
	}
	else
	{
		std::cout << "Failed to save archive!" << std::endl;
	}

	if (archive.load())
	{
		std::cout << "Archive loaded successfully!" << std::endl;
	}
	else
	{
		std::cout << "Failed to load archive!" << std::endl;
	}

	if (archive.add("file1.txt", "file2", false, false, 0, 0, false, {}))
	{
		std::cout << "File added successfully!" << std::endl;
	}
	else
	{
		std::cout << "Failed to add file!" << std::endl;
	}

	archive.info();
	archive.list();

	if (archive.save())
	{
		std::cout << "Archive saved successfully!" << std::endl;
	}
	else
	{
		std::cout << "Failed to save archive!" << std::endl;
	}

	return 0;
}
