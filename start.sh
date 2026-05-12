#!/bin/bash
# Nexorix Server — Linux Startup Script
cd "$(dirname "$0")"
export LD_LIBRARY_PATH=".:./NexorixPlugins:$LD_LIBRARY_PATH"
chmod +x nx-server 2>/dev/null
./nx-server "$@"
