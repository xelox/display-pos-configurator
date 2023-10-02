#! /bin/bash
./build.sh
./main &
pid=$!
while inotifywait -e modify,create,delete ./src; do 
    clear
    if [ -n "$pid" ] && [ -e /proc/$pid ]; then
        kill "$pid"
    fi
    ./build.sh
    ./main &
    pid=$!
done
