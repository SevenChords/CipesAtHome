#!/bin/sh

if [ ! -f /config/config.txt ]; then
    echo "Config file missing. Creating one."

    # Use sed to modify default values in config.txt if environment variables are set
    if [ ! -z ${SELECT} ]; then
        echo "Setting select to $SELECT"
        sed -ie "/^\s*select =.*/ s/= \([0-9]\+\)/= $SELECT/" config.txt
    fi

    if [ ! -z ${RANDOMISE} ]; then
        echo "Setting randomise to $RANDOMISE"
        sed -ie "/^\s*randomise =.*/ s/= \([0-9]\+\)/= $RANDOMISE/" config.txt
    fi

    if [ ! -z ${LOG_LEVEL} ]; then
        echo "Setting logLevel to $LOG_LEVEL"
        sed -ie "/^\s*logLevel =.*/ s/= \([0-9]\+\)/= $LOG_LEVEL/" config.txt
    fi

    if [ ! -z ${BRANCH_LOG_INTERVAL} ]; then
        echo "Setting branchLogInterval to $BRANCH_LOG_INTERVAL"
        sed -ie "/^\s*branchLogInterval =.*/ s/= \([0-9]\+\)/= $BRANCH_LOG_INTERVAL/" config.txt
    fi

    if [ ! -z ${WORKER_COUNT} ]; then
        echo "Setting workerCount to $WORKER_COUNT"
        sed -ie "/^\s*workerCount =.*/ s/= \([0-9]\+\)/= $WORKER_COUNT/" config.txt
    fi

    if [ ! -z ${USERNAME} ]; then
        echo "Setting Username to $USERNAME"
        sed -ie "/^\s*Username =.*/ s/= \".*\"/= \"$USERNAME\"/" config.txt
    fi

    # Copy config to volume where it will be used
    cp config.txt /config/config.txt
fi

# Make CWD /config to force the app to use /config/config.txt rather than /app/config.txt without putting the binary in the volume
cd /config

/app/recipesAtHome