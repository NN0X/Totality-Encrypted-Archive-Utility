#include <iostream>

// for testing purposes
#include <fstream>

//

#include "archive.h"

int main()
{
	TEA archive(".", "archive");
	if (archive.load())
	{
		std::cout << "Archive loaded" << std::endl;
	}
	else
	{
		std::cout << "Archive not loaded" << std::endl;
	}

	return 0;
}
