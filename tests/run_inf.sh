#!/bin/bash
i=0
while (./dmn_tests --log_level=message -x)
    do echo "OK $i"
    let i=i+1
done
echo "Failed at $i"
