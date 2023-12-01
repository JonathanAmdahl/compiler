# Makefile for compiler project

# Compiler and compiler flags
CXX = g++
CXXFLAGS = -Wall -g

# Define the source files
SOURCES = compiler.cc demo.cc inputbuf.cc lexer.cc

# Define the object files
OBJECTS = $(SOURCES:.cc=.o)

# Define the executable file
EXECUTABLE = a.out

# Default target
all: $(SOURCES) $(EXECUTABLE)

# Linking the executable
$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@

# Compiling source files to object files
.cc.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean target for removing compiled files
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
