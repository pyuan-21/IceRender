cmake_minimum_required(VERSION 3.9)

project(IceRender LANGUAGES CXX)

# this will check folder "External/CMakeLists.txt"
add_subdirectory(External)

# --------------------------------------------------------------------------------------------------------- #
# --------- below codes are used to include .hpp/.cpp files into .sln file(VS project/solution), --------- #
# --------- and generate its corresponding filter/group/folder inside .sln file automatically. --------- #
set(ALL_FILES "") # it will append each file into it.

# --------- set "all_file_paths" to a list of all files. --------- #
file(GLOB_RECURSE all_file_paths RELATIVE ${CMAKE_SOURCE_DIR} "Sources/*")

foreach(FILE ${all_file_paths}) 
    # message(${FILE}) 
    
    # Get the directory of the source file
    get_filename_component(PARENT_DIR "${FILE}" DIRECTORY)

    # Remove common directory prefix to make the group
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP_NAME "${PARENT_DIR}")

    # 'GROUP_NAME' is the filter/group/folder name inside VS solution.
    # message(${GROUP_NAME})

    # Group into "Source Files" and "Header Files" inside VS solution.
    if ("${FILE}" MATCHES ".*\\.cpp")
        # message("Source Files/${GROUP_NAME}")
        set(GROUP_NAME "Source Files/${GROUP_NAME}") # add prefix 'Source Files' to 'GROUP_NAME' name.
    elseif("${FILE}" MATCHES ".*\\.h")
        # message("Header Files/${GROUP_NAME}")
        set(GROUP_NAME "Header Files/${GROUP_NAME}") # add prefix 'Header Files' to 'GROUP_NAME' name.
    endif()

    # message("${FILE}")
    source_group("${GROUP_NAME}" FILES "${FILE}") # 'GROUP_NAME' is defined above
    list(APPEND ALL_FILES ${FILE})
endforeach()

# message("${ALL_FILES}")

# ------ all references are used to write above codes ------#
# [refer](https://stackoverflow.com/questions/41078807/cmake-and-visualstudio-group-files-in-solution-explorer)
# [refer](https://stackoverflow.com/questions/31037882/whats-the-cmake-syntax-to-set-and-use-variables)
# [refer](https://stackoverflow.com/questions/68139352/how-to-print-variables-in-cmake)
# [refer](https://stackoverflow.com/questions/20251829/is-it-possible-to-add-files-to-a-cmake-generated-solution-folder-in-visual-studi)
# [refer](https://stackoverflow.com/questions/52666330/cmake-for-each-source-file-one-executable)
# --------------------------------------------------------------------------------------------------------- #


add_executable(IceRender ${ALL_FILES})

# --------------------------------------------------------------------------------------------------------- #


set_target_properties(IceRender PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_link_libraries(IceRender LINK_PRIVATE glfw)

target_link_libraries(IceRender LINK_PRIVATE glm)

# the below two codes are replaced by the next line code. refer:https://github.com/Dav1dde/glad/issues/186
# include_directories(${CMAKE_CURRENT_SOURCE_DIR} External/glad/include)
# "External/glad/src/glad.c" # put this into above "file()"

target_link_libraries(IceRender LINK_PRIVATE glad)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} External/stb_image/)

# include "nlohmann_json"
# (how to use it)[https://github.com/nlohmann/json#examples]
# just use it for configuring the scene (maybe delete it later)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} External/nlohmann/)