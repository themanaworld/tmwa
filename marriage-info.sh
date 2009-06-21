#! /bin/sh

cd ~/tmwserver

cat save/athena.txt | sort -n > athena-sorted.txt

printf "Top married couples\n===================\n" > ~/public_html/stats/married.txt
cat athena-sorted.txt | ./marriage-info -c | sort -rn | awk '{printf "%d   %s  oo  %s  (%s (%s, %d), %s(%s, %d))\n", ++rank, $3, $6, $3, $4, $5, $6, $7, $8}' | tr '%' ' ' >> ~/public_html/stats/married.txt

printf "Top female singles\n==================\n" > ~/public_html/stats/female-singles.txt
cat athena-sorted.txt | ./marriage-info -f | sort -rn | awk '{printf "%d   %s(%d)\n", ++rank, $3, $1}' | tr '%' ' ' | head -50 >> ~/public_html/stats/female-singles.txt

printf "Top male singles\n===============\n" > ~/public_html/stats/male-singles.txt
cat athena-sorted.txt | ./marriage-info -m | sort -rn | awk '{printf "%d   %s(%d)\n", ++rank, $3, $1}' | tr '%' ' ' | head -50 >> ~/public_html/stats/male-singles.txt

printf "Top singles\n===============\n" > ~/public_html/stats/singles.txt
cat athena-sorted.txt | ./marriage-info -s | sort -rn | awk '{printf "%d   %s(%s, %d)\n", ++rank, $3, $4, $1}' | tr '%' ' ' | head -50 >> ~/public_html/stats/singles.txt

rm athena-sorted.txt

