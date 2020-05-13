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

REST_URI="https://api.$URI"

WS_PUBLIC_URI="wss://${ENV}ws.$URI"
WS_PRIVATE_URI="wss://${ENV}ws-auth.$URI"

$PREFIX ./roq-kraken \
	--name "$NAME" \
	--config-file "$CONFIG_FILE" \
	--client-listen-address "$CWD/$NAME.sock" \
	--metrics-listen-address "$CWD/${NAME}_metrics.sock" \
	--rest-uri "$REST_URI" \
	--ws-public-uri "$WS_PUBLIC_URI" \
	--ws-private-uri "$WS_PRIVATE_URI" \
	$@
