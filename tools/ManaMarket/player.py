#!/usr/bin/python
"""
    Copyright 2011, Dipesh Amin <yaypunkrock@gmail.com>
    Copyright 2011, Stefan Beller <stefanbeller@googlemail.com>

    This file is part of tradey, a trading bot in the mana world
    see www.themanaworld.org
"""

import copy

class Item:
    pass

class Player:
    def __init__(self, name):
        self.inventory = {}
        self.name = name
        self.id = 0
        self.sex = 0
        self.map = ""
        self.x = 0
        self.y = 0

        self.EXP = 0
        self.MONEY = 0
        self.WEIGHT = 0
        self.MaxWEIGHT = 0

    def find_inventory_index(self, item_id):
        for item in self.inventory:
            if item > 1:
                if self.inventory[item].itemId == item_id:
                    return item
        return -10 # Not found - bug somewhere!

    def remove_item(self, index, amount):
        if index in self.inventory:
            self.inventory[index].amount -= amount
            if self.inventory[index].amount == 0:
                del self.inventory[index]

    def check_inventory(self, user_tree, sale_tree):
        # Check the inventory state.
        test_node = copy.deepcopy(self.inventory)
        for elem in sale_tree.root:
            item_found = False
            for item in test_node:
                if int(elem.get('itemId')) == test_node[item].itemId \
                and int(elem.get('amount')) <= test_node[item].amount:
                    test_node[item].amount -= int(elem.get('amount'))
                    if test_node[item].amount == 0:
                        del test_node[item]
                    item_found = True
                    break

            if not item_found:
                return "Server and client inventory out of sync."

        total_money = 0
        for user in user_tree.root:
            total_money += int(user.get('money'))

        if total_money != self.MONEY:
            return "Server and client money out of sync. Market: %s Player %s" % (total_money, self.MONEY)

        return 0

if __name__ == '__main__':
    print "Do not run this file directly. Run main.py"
