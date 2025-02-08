#include <iostream>

// for testing purposes
#include <fstream>

//

#include "archive.h"

int main()
{
        // TODO: create clean cache function to remove unnecessary data from cache
        // INFO: file headers cache freeing order: (includes positions of file headers stored in DataHeader struct)
        // 1. search and remove file headers that are not in data cache
        // 2. search and remove file headers that are not directories
        // 3. search and remove file headers that are directories but have no children
        // 4. search and remove file headers that are directories but have no children in data cache
        // 5. search and remove directories with the smallest amount of children
        // 6. search and remove directories with the smallest amount of children in data cache
        // 7. search and remove directories furthest from root
        // 8. remove random file headers

        // INFO: data cache freeing order:
        // 1. remove data that is not in file headers cache
        // 2. remove from biggest to smallest data

        // INFO: common flags cache freeing order:
        // 1. remove unused flags
        // 2. remove flags that are used the least
        // 3. remove flags that are used the least in data cache (files)
        // 4. remove flags that are used the least in file headers cache (directories)
        // 5. remove from right to left

        // INFO: cache management order:
        // 1. manage file headers cache
        // 2. manage data cache
        // 3. manage common flags cache
        // 4. loop until both true:
        //      - file headers doesn't use uncached common flags
        //      - data cache doesn't contain files that are not in file headers cache

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

        if (archive.add("resources/file2.png", "test1/test2/file3", false, false, 0, 0, false, {}))
        {
                std::cout << "File added successfully!" << std::endl;
        }
        else
        {
                std::cout << "Failed to add file!" << std::endl;
        }

        if (archive.add("resources/file3.webm", "test1/test2/file4", false, false, 0, 0, false, {}))
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
