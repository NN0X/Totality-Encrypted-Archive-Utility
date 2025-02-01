#include <iostream>

// for testing purposes
#include <fstream>

//

#include "archive.h"

int main()
{
        // TODO: test move file in place functions (consider splitting them into another project)

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

	if (archive.add("resources/file1.txt", "test1/test2/file2", false, false, 0, 0, false, {}))
	{
		std::cout << "File added successfully!" << std::endl;
	}
	else
	{
		std::cout << "Failed to add file!" << std::endl;
	}

        if (archive.add("resources/file1.txt", "dir1/dir2/dir3/dir4/dir5/dir6/file3", false, false, 0, 0, false, {}))
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
