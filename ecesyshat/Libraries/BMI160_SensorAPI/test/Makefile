# Makefile to build main.cpp against system‑installed libbmi160wrapper

CXX        := g++
# Look in /usr/include for bmi160_wrapper.hpp & bmi160.h
CXXFLAGS   := -Wall -Wextra -std=c++17 -pthread -I/usr/include
# Link against /usr/lib/libbmi160wrapper.so and pthread
LDFLAGS    := -L/usr/lib -lBMI160wrapper -lpthread -lI2Cwrap



SRC        := main.cpp
OBJ        := main.o
BIN        := main

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJ)
	@echo "Linking $@"
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ): $(SRC)
	@echo "Compiling $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "Cleaning up"
	-rm -f $(OBJ) $(BIN)

