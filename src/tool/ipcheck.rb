#!/usr/bin/env ruby
# A script to check which characters play from the same IP address
# (c) 2009  Bjorn Lindeijer
#
# Summary of how it works:
#
#  Character -> Account
#  Account -> IP
#  IP -> Accounts
#  Accounts -> Characters
#

if ARGV.length != 1
    puts "Usage: ipcheck.rb character_name"
    exit 1
else
    $search_character = ARGV[0]
    puts "Searching for character #{$search_character}"
end

class Character
    attr_reader :name, :account_id

    def initialize(name, account_id)
        @name = name
        @account_id = account_id
    end
end

class Account
    attr_reader :id, :ip, :last_login

    def initialize(id, ip, last_login)
        @id = id
        @ip = ip
        @last_login = last_login
    end
end

accounts = Array.new
characters = Array.new

File.open("save/athena.txt", "r") do |f|
    f.each_line do |line|
        split = line.split("\t")
        if split.length >= 3
            account_id, char_index = split[1].split(',')
            character_name = split[2]
            characters.push(Character.new(character_name, account_id))
            if $search_character.casecmp(character_name) == 0
                puts "Searching for account #{account_id}"
                $search_account_id = account_id
            end
        end
    end
end

if not $search_account_id
    puts "Error: character #{$search_character} not found!"
    exit 1
end

File.open("save/account.txt", "r") do |f|
    f.each_line do |line|
        split = line.split("\t")
        if split.length >= 11
            account_id, last_login, ip = split[0], split[3], split[10]
            accounts.push(Account.new(account_id, ip, last_login))
            if $search_account_id == account_id
                puts "Searching for IP #{ip}"
                $search_ip = ip
            end
        end
    end
end

accounts.find_all { |a| a.ip == $search_ip }.each do |a|
    puts "Characters for account #{a.id} (last login #{a.last_login}):"
    characters.find_all { |c| c.account_id == a.id }.each do |c|
        puts "  #{c.name}"
    end
end
