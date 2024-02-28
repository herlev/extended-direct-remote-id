#!/usr/bin/env bash

VENDOR_ID="303a"
PRODUCT_ID="1001"

for tty_device in /dev/ttyACM*; do
    vendor_id=$(udevadm info --query all --name "$tty_device" | grep "E: ID_VENDOR_ID" | awk -F'=' '{print $2}' | tr -d '"')
    product_id=$(udevadm info --query all --name "$tty_device" | grep "E: ID_MODEL_ID" | awk -F'=' '{print $2}' | tr -d '"')

    if [ "$vendor_id" == "$VENDOR_ID" ] && [ "$product_id" == "$PRODUCT_ID" ]; then
        echo "$tty_device"
        exit 0
    fi
done

echo "No matching device found." >&2
exit 1
