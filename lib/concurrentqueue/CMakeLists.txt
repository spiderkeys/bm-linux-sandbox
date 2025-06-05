cmake_minimum_required(VERSION 3.19)
project(concurrentqueue CXX)

# ==============================================
# Targets

add_library( ${PROJECT_NAME} INTERFACE )

target_include_directories(${PROJECT_NAME} 
        INTERFACE 
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/>
)

install(TARGETS ${PROJECT_NAME} 
    EXPORT ${PROJECT_NAME}Targets
)