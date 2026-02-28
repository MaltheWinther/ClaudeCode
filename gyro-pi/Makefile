CXX      = g++
CXXFLAGS = -std=c++17 -Wall -I/usr/include
LDFLAGS  = -L/usr/lib -lBMI160Wrapper -lm -Wl,-rpath,/usr/lib

TARGET = gyro_stream

$(TARGET): gyro_stream.cpp spi_interface.hpp
	$(CXX) $(CXXFLAGS) -o $@ gyro_stream.cpp $(LDFLAGS)

clean:
	rm -f $(TARGET)
