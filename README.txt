README.txt
Charlie Milius
ComS 229
Sun Apr 19, 2015
Assignment 1.10 PC Equipment and Updated Combat

I included the Makefile with my .tar to compile the program as rogue.  
RUN INSTRUCTIONS:
$ make
$ ./rlg229

For this week's extension, I did not add any new files.  I just extended the files I have been using 
for the past two weeks.  I used Professor Sheaffer's solution to assignment 1.09 as a base for this 
week.

Most of my added code is in io.c, object.cpp, and move.c.

I took care of making a UI and object carry/equip in io.c.  I added two object pointer arrays to the pc 
struct, and then went to work on the interface.  When w, t, d, or x are pressed, a basic UI pops up over 
the top of the dungeon.  It will auto populate with anything that the PC is carrying, or has equipped.  If 
the slot has nothing in it, then two dash marks will be displayed.  In the bottom left of the window, you 
can also see which mode you are in.  There is a star next to the list of items you can interact with and by 
pressing the up and down keys, you can select which item you want to perform that action on.  The spacebar 
tells the program to do the action.  I am not making new items when I do this, I am just moving the location 
of the pointer.

WEAPON, OFFHAND, RANGED, ARMOR, HELMET, CLOAK, GLOVES, BOOTS, AMULET, LIGHT, and two for RING, are ‘numbered’ 
a–l respectively.

You can exit this menu by pressing the ESC key, or the same key you used to start the menu.

Drop will drop in all the places immediately around the PC, if all those are taken, it can no longer drop an 
item (I didn't think the PC would be throwing items all over the place).

I also added a status bar on the right side of the dungeon that tells PC's speed and HP statuses.  I couldn't 
think of how to do damage or anything else since they are dice values, so I didn't include them.  These stats 
are updated immediately after an item is equipped/unequipped.

Combat will work if you try to walk on another monster.  You will both standoff and inflict damage on one another 
(given that the NPC has a turn again before you do).  Each HP value is updated and whoever reaches zero first loses. 
Having HP has made this game much more enjoyable and less stressful to play! :)  NPC characters will only attack 
the PC, so they no longer kill one another in an effort to get to you first.

Thanks for the assignment extension.  It really saved me this week.

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  