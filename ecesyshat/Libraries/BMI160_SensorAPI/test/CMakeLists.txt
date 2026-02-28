add_executable(BMI160WrapperTest main.cpp)

target_include_directories(BMI160WrapperTest PRIVATE ${PROJECT_SOURCE_DIR}/include)

# Explicitly link required libraries
target_link_libraries(BMI160WrapperTest PRIVATE
    BMI160Wrapper
    I2Cwrap
    rt
    SSD1306_OLED_RPI
)


