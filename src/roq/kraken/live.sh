#!/usr/bin/env bash

CWD="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

if [ "$1" == "debug" ]; then
	PREFIX="libtool --mode=execute gdb --args"
else
	PREFIX=
fi

NAME="kraken"

CONFIG_FILE="$CWD/config/$NAME.toml"

ENV=""

URI="kraken.com"

REST_URI="https://${ENV}api.$URI/0"
WS_URI_PUBLIC="wss://${ENV}ws.$URI"
WS_URI_PRIVATE="wss://${ENV}ws-auth.$URI"

$PREFIX ./roq-kraken \
	--name "$NAME" \
	--config-file "$CONFIG_FILE" \
	--rest-uri "$REST_URI" \
	--ws-uri "$WS_URI_PUBLIC" \
	--listen "$CWD/$NAME.sock" \
	--metrics "$CWD/${NAME}_metrics.sock" \
	$@
