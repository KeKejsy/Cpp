cmake_minimum_required(VERSION 3.15)
project(RhythmMaster)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SFML 3 REQUIRED COMPONENTS Graphics Audio Window System)

# 获取 DLL 所在目录（MSYS2 UCRT64）
find_path(MINGW_BIN_DIR NAMES "libstdc++-6.dll" PATHS "C:/msys64/ucrt64/bin" NO_DEFAULT_PATH)

add_executable(rhythm_master
    src/GameMain.cpp
    src/GameFramework.cpp
)

target_link_libraries(rhythm_master
    SFML::Graphics
    SFML::Audio
    SFML::Window
    SFML::System
)

# 构建后自动复制 DLL
if(MINGW_BIN_DIR)
    add_custom_command(TARGET rhythm_master POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW_BIN_DIR}/libsfml-audio-3.dll"
            "${MINGW_BIN_DIR}/libsfml-graphics-3.dll"
            "${MINGW_BIN_DIR}/libsfml-window-3.dll"
            "${MINGW_BIN_DIR}/libsfml-system-3.dll"
            "${MINGW_BIN_DIR}/libgcc_s_seh-1.dll"
            "${MINGW_BIN_DIR}/libstdc++-6.dll"
            "${MINGW_BIN_DIR}/libwinpthread-1.dll"
            $<TARGET_FILE_DIR:rhythm_master>
    )
endif()

# 成员 B 测试 (文件已删除，暂时注释)
# add_executable(test_member_b
#     src/test_member_b.cpp
#     src/GameFramework.cpp
# )
#
# target_link_libraries(test_member_b
#     SFML::Graphics
#     SFML::Audio
#     SFML::Window
#     SFML::System
# )
#
# if(MINGW_BIN_DIR)
#     add_custom_command(TARGET test_member_b POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy_if_different
#             "${MINGW_BIN_DIR}/libsfml-audio-3.dll"
#             "${MINGW_BIN_DIR}/libsfml-graphics-3.dll"
#             "${MINGW_BIN_DIR}/libsfml-window-3.dll"
#             "${MINGW_BIN_DIR}/libsfml-system-3.dll"
#             "${MINGW_BIN_DIR}/libgcc_s_seh-1.dll"
#             "${MINGW_BIN_DIR}/libstdc++-6.dll"
#             "${MINGW_BIN_DIR}/libwinpthread-1.dll"
#             $<TARGET_FILE_DIR:test_member_b>
#     )
# endif()
