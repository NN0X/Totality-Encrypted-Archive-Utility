#include <iostream>

#include "archive.h"

int main()
{
	Archive archive("archive", "");
	archive.addFile("file1.txt"); // adds file1.txt from current directory to top level archive directory
	archive.addFile("file2.png"); // adds file2.png from current directory to top level archive directory
	archive.addFile("file3.webm"); // adds file3.webm from current directory to top level archive directory

	archive.addDirectory("dir1"); // adds dir1 to top level archive directory
	archive.setDirectory("dir1"); // sets current directory to dir1
	archive.addFile("file1.txt"); // adds file1.txt from current directory to dir1 in top level archive directory
	archive.addFile("file2.png"); // adds file2.png from current directory to dir1 in top level archive directory
	archive.addFile("file3.webm"); // adds file3.webm from current directory to dir1 in top level archive directory

	archive.addDirectory("dir2"); // adds dir2 to dir1 in top level archive directory
	archive.setDirectory(""); // sets current directory to top level archive directory
	archive.setFile("file1.txt"); // sets current file to file1.txt in top level archive directory
	archive.extractFile("file1_copy.txt"); // extracts file1.txt from top level archive directory to file1_copy.txt in current directory
	archive.setDirectory("dir1"); // sets current directory to dir1 in top level archive directory
	archive.setDirectory("dir2"); // sets current directory to dir2 in dir1 in top level archive directory
	archive.extractDirectory("dir2_copy"); // extracts dir2 from dir1 in top level archive directory to dir2_copy in current directory
	archive.extractAll("extracted"); // extracts all archive files and directories to directory "extracted" in current directory
	archive.compressHUFFMAN(); // compresses archive using HUFFMAN algorithm
	archive.setDirectory("dir1"); // sets current directory to dir1 in top level archive directory
	archive.encryptDirectoryBRUTUS(); // encrypts dir1 using BRUTUS algorithm

	archive.encryptBRUTUS(); // encrypts the archive using BRUTUS algorithm
	archive.save(); // saves archive to disk
}
