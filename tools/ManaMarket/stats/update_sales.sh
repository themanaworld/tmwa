#!/bin/bash
cd /home/tmwserver/ManaMarket/stats
python /home/tmwserver/ManaMarket/stats/process_salelog/main.py /home/tmwserver/ManaMarket/data/logs/sale.log /var/www/tmw-homepage/server/manamarket.html
python /home/tmwserver/ManaMarket/stats/process_salelog/main_stat.py /home/tmwserver/ManaMarket/data/logs/sale.log /var/www/tmw-homepage/server/manamarket_stats.html
chmod 644 /var/www/tmw-homepage/server/manamarket.html /var/www/tmw-homepage/server/manamarket_stats.html
