#!/usr/bin/python
"""

    Copyright 2011, Dipesh Amin <yaypunkrock@gmail.com>
    Copyright 2011, Stefan Beller <stefanbeller@googlemail.com>

    tradey, a package, which implements an Automated Market Bot for "The Mana World" a 2D MMORPG.

"""

import logging
import socket
import sys
import time
import string
import utils
import locale

class SaleStat:
    pass

ItemDB = utils.ItemDB()

def main():
    in_file = sys.argv[1]
    out_file = sys.argv[2]
    locale.setlocale(locale.LC_ALL, '')

    out_obj = open(out_file, 'w')
    in_obj = open(in_file, 'r')
    
    out_obj.write('<html> \n \
    <head> \n \
    <title>ManaMarket Statistics</title> \n \
    <style type="text/css"> \n \
    td, th { border: 1px solid black } \n \
    table {border: 1px solid black} \n \
    </style> \n \
    </head> \n \
    <body> \n \
    <h2> ManaMarket Statistics </h2> \n \
    <table cellpadding="5" cellspacing="0"> \n \
    <tr> \n \
    <th rowspan="2"> Item Name</th> \n \
    <th rowspan="2"> Total Amount Sold</th> \n \
    <th colspan="5">Price</th> \n \
    <th rowspan="2">Last Sold</th> \n \
    </tr> \n \
    <tr> \n \
    <td>Min</td> \n \
    <td>Max</td> \n \
    <td>Average (Week)</td> \n \
    <td>Average (Month)</td> \n \
    <td>Average (Overall)</td> \n \
    </tr> \n ')

    sales = in_obj.readlines()

    sale_dict = {}

    # Put sales date into a dict.
    sale_total = 0
    for line in sales:
        line = line.split()
        item_id = int(line[0])
        name = ItemDB.getItem(int(line[0])).name
        amount = int(line[1])
        price = int(line[2])
        t_time = float(line[3])
        sale_total += int(line[2])
        
        if item_id not in sale_dict:
            sale_dict[item_id] = []

        sale_dict[item_id].append([amount, price, t_time])
    
    str_list = []
    # calculate the stats
    for item in sale_dict:
	average_week = 0
        average_week_amount = 0
        average_month = 0
        average_month_amount = 0
        average_all_time = 0
        average_all_time_amount = 0
        min_price = 0
        max_price = 0
        last_sold = 0

        for n in range(len(sale_dict[item])):

            #out_obj.write('<tr>')

            if min_price > sale_dict[item][n][1]/sale_dict[item][n][0] or min_price == 0:
                min_price = sale_dict[item][n][1]/sale_dict[item][n][0]

            if max_price < sale_dict[item][n][1]/sale_dict[item][n][0] or max_price == 0:
                max_price = sale_dict[item][n][1]/sale_dict[item][n][0]

            if last_sold < sale_dict[item][n][2] or last_sold == 0:
                last_sold = sale_dict[item][n][2]

            if (time.time()-sale_dict[item][n][2]) < 7*24*60*60:
                average_week_amount += sale_dict[item][n][0]
                average_week += sale_dict[item][n][1]

            if (time.time()-sale_dict[item][n][2]) < 30*24*60*60:
                average_month_amount += sale_dict[item][n][0]
                average_month += sale_dict[item][n][1]

            
            average_all_time_amount += sale_dict[item][n][0]
            average_all_time += sale_dict[item][n][1]

        average_all_time /= average_all_time_amount
        if average_week_amount > 0:        
            average_week /= average_week_amount
        if average_month_amount > 0:
            average_month /= average_month_amount
        
        str_list.append([average_all_time_amount,'<td>'+ItemDB.getItem(item).name+'</td>'+'<td>'+locale.format("%d", average_all_time_amount, grouping=True)+'</td>'+ \
        '<td>'+ locale.format("%d", min_price, grouping=True) +'</td>'+'<td>'+ locale.format("%d", max_price, grouping=True) +'</td>'+'<td>'+ \
        locale.format("%d", average_week, grouping=True) +'</td>'+ '<td>'+ locale.format("%d", average_month, grouping=True) + \
        '</td>'+'<td>'+ locale.format("%d", average_all_time, grouping=True)+'</td>'+'<td>'+ time.asctime(time.gmtime(last_sold)) +'</td>'])

    while len(str_list) > 0:
        pos = 0
        max_amount = 0
        for m in range(len(str_list)):
            if max_amount < str_list[m][0] or max_amount == 0:
                max_amount = str_list[m][0]
                pos = m
        out_obj.write('<tr>')
        out_obj.write(str_list[pos][1])
        str_list.pop(pos)


        out_obj.write('</tr> \n')

    out_obj.write('</table> \n')
    out_obj.write('</br><b>Total sales: '+str(sale_total)+' GP</b> </br>')
    out_obj.write('Updated: '+time.asctime(time.gmtime())+'</br>')
    out_obj.write('For a detailed page showing all sales '+'<a href="manamarket.html">click here</a>')

    out_obj.write('</body></html> \n')
    in_obj.close()
    out_obj.close()


if __name__ == '__main__':
    main()
