#!/bin/bash
MM_ACTIVE=1

while [ $MM_ACTIVE != 0 ]
do
    echo "Starting tradey...."
    python2.7 main.py >> error_log.txt

    if [ "$?" != "0" ]; then
        # Inventory/Money out of sync
        echo "tradey quit with error at:"
        date
        MM_ACTIVE=0
    else
        echo "Tradey quit normally, sleeping 60 seconds before a restart"
        sleep 60
    fi
done
