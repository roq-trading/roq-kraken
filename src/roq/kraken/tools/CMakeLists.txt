set(TARGET_NAME ${PROJECT_NAME}-tools)

set(SOURCES crypto.cpp)

add_library(${TARGET_NAME} OBJECT ${SOURCES})

add_dependencies(${TARGET_NAME} autogen-headers)

target_link_libraries(${TARGET_NAME} PRIVATE ${PROJECT_NAME}-flags roq-core::roq-core roq-utils::roq-utils fmt::fmt)
