*************************** NOTICE ***************************
**************************************************************

This program is free to use for any non-commercial private RO
server running an Athena based server, and is povided as-is.
The author of this program is only responsible for errors
that occur only to this program while it is in operation,
provided the program has not been modified in any way. If you
come across any errors or would like to leave comments and/or
suggestions about this program, please feel free to notify the
author of this program at spamrat42@gmail.com.

Please make sure to create a backup of your current database
files and your data folder prior to using this program.

==============================================================

This package contains the following files...

ADE.ini
Athena Database Editor.exe
history.txt
ID Helper.txt
readme.txt

sample data (empty folder)

sample db (folder)
  card_labels.txt
  item_descriptions.txt

sample mobs (empty folder)
  
==============================================================
= NOTE: All packages from 1.4.1 and newer will not include the
=	files from the latest eAthena server distribution. So
=	make sure to copy your current files from your server
=	into the appropriate folders listed above, or set the
=	correct paths in the ADE.ini file.
==============================================================

Thank you for downloading this tool I wrote for the eAthena
817 Text based server, but it should work even with the more
recent versions like eAthena 1.0 RC5. I hope you find this
tool to be useful.

The files in the db folder that come with this program are
custom files that are to be used with ADE. To change the
directory that this program looks in when opening or saving
files, open the ADE.ini file and enter the full or relative
path. The ADE.ini when first installed will already be
pointing to the relative folders listed above. You can also
simply copy your current db files to those folders, allowing
you to edit them without editing the db files you're
currently using.

All entries that have been commented out will not be read,
and won't be carried over into the new DB files. So please
make sure to back them up before using this program for the
first time. I will eventually add support for handling
commented lines in future versions.

One of the main features of this program will let you create
updated files for the client's data folder as well as update
the other relative db files for the server. When using this
feature, the following files will be created...

DB Folder - Server Files
================================
class_equip_db.txt
item_value_db.txt

Data Folder - Client Files
================================
cardpostfixnametable.txt
cardprefixnametable.txt
idnum2itemdesctable.txt
idnum2itemdisplaynametable.txt
itemslotcounttable.txt
num2itemdesctable.txt
num2itemdisplaynametable.txt

Note: There is a checkbox that will say to click it if the
      descriptions are garbled. Please only check after
      trying the default description files it produces.

If there are any files you think that were left out
that should have been updated as well for the client's
data folder, please let me know at the email address
provided above.

The item_descriptions.txt file is where all of the
descriptions are stored. Please note if you're manually
changing something in it, that you should the » character
for a new line. (Use ALT + 175 to type it.) It should
currently have the majority of the basic descriptions.
You won't need to add how much defense or the attack
power of equipment, as it will automatically be added
when the files are created. (It gets the values directly
from the item db files.)

The card_labels.txt file is where all the card labels are
stored. If you already have a custom set of card labels,
just copy the cardprefixnametable.txt file from your data
folder into the db folder and rename it to card_labels.txt.
Any missing labels will or blank labels will result in the
default label of "Carded" as a prefix.

I will eventually try to update this program to try
including other db files as well as to help make sure
it works with all versions of the eAthena text based
server from 817 to the current version. I'll also try
to add more support for other files that are also needed
to be generated for the client's data folder.

==============================================================
= NOTE 2: The item_descriptions.txt file that comes with this
=         package has incorrect and blank entries. Please make
=         sure to go over all the descriptions if you plan on
=         using it to generate description files for your
=         server's data folder.
==============================================================
