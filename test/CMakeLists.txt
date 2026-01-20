include(RoqAutogen)

set(TARGET_NAME ${PROJECT_NAME}-test)

set(SOURCES
    # json_balance_ack.cpp  XXX FIXME need key-double
    json_balances.cpp
    json_book.cpp
    json_error.cpp
    json_executions.cpp
    json_heartbeat.cpp
    json_instrument.cpp
    json_open_positions_ack.cpp
    json_pong.cpp
    json_status.cpp
    json_subscribe.cpp
    json_ticker.cpp
    json_token_ack.cpp
    json_trade.cpp
    main.cpp)

roq_gitignore(OUTPUT .gitignore SOURCES ${TARGET_NAME})

add_executable(${TARGET_NAME} ${SOURCES})

target_link_libraries(${TARGET_NAME} PRIVATE ${PROJECT_NAME}-json Catch2::Catch2)

if(ROQ_BUILD_TYPE STREQUAL "Release")
  set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE -s)
endif()

add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
