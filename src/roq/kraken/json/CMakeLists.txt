set(TARGET_NAME "${PROJECT_NAME}-json")

set(SOURCES parser_private.cpp parser_public.cpp)

include(RoqAutogen)

set(AUTOGEN_SCHEMAS
    add_order_status.json
    asset_pairs.json
    asset_pairs_item.json
    assets.json
    assets_item.json
    book.json
    book_item.json
    cancel_order_status.json
    channel.json
    error.json
    event.json
    heartbeat.json
    open_orders.json
    open_orders_item.json
    open_positions.json
    order.json
    order_type.json
    own_trades.json
    own_trades_item.json
    ping.json
    pong.json
    positions.json
    positions_item.json
    result_field.json
    side.json
    spread.json
    subscription_status.json
    system_status.json
    time_.json
    token.json
    trade.json
    trade_balance.json
    trade_balance_item.json
    trade_item.json
    update_type.json)

roq_autogen(
  OUTPUT
  AUTOGEN_HEADERS
  NAMESPACE
  "roq/kraken/json"
  OUTPUT_TYPE
  "json"
  FILE_TYPE
  "h"
  SOURCES
  ${AUTOGEN_SCHEMAS})

set(GITIGNORE "${CMAKE_CURRENT_SOURCE_DIR}/.gitignore")

file(WRITE "${GITIGNORE}" "")

foreach(path ${AUTOGEN_SCHEMAS})
  get_filename_component(name ${path} NAME_WE)
  file(APPEND "${GITIGNORE}" "${name}.h\n${name}.cpp\n")
endforeach()

add_custom_target(autogen-json-headers ALL DEPENDS ${AUTOGEN_HEADERS})

roq_autogen(
  OUTPUT
  AUTOGEN_SOURCES
  NAMESPACE
  "roq/kraken/json"
  OUTPUT_TYPE
  "json"
  FILE_TYPE
  "cpp"
  SOURCES
  ${AUTOGEN_SCHEMAS})

add_library("${TARGET_NAME}" STATIC ${SOURCES} ${AUTOGEN_SOURCES})

add_dependencies("${TARGET_NAME}" autogen-json-headers)

target_link_libraries("${TARGET_NAME}" roq-core::roq-core roq-logging::roq-logging fmt::fmt)
