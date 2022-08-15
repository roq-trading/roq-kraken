#!/usr/bin/env bash

CWD="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

if [ "$1" == "debug" ]; then
	PREFIX="gdb --args"
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
	--config_file "$CONFIG_FILE" \
  --event_log_dir "$HOME/var/lib/roq/data" \                                                                            
  --event_log_symlink \                                                                                                 
  --client_listen_address "$HOME/run/$NAME.sock" \                                                                      
  --metrics_listen_address "$HOME/run/${NAME}_metrics.sock" \
	--rest_uri "$REST_URI" \
	--ws_public_uri "$WS_PUBLIC_URI" \
	--ws_private_uri "$WS_PRIVATE_URI" \
	$@
