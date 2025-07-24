if(CMAKE_SYSTEM_NAME MATCHES "NintendoWii|NintendoGameCube")
    list(APPEND EXTRA_FILES opengx_shaders.c)
endif()

if(GLM_FOUND)
    add_example(cube_tex cube_tex.cpp)
endif()
add_example(gl2gears gl2gears.c)
