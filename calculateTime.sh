#!/bin/bash

calculate_time_difference() {
    START_TIME=$(head -n 1 "$1")
    END_TIME=$(tail -n 1 "$1")
    TIME_DIFFERENCE=$((END_TIME - START_TIME))
    UNIT="seconds"
    if [ $TIME_DIFFERENCE -gt 500 ]; then
        TIME_DIFFERENCE_M=$((TIME_DIFFERENCE / 60))
        UNIT_M="minutes"
        TIME_DIFFERENCE_S=$((TIME_DIFFERENCE % 60))
        UNIT_S="seconds"
        echo "Took $TIME_DIFFERENCE_M $UNIT_M $TIME_DIFFERENCE_S $UNIT_S"
    else
        echo "Took $TIME_DIFFERENCE $UNIT"
    fi
}

calculate_time_difference "$1"