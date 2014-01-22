#!/bin/bash

ROOT=$HOME/syndicate/syndicate-MS-clients-root
NAME="syndicate-MS-clients"
VERSION="0.$(date +%Y\%m\%d\%H\%M\%S)"

DEPS="python-syndicate" 

DEPARGS=""
for pkg in $DEPS; do
   DEPARGS="$DEPARGS -d $pkg"
done

source /usr/local/rvm/scripts/rvm

fpm --force -s dir -t rpm -a noarch -v $VERSION -n $NAME $DEPARGS -C $ROOT --license "Apache 2.0" --vendor "Princeton University" --description "Syndicate MS clients.  This includes syntool.py" $(ls $ROOT)

