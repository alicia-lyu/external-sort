#!/bin/bash

calculate_time_difference() {
    START_TIME=$(head -n 1 "$1")
    END_TIME=$(tail -n 1 "$1")
    TIME_DIFFERENCE=$((END_TIME - START_TIME))
    if [ $TIME_DIFFERENCE -gt 500 ]; then
        TIME_DIFFERENCE_M=$((TIME_DIFFERENCE / 60))
        TIME_DIFFERENCE_S=$((TIME_DIFFERENCE % 60))
        echo "Took $TIME_DIFFERENCE_M minutes and $TIME_DIFFERENCE_S seconds"
    else
        echo "Took $TIME_DIFFERENCE seconds"
    fi
}

calculate_time_difference "$1"