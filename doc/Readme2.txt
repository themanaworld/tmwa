      ______  __    __                                 
     /\  _  \/\ \__/\ \                                
   __\ \ \L\ \ \ ,_\ \ \___      __    ___      __     
 /'__`\ \  __ \ \ \/\ \  _ `\  /'__`\/' _ `\  /'__`\   
/\  __/\ \ \/\ \ \ \_\ \ \ \ \/\  __//\ \/\ \/\ \L\.\_ 
\ \____\\ \_\ \_\ \__\\ \_\ \_\ \____\ \_\ \_\ \__/.\_\
 \/____/ \/_/\/_/\/__/ \/_/\/_/\/____/\/_/\/_/\/__/\/_/
  _   _   _   _   _   _   _     _   _   _   _   _   _
 / \ / \ / \ / \ / \ / \ / \   / \ / \ / \ / \ / \ / \ 
( e | n | g | l | i | s | h ) ( A | t | h | e | n | a )
 \_/ \_/ \_/ \_/ \_/ \_/ \_/   \_/ \_/ \_/ \_/ \_/ \_/


--------------------------------------------------------------
eAthena VERSION
--------------------------------------------------------------
Version 1.0.0 Release Candidate 3
--------------------------------------------------------------
This is a release candidate to iron out any bugs before
official release.
--------------------------------------------------------------
Caution : Make sure you are giving your users account id's 
greater than 2000000, failure to do so can cause a number of
problems.
--------------------------------------------------------------
eATHENA README
--------------------------------------------------------------
Thank you for downloading eAthena.
We hope that you'll enjoy using this software.
--------------------------------------------------------------
--------------------------------------------------------------
eAthena Features
--------------------------------------------------------------
> eAthena Completed Features
--------------------------------------------------------------
- PvP (Player VS Player)
- GvG (Guild VS Guild)
- qPets (Cute Pets)
- Monster skills
- 2-2 Jobs (Alternate 2nd Jobs)
- SuperNovice (Alternate 1st Job)
- WoE (War of Emperium)
- Remote administration of accounts (ladmin softwares)
- Pet-Equipped Mobs
- Management of day/night
--------------------------------------------------------------
> eAthena Incompleted Features (List)
--------------------------------------------------------------
- Advanced Classes (Upper Jobs)
- Baby Classes (Baby Jobs)
- Pet Skills
- Equipment Breaking System
- PK Server Mode
--------------------------------------------------------------
> eAthena Features - Estimated completion rate (Detailed)
--------------------------------------------------------------
- Guild Wars (War of Emperium) - 100% Complete (4/4)
- Novice Skills - 100% Complete:(3/3)
- 1st Job Skills - 100%
  * Swordsman - 100% Complete:(10/10)
  * Acolyte - 100% Complete:(16/16)
  * Archer - 100% Complete:(7/7)
  * Magician - 100% Complete:(14/14)
  * Merchant - 100% Complete:(10/10)
  * Thief - 100% Complete:(10/10)
- 2-1 Skills (2nd Job Skills) - 100%
  * Knight - 100% Complete:(10/10)
  * Priest - 100% Complete:(18/18)
  * BlackSmith - 100% Complete:(21/21)
  * Wizard - 100% Complete:(13/13)
  * Assassin - 100% Complete:(10/10)
  * Hunter - 100% Complete:(17/17)
- 2-2 Skills (Alternate 2nd Job Skills) - 90.5%
  * Crusader - 100% Complete:(18/18)
  * Monk - 100% Complete:(15/15)
  * Alchemist - 54% Complete:(12/22) Complete:(10/22) (Blame Gravity For The Missing Skills)
  * Sage - 100% Complete:(20/20)
  * Rogue - 80% Complete:(12/15) Incomplete:(3/15)
  * Bard - 100% Complete:(16/16)
  * Dancer - 100% Complete:(16/16)
- 2-1-1 Skills (Advanced 2-1 Job Skills) - 80.5%
  * Lord Knight - 94% - Complete:(7/8) Incomplete:(1/8)
  * High Priest - 66% - Complete:(1/3) Incomplete:(2/3)
  * High Wizard - 100% Complete:(4/4)
  * Whitesmith - 75% Complete:(3/4) Incomplete:(1/4)
  * Sniper - 88% - Complete:(3/4) Incomplete:(1/4)
  * Assassin Cross - 60% Complete:(3/5) Incomplete:(2/5)
- 2-2-1 Skills (Advanced 2-2 Job Skills) - 47%
  * Paladin - 65%
  * Champion - 100%
  * Professor - 100%
  * Stalker - 65%
  * Creator - 0%
  * Clown - 0%
  * Gypsy - 0%
- Pet Skills - 95%
  * Loot Skills - 100%
  * Buff Skills - 100%
  * Attack Skills - 90%
  * Assist Skills - 100%
- Equipment Breaking System - 50%
  * Weapon Breaking - 85%
  * Armor Breaking - 80%
  * Equipment Repair - 80%
- PK Server Mode - ??%
  * Need more info on this mode to be called complete.
--------------------------------------------------------------
> eAthena - What we currently do not have
--------------------------------------------------------------
- Wedding System (In Progress!)
--------------------------------------------------------------
--------------------------------------------------------------


--------------------------------------------------------------
--------------------------------------------------------------
System Requirements
--------------------------------------------------------------
> Minimum System Requirements
--------------------------------------------------------------
800Mhz Processor
256MB RAM

NOTE: For this setup, it's recommended to not load as many maps
and spawn as many monsters as it comes default with eAthena.
To do this, comment off unused maps and control monster spawn
rate using mob_count in battle_athena.conf.

--------------------------------------------------------------
> Recommended System Requirements
--------------------------------------------------------------
1+Ghz Processor
512+MB RAM

NOTE: For this setup, generally, running the eAthena default
should be fine. If you have alot of extra RAM, you might even
consider raising the mob_count in battle_athena.conf so that
there will be more than normal monsters in your server
--------------------------------------------------------------
--------------------------------------------------------------


--------------------------------------------------------------
--------------------------------------------------------------
Setting up eAthena
--------------------------------------------------------------
> Setting up a new eAthena
--------------------------------------------------------------
1. Run the setupwizard program. This will assist you in
   configuring Athena for the first time. Just follow the
   instructions in the program.
2. Edit motd.txt, grf-files.txt & .conf files as neccessary
3. If you want to add a user, go to tools and use the AddUser
   tool
4. Run the login-server, char-server and map-server in the
   main Athena directory
5. You're done!

--------------------------------------------------------------
> Upgrading from older eAthena:
--------------------------------------------------------------
- From a release newer than 953 or 817:
1. Transfer your old /save/ folder contents (Athena.txt,
   Account.txt, Storage.txt, etc.) to the new /safe/ folder
2. Transfer/Edit grf-files.txt, gm-account.txt and motd.txt as
   neccessary
3. Transfer any extra custom NPCs to the /npc/ folder and
   edit if neccessary
4. Edit .conf files to setup your server
5. You're done!

NOTE: It is not recommended to transfer your DB files
as the DB files are constantly updated by the eAthena/jAthena
team. Just read your changes to the new DB files instead of
replacing the new DB file with your old one.
--------------------------------------------------------------
- From a release older than 670:
1. Transfer your old data files (Athena.txt, Account.txt, 
   Storage.txt, Pet.txt, etc) into the /save/ folder
2. Transfer/Edit grf-files.txt, gm-account.txt as neccessary
3. Transfer any extra custom NPCs/DBs to the new /npc/ folder
   and edit if neccessary
4. Edit .conf files to setup your server
5. You're done!

NOTE: It is not recommended to transfer your .conf files from
your old eAthena setup to this new one because there are always
changes and new additions to these .conf files all the time

NOTE 2: It is also not recommended to transfer your DB files
as the DB files are constantly updated by the eAthena/jAthena
team. Just readd your changes to the new DB files instead of
replacing the new DB file with your old one.
--------------------------------------------------------------
--------------------------------------------------------------


--------------------------------------------------------------
--------------------------------------------------------------
Playing eAthena
--------------------------------------------------------------
1. Make sure all the files are setup properly (conf files)
2. Load eAthena
	2a. Run login-server
	2b. Run char-server
	2c. Run map-server
3. Wait for map-server to finish loading.
4. Play using a custom XML.
--------------------------------------------------------------
--------------------------------------------------------------


--------------------------------------------------------------
--------------------------------------------------------------
Frequently Asked Questions
--------------------------------------------------------------

A: You're missing the cygwin dlls. Please get the latest dll at:
   http://www.cygwin.com/snapshot. If you're unsure, asking 
   around in our IRC chatroom will get you around too, but
   always remember - use common sense and search before asking.

Q: My map-server won't load! It appears to be loading things
   before it dissapeared suddenly! HELP!
A: Use command line to load map-server. It should tell you the
   error. If you're missing a map, update your kRO Sakray or
   uncomment the map from map_athena.conf. If you have an
   errornous NPC, fix it or comment it off. Anything other than
   that, feel free to ask around.

Q: My map-server failed to load 'adata.grf'! Where do I find
   this adata.grf? My map-server won't load without it!
A: The error that caused the map-server to not load is not the
   adata.grf. You do NOT need to have adata.grf so that the map
   server can load. The error is probably something else.

Q: All 3 of the servers are loaded, but I am still having
   problems accessing it! What do I do?
A: First, check the IPs in map_athena.conf and char_athena.conf.
   If that's correct, check the ports to make sure they match.
   If that's correct too, you probably cannot handle the server
   load. Lower the monster spawning rate using mob_count in
   battle_athena.conf and it should be fine.

Q: You said eAthena has 2-2 skills. Well, I tried it and there
   are no 2-2 skills when I use 2-2 characters! You lie!
A: No, I did not. You have an outdated Ragexe/Sakexe that does
   not support 2-2 skills. Update it to a later one which does.

Q: How do I start Guild Wars/War of Emperium??? HELP!!!
A: Read the help.txt for full list of commands that GMs can use.
   @agitstart and @agitend is also listed there as commands to
   start and end Guild Wars.

Q: My Ragnarok Online crashed while playing with eAthena! What
   do I do now?
A: Well, if your Ragnarok crashes, it's most probably not
   anything to do with eAthena. Something is wrong with your
   Ragnarok installation. Try reinstalling or updating.

Q: Is eAthena compatible with mySQL? Can I use mySQL as the DB
   instead of using text files?
A: Yes, eAthena is compatible with mySQL. A tutorial on how to
   setup this is coming soon.

Q: Is eAthena compatible with msSQL? Can I use msSQL as the DB
   instead of using text files?
A: No, eAthena is not compatible with msSQL. You cannot use
   msSQL with eAthena.

Q: I found a bug! Where do I report it?
A: Drop the developers a line at the IRC chatroom. Or just post
   it in the bug report forum. We check them out too. :)

Q: I know alot of C and I'm able to help improve eAthena and
   add new features. How can I join your development team?
A: Try talking to one of the current developers in the eAthena
   channel.

Q: What is this AthenaAdvanced/omniAthena that I keep hearing of?
A: Huh? What's that?

Q: Why does eAthena crash, and how come you can't prevent this?
A: eAthena is a working in progress we never stated eAthena is
   bug free, most likely no software is bugfree, if they state
   this they are lying sons of beeps and you should sue them
   for false advertisement.
--------------------------------------------------------------
--------------------------------------------------------------


--------------------------------------------------------------
--------------------------------------------------------------
Support
--------------------------------------------------------------
For support, first check the Frequent Asked Questions.
If the question doesn't exist, check the forums. Search before
posting to see if the question had been asked and answered.
If not, post and we'll try and help you.
Our forums is located at: http://eathena.deltaanime.net
You can also visit our IRC chatroom and ask your questions
there.
Our IRC chatroom is located at: irc.deltaanime.net #athena 
--------------------------------------------------------------
--------------------------------------------------------------


--------------------------------------------------------------
--------------------------------------------------------------
Development
--------------------------------------------------------------
eAthena is based on the original Athena, by the Japanese
Development Team. A big thanks to them for working hard on
Athena.

The eAthena Development Team:
AppleGirl
Akaru
Evera/Lorri
Wizputer
Valaris
Yor
Kalaspuff
Syrus22
Darkchild
CLOWNISIUS
RichG
Kashy
Lupus

NOTE: Sorry if I missed anyone while writing this. If you are
in the team but not included in this list, PM me.
--------------------------------------------------------------
--------------------------------------------------------------


--------------------------------------------------------------
--------------------------------------------------------------
Special Thanks and Shoutouts!
--------------------------------------------------------------
The eAthena development team would like to specially thank:
- the Japanese Development Team. A big thanks to them for
  working hard on Athena which eAthena is based on.
- RoVeRT, our ex-developer. eAthena wouldn't have this many
  features without him!
- sara-chan, who had also contributed alot to the development
  of eAthena! Thanks!
- Gravity, for making Ragnarok Online. eAthena wouldn't even
  exist if it wasn't for them!
- ASB for hosting our forum section :)
- Deltaanime for hosting our new official forums :)
- And others who gave continuously endless support to eAthena!
- fov and Kinko for their bugfixes.

NOTE: Sorry if I missed anyone while writing this. If you are
in the team but not included in this list, PM me.
--------------------------------------------------------------
--------------------------------------------------------------


--------------------------------------------------------------
--------------------------------------------------------------
License:
--------------------------------------------------------------
eAthena is licensed under the GPL. Please refer to LICENSE in
your Athena folder for the full GPL License statement.
--------------------------------------------------------------
--------------------------------------------------------------
