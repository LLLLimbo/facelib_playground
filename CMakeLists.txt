cmake_minimum_required(VERSION 3.16.3)
project(facelib_playground)

set(OS ubuntu18)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fexec-charset=UTF-8")

INCLUDE_DIRECTORIES(include)
AUX_SOURCE_DIRECTORY(src DIR_SRCS)

INCLUDE_DIRECTORIES(os/cpu/${OS}/usr/include)
FILE(GLOB CUSTOM_LIBS os/cpu/${OS}/usr/lib/*)


MESSAGE("FOUND LIBS" ${CUSTOM_LIBS})

ADD_LIBRARY(${PROJECT_NAME} SHARED ${DIR_SRCS})

TARGET_LINK_LIBRARIES(${PROJECT_NAME}
        ${CUSTOM_LIBS}
        )


ADD_EXECUTABLE(${PROJECT_NAME}test ${DIR_SRCS})

TARGET_LINK_LIBRARIES(${PROJECT_NAME}test ${PROJECT_NAME}
        ${CUSTOM_LIBS}
        )

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${PROJECT_SOURCE_DIR}/lib)
INSTALL(TARGETS ${PROJECT_NAME}test DESTINATION ${PROJECT_SOURCE_DIR})

