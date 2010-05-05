#!/bin/bash

ATHENATXT="save/athena.txt"
DEST="$HOME/public_html/stats"

:> ${DEST}/top-money.txt
(echo "TOP 50 RICHEST PLAYERS"; echo ""; cat ${ATHENATXT} \
| awk -F , '{sub(/^[0-9] */,"",$2);sub(/ *[0-9]$/,"",$2); sub(/ *[0-9]+$/,"",$6);print $6,$2}' \
| sort -nr | head -n 50 | awk '{first=$1;$1="";print $0, "("first")"}' | nl -s ". "; echo ""; echo "Generated at `date`") >> ${DEST}/top-money.txt

:> ${DEST}/top-highest-level.txt
(echo "TOP 50 HIGHEST LEVEL PLAYERS"; echo ""; cat ${ATHENATXT} \
| awk -F , '{sub(/^[0-9] */,"",$2);sub(/ *[0-9]$/,"",$2); print $3,$2}' \
| sort -nr | head -n 50 | awk '{first=$1;$1="";print $0, "("first")"}' | nl -s ". "; echo ""; echo "Generated at `date`") >> ${DEST}/top-highest-level.txt

:> ${DEST}/top-highest-str.txt
(echo "TOP 50 HIGHEST LEVEL PLAYERS"; echo ""; cat ${ATHENATXT} \
| awk -F , '{sub(/^[0-9] */,"",$2);sub(/ *[0-9]$/,"",$2); sub(/^[0-9]+[[:space:]]*/,"",$9);print $9,$2}' \
| sort -nr | head -n 50 | awk '{first=$1;$1="";print $0, "("first")"}' | nl -s ". "; echo ""; echo "Generated at `date`") >> ${DEST}/top-highest-str.txt

:> ${DEST}/top-highest-agi.txt
(echo "TOP 50 HIGHEST LEVEL PLAYERS"; echo ""; cat ${ATHENATXT} \
| awk -F , '{sub(/^[0-9] */,"",$2);sub(/ *[0-9]$/,"",$2); print $10,$2}' \
| sort -nr | head -n 50 | awk '{first=$1;$1="";print $0, "("first")"}' | nl -s ". "; echo ""; echo "Generated at `date`") >> ${DEST}/top-highest-agi.txt

:> ${DEST}/top-highest-vit.txt
(echo "TOP 50 HIGHEST LEVEL PLAYERS"; echo ""; cat ${ATHENATXT} \
| awk -F , '{sub(/^[0-9] */,"",$2);sub(/ *[0-9]$/,"",$2); print $11,$2}' \
| sort -nr | head -n 50 | awk '{first=$1;$1="";print $0, "("first")"}' | nl -s ". "; echo ""; echo "Generated at `date`") >> ${DEST}/top-highest-vit.txt

:> ${DEST}/top-highest-int.txt
(echo "TOP 50 HIGHEST LEVEL PLAYERS"; echo ""; cat ${ATHENATXT} \
| awk -F , '{sub(/^[0-9] */,"",$2);sub(/ *[0-9]$/,"",$2); print $12,$2}' \
| sort -nr | head -n 50 | awk '{first=$1;$1="";print $0, "("first")"}' | nl -s ". "; echo ""; echo "Generated at `date`") >> ${DEST}/top-highest-int.txt

:> ${DEST}/top-highest-dex.txt
(echo "TOP 50 HIGHEST LEVEL PLAYERS"; echo ""; cat ${ATHENATXT} \
| awk -F , '{sub(/^[0-9] */,"",$2);sub(/ *[0-9]$/,"",$2); print $13,$2}' \
| sort -nr | head -n 50 | awk '{first=$1;$1="";print $0, "("first")"}' | nl -s ". "; echo ""; echo "Generated at `date`") >> ${DEST}/top-highest-dex.txt

:> ${DEST}/top-highest-luk.txt
(echo "TOP 50 HIGHEST LEVEL PLAYERS"; echo ""; cat ${ATHENATXT} \
| awk -F , '{sub(/^[0-9] */,"",$2);sub(/ *[0-9]$/,"",$2); sub(/[[:space:]]*[0-9]+$/,"",$14);print $14,$2}' \
| sort -nr | head -n 50 | awk '{first=$1;$1="";print $0, "("first")"}' | nl -s ". "; echo ""; echo "Generated at `date`") >> ${DEST}/top-highest-luk.txt
