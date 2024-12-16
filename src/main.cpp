#include <iostream>

// for testing purposes
#include <fstream>

//

#include "archive.h"

int main()
{
	TEA archive(".", "archive");

	if (archive.add("test", "test", false, false, 0, 0, false, {}))
	{
		std::cout << "File added successfully!" << std::endl;
	}
	else
	{
		std::cout << "Failed to add file!" << std::endl;
	}

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
