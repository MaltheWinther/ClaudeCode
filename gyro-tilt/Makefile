CXX      = g++
CXXFLAGS = -std=c++17 -Wall -I/usr/include
LDFLAGS  = -L/usr/lib -lBMI160Wrapper -lm -Wl,-rpath,/usr/lib

TARGET = tilt_display
SRCS   = tilt_display.cpp

$(TARGET): $(SRCS) spi_interface.hpp
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS) $(LDFLAGS)

clean:
	rm -f $(TARGET)
