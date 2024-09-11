CXX = clang++
CFLAGS = -O3 -Wall -Wextra -Wpedantic
SRC = src
OUT = main

all:
	rm -f -r build
	mkdir build
	$(CXX) $(CFLAGS) $(SRC)/*.cpp -o build/$(OUT) 2> build/make.log
	cp -f -r resources build/resources
	cp LICENSE.md build/LICENSE.md
	cp NOTICE.md build/NOTICE.md
	cp README.md build/README.md
	@echo "Build complete. Executable is located at build/$(OUT)"
