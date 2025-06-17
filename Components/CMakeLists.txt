# Set minimum required CMake version
cmake_minimum_required(VERSION 3.10)


# Set project name
project(KingModComponents)


# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Define source files
set(SOURCES
    Components/Main.cpp
    Components/GUI.cpp
    Components/BallPrediction.cpp
    DiscordRPC.cpp
    DiscordRPCWrapper.cpp
    # Add other source files here
)


# Add the library
add_library(KingModComponents SHARED ${SOURCES})


# Set preprocessor definitions
target_compile_definitions(KingModComponents PRIVATE 
    DISCORD_RPC_CPP_EXPORTS
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)


# Include directories
target_include_directories(KingModComponents PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    # Add other include directories here
)


# Link libraries
target_link_libraries(KingModComponents PRIVATE
    # Add other libraries to link against here
    # Example: user32.lib gdi32.lib
)


# Copy Discord RPC DLL to output directory
add_custom_command(TARGET KingModComponents POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/discord-rpc.dll
    $<TARGET_FILE_DIR:KingModComponents>
    COMMENT "Copying discord-rpc.dll to output directory"
)


# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
