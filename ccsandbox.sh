#!/bin/bash
sandbox-exec -f ./permissive-open.sb -D TARGET_DIR="$(pwd)" -D HOME_DIR="$HOME" claude --dangerously-skip-permissions -c

