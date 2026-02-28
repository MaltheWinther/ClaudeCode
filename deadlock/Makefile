CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread

TARGET = plcs

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) -o $@ main.cpp

clean:
	rm -f $(TARGET)
