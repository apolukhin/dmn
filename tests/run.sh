#!/bin/bash

SRCDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

docker rmi -f dmn-test

cp $SRCDIR/dockers/dmn-test.dockerfile ./
docker build -t dmn-test -f dmn-test.dockerfile .

docker run -i -t dmn-test
#
