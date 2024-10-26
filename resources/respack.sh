#!/usr/bin/env bash


# etag file
tags=etags.txt
compressor="gz"
compress_cmd="zopfli"
compress_args=""
dst="../data"

refresh_rq=0
tzcsv=https:/raw.githubusercontent.com/nayarsystems/posix_tz_db/master/zones.csv

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
    if [ ! -f   ${dst}/${res}.${compressor} ] || [ html/${res} -nt   ${dst}/${res}.${compressor} ] ; then
        cp html/${res}   ${dst}/${res}
        ${compress_cmd}    ${dst}/${res} && touch -r html/${res}   ${dst}/${res}.${compressor}
    fi
}

echo "Preparing resources for EmbUI FS image" 

mkdir -p ${dst}/css ${dst}/js
cat html/css/pure*.css html/css/grids*.css > ${dst}/css/pure.css
${compress_cmd}  ${dst}/css/pure.css
cat html/css/*_default.css > ${dst}/css/style.css
${compress_cmd}  ${dst}/css/style.css
cat html/css/*_light.css > ${dst}/css/style_light.css
${compress_cmd}  ${dst}/css/style_light.css
cat html/css/*_dark.css > ${dst}/css/style_dark.css
${compress_cmd}  ${dst}/css/style_dark.css

cp -u html/css/*.jpg ${dst}/css/
cp -u html/css/*.webp ${dst}/css/
cp -u html/css/*.svg* ${dst}/css/

echo "Packing EmbUI js" 
embui_js="dyncss.js lib.js maker.js"
# combine and compress js files in one bundle
for f in ${embui_js}
do
    cat html/js/${f} >> ${dst}/js/embui.js
done
${compress_cmd}  ${dst}/js/embui.js

cp -u html/js/lodash.custom.js* ${dst}/js/

echo "Packing static files"
# static gz files
static_gz_files='js/ui_embui.json js/ui_embui.i18n.json js/ui_embui.lang.json index.html favicon.ico'
for f in ${static_gz_files}
do
    updlocalarchive $f
done

echo "Update TZ"
# update TZ info
if freshtag ${tzcsv} || [ $refresh_rq -eq 1 ] ; then
    echo "Updating TZ info"
    echo '"label","value"' > ${dst}/js/zones.csv
    curl -sL $tzcsv >> ${dst}/js/zones.csv
    python tzgen.py
    ${compress_cmd} -f ${dst}/js/tz.json
    rm -f ${dst}/js/tz.json ${dst}/js/zones.csv
fi



mv -f newetags.txt $tags
