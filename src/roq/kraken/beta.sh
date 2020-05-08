#!/usr/bin/env bash

CWD="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

if [ "$1" == "debug" ]; then
	PREFIX="libtool --mode=execute gdb --args"
else
	PREFIX=
fi

NAME="kraken-beta"

CONFIG_FILE="$CWD/config/$NAME.toml"

ENV="beta-"

URI="kraken.com"

REST_URI="https://api.$URI"

WS_PUBLIC_URI="wss://${ENV}ws.$URI"
WS_PRIVATE_URI="wss://${ENV}ws-auth.$URI"

$PREFIX ./roq-kraken \
	--name "$NAME" \
	--config-file "$CONFIG_FILE" \
	--rest-uri "$REST_URI" \
	--ws-public-uri "$WS_PUBLIC_URI" \
	--ws-private-uri "$WS_PRIVATE_URI" \
	--listen "$CWD/$NAME.sock" \
	--metrics "$CWD/${NAME}_metrics.sock" \
	$@
