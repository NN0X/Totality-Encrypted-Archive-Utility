#include <iostream>

// for testing purposes
#include <fstream>

//

#include "archive.h"

int main()
{
	TEA archive(".", "archive");

	if (archive.add("", "test1", false, false, 0, 0, true, {}))
	{
		std::cout << "Directory added successfully!" << std::endl;
	}
	else
	{
		std::cout << "Failed to add directory!" << std::endl;
	}

	if (archive.add("resources/file1.txt", "file1", false, false, 0, 0, false, {}))
	{
		std::cout << "File added successfully!" << std::endl;
	}
	else
	{
		std::cout << "Failed to add file!" << std::endl;
	}

	archive.setMetadata("metadata");

	// BUG: weird behavior when adding file to directory with path containing existing directory
	// BUG: for some reason the behavior is not consistent

	if (archive.add("resources/file1.txt", "test1/test2/file2", false, false, 0, 0, false, {}))
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
