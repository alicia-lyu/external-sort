#!/bin/bash

calculate_time_difference() {
    START_TIME=$(head -n 1 "$1")
    END_TIME=$(tail -n 1 "$1")
    TIME_DIFFERENCE=$((END_TIME - START_TIME))
    UNIT="seconds"
    if [ $TIME_DIFFERENCE -gt 500 ]; then
        TIME_DIFFERENCE=$((TIME_DIFFERENCE / 60))
        UNIT="minutes"
    fi
    echo "Took $TIME_DIFFERENCE $UNIT"
}

calculate_time_difference "$1"