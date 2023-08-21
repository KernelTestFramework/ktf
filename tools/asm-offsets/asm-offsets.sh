#!/bin/bash

INPUT=$1
OUTPUT=$2

cat <<EOT > "$OUTPUT"
/* This file has been automatically generated from $INPUT. */

#ifndef KTF_ASM_OFFSETS_H
#define KTF_ASM_OFFSETS_H

$(awk -F '@@' '/\.ascii "@@/ {print $2}' "$INPUT" | tr -d '$')

#endif /* KTF_ASM_OFFSETS_H */
EOT
