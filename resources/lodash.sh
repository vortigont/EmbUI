#!/bin/sh

#npm i -g npm
#npm i -g lodash-cli

# make custom build of lodash js lib
rm -f *.js *.gz
lodash include=at,get,has,hasIn,merge,,mergeWith,set,setWith -c | gzip -9 > html/js/lodash.custom.js.gz
#lodash category=array,object -c | gzip -9 > lodash.custom.js.gz