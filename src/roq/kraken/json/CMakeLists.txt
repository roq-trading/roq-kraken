set(TARGET_NAME ${PROJECT_NAME}-json)

set(SOURCES encoder.cpp map.cpp parser.cpp)

include(RoqAutogen)

set(AUTOGEN_SCHEMAS
    add_order.json
    add_order_result.json
    balance_ack.json
    balance_ack_result_item.json
    balances_data_item.json
    balances_data_item_wallets_item.json
    balances.json
    book_data_bids_asks_item.json
    book_data.json
    book.json
    cancel_all.json
    cancel_all_result.json
    cancel_order.json
    cancel_order_result.json
    channel.json
    error.json
    event.json
    exec_type.json
    executions_data_item.json
    executions.json
    heartbeat.json
    instrument_data_assets_item.json
    instrument_data.json
    instrument_data_pairs_item.json
    instrument.json
    limit_price_type.json
    open_positions_ack.json
    open_positions_ack_result_item.json
    order.json
    order_status.json
    order_type.json
    pairs_status.json
    ping.json
    pong.json
    side.json
    status_data_item.json
    status.json
    subscribe.json
    subscribe_result.json
    ticker_data_item.json
    ticker.json
    time_in_force.json
    time_.json
    token_ack.json
    token_ack_result.json
    trade_balance_item.json
    trade_balance.json
    trade_data_item.json
    trade.json
    type.json
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

target_link_libraries(${TARGET_NAME} PRIVATE roq-core::roq-core roq-codec::roq-codec roq-utils::roq-utils roq-logging::roq-logging fmt::fmt)
