set(TARGET_NAME ${PROJECT_NAME}-json)

set(SOURCES map.cpp parser_private.cpp parser_public.cpp)

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

roq_autogen_hpp(
  OUTPUT
  AUTOGEN_HEADERS
  SOURCES
  ${AUTOGEN_SCHEMAS}
  TEMPLATE_DIR
  ${TEMPLATE_DIR}
  TEMPLATE_TYPE
  "json")

add_custom_target(autogen-json-headers ALL DEPENDS ${AUTOGEN_HEADERS})

roq_autogen_cpp(
  OUTPUT
  AUTOGEN_SOURCES
  SOURCES
  ${AUTOGEN_SCHEMAS}
  TEMPLATE_DIR
  ${TEMPLATE_DIR}
  TEMPLATE_TYPE
  "json")

roq_gitignore(OUTPUT .gitignore SOURCES ${TARGET_NAME} ${AUTOGEN_HEADERS} ${AUTOGEN_SOURCES})

add_library(${TARGET_NAME} OBJECT ${SOURCES} ${AUTOGEN_SOURCES})

add_dependencies(${TARGET_NAME} autogen-json-headers)

target_link_libraries(${TARGET_NAME} roq-core::roq-core roq-logging::roq-logging fmt::fmt)
