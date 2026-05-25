cmake_minimum_required(VERSION 3.16)
project(strictify_validator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(strictify
    src/main.cpp
    src/utils.cpp
    src/parsers.cpp
    src/frontmatter_validator.cpp
    src/content_validator.cpp
    src/card_validator.cpp
)
