#!/usr/bin/env bash


# etag file
tags=etags.txt
compressor="gz"
compress_cmd="zopfli"
compress_args=""

refresh_rq=0
tzcsv=https://raw.githubusercontent.com/nayarsystems/posix_tz_db/master/zones.csv

optstring=":hf:c:"

usage(){
  echo "Usage: `basename $0` [-h] [-f] [-c zopfli|gz|br]"
cat <<EOF
Options:
    -f          force update all resoruces
    -c          compressor to use 'gz' for gzip (default), or 'br' for brotli
    -h          show this help
EOF
}


# parse cmd options
while getopts ${optstring} OPT; do
    case "$OPT" in
        c)
            echo "Set compressor to: $OPTARG"
            compress_cmd=$OPTARG
            ;;
        h)
            echo $USAGE
            exit 0
            ;;
        f)
            echo "Force refresh"
            rm -f $tags
            refresh_rq=1
            ;;
        :)
            echo "$0: mandatory argument is missin for -$OPTARG" >&2
            ;;
        \?)
            # getopts issues an error message
            echo $USAGE >&2
            exit 1
            ;;
    esac
done

compress_zopfli(){
    local src="$1"
    zopfli ${compress_args} ${src}
    rm -f ${src}
}

compress_gz(){
    local src="$1"
    gzip ${compress_args} ${src}
}

compress_br(){
    local src="$1"
    brotli ${compress_args} ${src}
}


if [[ "$compress_cmd" = "gz" ]] ; then
    compress_cmd=`which gzip`
    if [ "x$compress_cmd" = "x" ]; then
        echo "ERROR: gzip compressor not found!"
        exit 1
    fi
    compress_cmd=compress_gz
    compress_args="-9"
elif [[ "$compress_cmd" = "zopfli" ]] ; then
    compress_cmd=`which zopfli`
    if [ "x$compress_cmd" = "x" ]; then
        echo "ERROR: zopfli compressor not found!"
        exit 1
    fi
    compress_cmd=compress_zopfli
elif [[ "$compress_cmd" = "br" ]] ; then
    compress_cmd=`which brotli`
    if [ "x$compress_cmd" = "x" ]; then
        echo "ERROR: brotli compressor not found!"
        exit 1
    fi
    compress_cmd=compress_br
    compress_args="--best"
fi
echo "Using compressor: $compress_cmd"

[ -f $tags ] || touch $tags

# check github file for a new hash
freshtag(){
    local url="$1"
    etag=$(curl -sL -I $url | grep etag | awk '{print $2}')
    if [[ "$etag" = "" ]] ; then
	    return 0
    fi
    echo "$url $etag" >> newetags.txt
    if [ $(grep -cs $etag $tags) -eq 0 ] ; then
        #echo "new tag found for $url"
        return 0
    fi
    #echo "old tag for $url"
    return 1
}

# update local file if source has newer version
updlocalarchive(){
    local res=$1
    echo "check: $res"
    [ ! -f html/${res} ] && return
    if [ ! -f data/${res}.${compressor} ] || [ html/${res} -nt data/${res}.${compressor} ] ; then
        cp html/${res} data/${res}
        ${compress_cmd}  data/${res} && touch -r html/${res} data/${res}.${compressor}
    fi
}

echo "Preparing resources for EmbUI FS image" 

mkdir -p ./data/css ./data/js
cat html/css/pure*.css html/css/grids*.css > ./data/css/pure.css
${compress_cmd}  ./data/css/pure.css
cat html/css/*_default.css > ./data/css/style.css
${compress_cmd}  ./data/css/style.css
cat html/css/*_light.css > ./data/css/style_light.css
${compress_cmd}  ./data/css/style_light.css
cat html/css/*_dark.css > ./data/css/style_dark.css
${compress_cmd}  ./data/css/style_dark.css

cp -u html/css/*.jpg ./data/css/
cp -u html/css/*.webp ./data/css/
cp -u html/css/*.${compressor} ./data/css/

echo "Packing EmbUI js" 
embui_js="dyncss.js lib.js maker.js"
# combine and compress js files in one bundle
for f in ${embui_js}
do
    cat html/js/${f} >> data/js/embui.js
done
${compress_cmd}  data/js/embui.js

cp -u html/js/lodash.custom.js* data/js/

echo "Packing static files"
# static gz files
static_gz_files='js/ui_sys.json index.html favicon.ico'
for f in ${static_gz_files}
do
    updlocalarchive $f
done

echo "Update TZ"
# update TZ info
if freshtag ${tzcsv} || [ $refresh_rq -eq 1 ] ; then
    echo "Updating TZ info"
    echo '"label","value"' > ./data/js/zones.csv
    curl -sL $tzcsv >> ./data/js/zones.csv
    python tzgen.py
    ${compress_cmd} f ./data/js/tz.json
    rm -f ./data/js/tz.json ./data/js/zones.csv
else
    unzip -o -d ./data/ data.zip "js/tz.json.${compressor}"
fi


cd ./data
zip --filesync -r -0 --quiet ../data.zip ./*
cd ..
rm -r ./data

mv -f newetags.txt $tags

echo "Content of data.zip file should be used to create LittleFS image"