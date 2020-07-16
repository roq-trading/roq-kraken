set(TARGET_NAME "${PROJECT_NAME}-test")

set(SOURCES
  json_book.cpp
  json_token.cpp
  main.cpp
)

add_executable("${TARGET_NAME}"
  ${SOURCES}
)

target_link_libraries("${TARGET_NAME}"
  "${PROJECT_NAME}-json"
  roq-server::roq-server
  roq-core::roq-core
  roq-logging::roq-logging
  gtest_main
  ${CROSSGUID_LIBRARIES}
  ${UUID_LIBRARIES}
  ${RT_LIBRARIES}
)

set_target_properties("${TARGET_NAME}" PROPERTIES LINK_FLAGS_RELEASE -s)

# run

add_test(NAME "${TARGET_NAME}" COMMAND "${TARGET_NAME}")
