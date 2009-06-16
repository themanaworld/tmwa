#!/bin/bash

:> ~/public_html/stats/top-money.txt
(echo "TOP 50 RICHEST PLAYERS"; echo ""; cat tmwserver/save/athena.txt \
| awk -F , '{sub(/^[0-9] */,"",$2);sub(/ *[0-9]$/,"",$2); sub(/ *[0-9]+$/,"",$6);print $6,$2}' \
| sort -nr | head -n 50 | awk '{first=$1;$1="";print $0, "("first")"}' | nl -s ". "; echo ""; echo "Generated at `date`") >> ~/public_html/stats/top-money.txt

:> ~/public_html/stats/top-highest-level.txt
(echo "TOP 50 HIGHEST LEVEL PLAYERS"; echo ""; cat tmwserver/save/athena.txt \
| awk -F , '{sub(/^[0-9] */,"",$2);sub(/ *[0-9]$/,"",$2); print $3,$2}' \
| sort -nr | head -n 50 | awk '{first=$1;$1="";print $0, "("first")"}' | nl -s ". "; echo ""; echo "Generated at `date`") >> ~/public_html/stats/top-highest-level.txt

:> ~/public_html/stats/top-highest-str.txt
(echo "TOP 50 HIGHEST LEVEL PLAYERS"; echo ""; cat tmwserver/save/athena.txt \
| awk -F , '{sub(/^[0-9] */,"",$2);sub(/ *[0-9]$/,"",$2); sub(/^[0-9]+[[:space:]]*/,"",$9);print $9,$2}' \
| sort -nr | head -n 50 | awk '{first=$1;$1="";print $0, "("first")"}' | nl -s ". "; echo ""; echo "Generated at `date`") >> ~/public_html/stats/top-highest-str.txt

:> ~/public_html/stats/top-highest-agi.txt
(echo "TOP 50 HIGHEST LEVEL PLAYERS"; echo ""; cat tmwserver/save/athena.txt \
| awk -F , '{sub(/^[0-9] */,"",$2);sub(/ *[0-9]$/,"",$2); print $10,$2}' \
| sort -nr | head -n 50 | awk '{first=$1;$1="";print $0, "("first")"}' | nl -s ". "; echo ""; echo "Generated at `date`") >> ~/public_html/stats/top-highest-agi.txt

:> ~/public_html/stats/top-highest-vit.txt
(echo "TOP 50 HIGHEST LEVEL PLAYERS"; echo ""; cat tmwserver/save/athena.txt \
| awk -F , '{sub(/^[0-9] */,"",$2);sub(/ *[0-9]$/,"",$2); print $11,$2}' \
| sort -nr | head -n 50 | awk '{first=$1;$1="";print $0, "("first")"}' | nl -s ". "; echo ""; echo "Generated at `date`") >> ~/public_html/stats/top-highest-vit.txt

:> ~/public_html/stats/top-highest-int.txt
(echo "TOP 50 HIGHEST LEVEL PLAYERS"; echo ""; cat tmwserver/save/athena.txt \
| awk -F , '{sub(/^[0-9] */,"",$2);sub(/ *[0-9]$/,"",$2); print $12,$2}' \
| sort -nr | head -n 50 | awk '{first=$1;$1="";print $0, "("first")"}' | nl -s ". "; echo ""; echo "Generated at `date`") >> ~/public_html/stats/top-highest-int.txt

:> ~/public_html/stats/top-highest-dex.txt
(echo "TOP 50 HIGHEST LEVEL PLAYERS"; echo ""; cat tmwserver/save/athena.txt \
| awk -F , '{sub(/^[0-9] */,"",$2);sub(/ *[0-9]$/,"",$2); print $13,$2}' \
| sort -nr | head -n 50 | awk '{first=$1;$1="";print $0, "("first")"}' | nl -s ". "; echo ""; echo "Generated at `date`") >> ~/public_html/stats/top-highest-dex.txt

:> ~/public_html/stats/top-highest-luk.txt
(echo "TOP 50 HIGHEST LEVEL PLAYERS"; echo ""; cat tmwserver/save/athena.txt \
| awk -F , '{sub(/^[0-9] */,"",$2);sub(/ *[0-9]$/,"",$2); sub(/[[:space:]]*[0-9]+$/,"",$14);print $14,$2}' \
| sort -nr | head -n 50 | awk '{first=$1;$1="";print $0, "("first")"}' | nl -s ". "; echo ""; echo "Generated at `date`") >> ~/public_html/stats/top-highest-luk.txt
