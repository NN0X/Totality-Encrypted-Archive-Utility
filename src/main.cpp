#include <iostream>

// for testing purposes
#include <fstream>

//

#include "archive.h"

int main()
{
        // TODO: test move file in place functions (consider splitting them into another project)
        // TODO: create clean cache function to remove unnecessary data from cache
        // INFO: file headers cache freeing order:
        // 1. search and remove file headers that are not in data cache
        // 2. search and remove file headers that are not directories
        // 3. search and remove file headers that are directories but have no children
        // 4. search and remove file headers that are directories but have no children in data cache
        // 5. search and remove directories with the smallest amount of children
        // 6. search and remove directories with the smallest amount of children in data cache
        // 7. search and remove directories furthest from root
        // 8. remove random file headers

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
	archive.tree();

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
