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

ItemDB = utils.ItemDB()

def main():
    in_file = sys.argv[1]
    out_file = sys.argv[2]
    locale.setlocale(locale.LC_ALL, '')

    out_obj = open(out_file, 'w')
    in_obj = open(in_file, 'r')

    out_obj.write('<html> \n')
    out_obj.write('<body> \n')
    out_obj.write('<head> \n')
    out_obj.write('<title>ManaMarket Sales</title> \n')
    out_obj.write('<h2>ManaMarket Sales</h2>')
    out_obj.write('<table border="1" cellpadding="5" cellspacing="0"> \n')
    out_obj.write('<tr bgcolor="6699FF"> <td>Item Name</td> <td>Amount</td> <td>Price</td> <td>Time</td> </tr>\n')

    sales = in_obj.readlines()

    sale_total = 0
    items_sold = 0
    for line in sales:
        line = line.split()
        t_time = time.gmtime(float(line[3]))
        unit_price = int(line[2])/int(line[1])
        out_obj.write('<tr> <td>'+ItemDB.getItem(int(line[0])).name+'</td> <td>'+locale.format("%d", int(line[1]), grouping=True)+'</td> <td>'+locale.format("%d", unit_price, grouping=True)+'</td> <td>'+time.asctime(t_time)+'</td> </tr>\n') 
        sale_total += int(line[2])       
        items_sold += int(line[1])

    out_obj.write('</table> \n')
    out_obj.write('</br><b>Total sales: '+str(sale_total)+' GP</b> </br>')
    out_obj.write('<b>Total number of items sold: '+str(items_sold)+'</b></br>')
    out_obj.write('Updated: '+time.asctime(time.gmtime()))
    out_obj.write('</body></html> \n')
    in_obj.close()
    out_obj.close()


if __name__ == '__main__':
    main()
