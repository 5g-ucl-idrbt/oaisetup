#!/bin/bash

set -euo pipefail

PREFIX=/opt/oai-nr-ue

# Based another env var, pick one template to use
if [[ -v USE_NFAPI ]]; then cp $PREFIX/etc/nr-ue.nfapi.conf $PREFIX/etc/nr-ue.conf; fi
# Sometimes, the templates are not enough. We mount a conf file on $PREFIX/etc. It can be a template itself.
if [[ -v USE_VOLUMED_CONF ]]; then cp $PREFIX/etc/mounted.conf $PREFIX/etc/nr-ue.conf; fi
# if none, pick the default
if [ ! -f $PREFIX/etc/nr-ue.conf ]; then cp $PREFIX/etc/nr-ue-sim.conf $PREFIX/etc/nr-ue.conf; fi

# Only this template will be manipulated
CONFIG_FILES=`ls $PREFIX/etc/nr-ue.conf || true`

for c in ${CONFIG_FILES}; do
    # Sometimes templates have no pattern to be replaced.
    if ! grep -oP '@[a-zA-Z0-9_]+@' ${c}; then
        echo "Configuration is already set"
        break
    fi

    # grep variable names (format: ${VAR}) from template to be rendered
    VARS=$(grep -oP '@[a-zA-Z0-9_]+@' ${c} | sort | uniq | xargs)

    # create sed expressions for substituting each occurrence of ${VAR}
    # with the value of the environment variable "VAR"
    EXPRESSIONS=""
    for v in ${VARS}; do
        NEW_VAR=`echo $v | sed -e "s#@##g"`
        if [[ "${!NEW_VAR}x" == "x" ]]; then
            echo "Error: Environment variable '${NEW_VAR}' is not set." \
                "Config file '$(basename $c)' requires all of $VARS."
            exit 1
        fi
        EXPRESSIONS="${EXPRESSIONS};s|${v}|${!NEW_VAR}|g"
    done
    EXPRESSIONS="${EXPRESSIONS#';'}"

    # render template and inline replace config file
    sed -i "${EXPRESSIONS}" ${c}
done

# Load the USRP binaries
if [[ -v USE_B2XX ]]; then
    $PREFIX/bin/uhd_images_downloader.py -t b2xx
elif [[ -v USE_X3XX ]]; then
    $PREFIX/bin/uhd_images_downloader.py -t x3xx
elif [[ -v USE_N3XX ]]; then
    $PREFIX/bin/uhd_images_downloader.py -t n3xx
fi

# in case we have conf file, append
new_args=()
while [[ $# -gt 0 ]]; do
  new_args+=("$1")
  shift
done

echo "=================================="
echo "== Starting NR UE soft modem"
if [[ -v USE_ADDITIONAL_OPTIONS ]]; then
    echo "Additional option(s): ${USE_ADDITIONAL_OPTIONS}"
    for word in ${USE_ADDITIONAL_OPTIONS}; do
        new_args+=("$word")
    done
    echo "${new_args[@]}"
    exec "${new_args[@]}"
else
    echo "${new_args[@]}"
    exec "${new_args[@]}"
fi
