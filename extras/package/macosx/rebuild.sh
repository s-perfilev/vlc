#!/bin/sh

# remove VLC.app to force rebuilding
rm -rf VLC.app

# build changed sources
`dirname $0`/build.sh
