CC			=g++
CXXFLAGS	=-O3 -std=c++17
SOURCES	 	=$(wildcard ./src/*.cc)
INCLUDES  	=-I./include
LIB_NAMES 	=-lpthread -static-libgcc -static-libstdc++
file        =$(SOURCES)
OBJ			=$(patsubst %.cc, %.o, $(file))
TARGET		=$(patsubst %.cc, %.out, $(file))

#links
%.out:%.o
	@mkdir -p output
	$(CC) $< $(LIB_NAMES) -o output/$(notdir $@)
	
#compile
%.o: %.cc
	$(CC) $(INCLUDES) -c $(CXXFLAGS) $< -o $@

all: $(TARGET)
	@mkdir -p output

.PHONY:clean

clean:
	@echo "Remove linked and compiled files......"
	@rm -rf output
	@rm -rf $(OBJ)
