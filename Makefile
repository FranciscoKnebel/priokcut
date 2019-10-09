INC=-I./include

all: pkuts

pkuts: src/utils.cpp src/aigreader.cpp src/priokcuts.cpp
	g++ $(INC) -O3 src/utils.cpp src/aigreader.cpp src/priokcuts.cpp -o priokcuts

toascii: src/toascii.cpp
	g++ $(INC) -O3 src/toascii.cpp -o toascii

clean:
	rm -rf priokcuts toascii