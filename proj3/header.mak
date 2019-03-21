# for programs using pthreads and curses
CXXFLAGS =	-ggdb
CFLAGS =	-ggdb -std=c99 -Wall -Wextra -pthread
# might use wide character data
CLIBFLAGS =	-lm -lncursesw -pthread
