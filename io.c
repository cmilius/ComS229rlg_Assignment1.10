#include <unistd.h>
#include <ncurses.h>

#include "io.h"
#include "move.h"
#include "object.h"

#include "dungeon.h"
#include "pc.h"
#include "character.h"
#include "descriptions.h"

/* We're going to be working in a standard 80x24 terminal, and, except when *
 * the PC is near the edges, we're going to restrict it to the centermost   *
 * 60x18, defining the center to be at (40,12).  So the PC can be in the    *
 * rectangle defined by the ordered pairs (11, 4) and (70, 21).  When the   *
 * PC leaves this zone, if already on the corresponding edge of the map,    *
 * nothing happens; otherwise, the map shifts by +-40 in x or +- 12 in y,   *
 * such that the PC is in the center 60x18 region.  Look mode will also     *
 * shift by 40x12 blocks.  Thus, the set of all possible dungeon positions  *
 * to correspond with the upper left corner of the dungeon are:             *
 *                                                                          *
 *   ( 0,  0), (40,  0), (80,  0)                                           *
 *   ( 0, 12), (40, 12), (80, 12)                                           *
 *   ( 0, 24), (40, 24), (80, 24)                                           *
 *   ( 0, 36), (40, 36), (80, 36)                                           *
 *   ( 0, 48), (40, 48), (80, 48)                                           *
 *   ( 0, 60), (40, 60), (80, 60)                                           *
 *   ( 0, 72), (40, 72), (80, 72)                                           */

void io_init_terminal(void)
{
  initscr();
  raw();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);
  start_color();
  init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
  init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
  init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
  init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
  init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
  init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
}

void io_reset_terminal(void)
{
  endwin();
}

void io_calculate_offset(dungeon_t *d)
{
  d->io_offset[dim_x] = ((d->pc.position[dim_x] - 20) / 40) * 40;
  if (d->io_offset[dim_x] < 0) {
    d->io_offset[dim_x] = 0;
  }
  if (d->io_offset[dim_x] > 80) {
    d->io_offset[dim_x] = 80;
  }
  d->io_offset[dim_y] = ((d->pc.position[dim_y] - 6) / 12) * 12;
  if (d->io_offset[dim_y] < 0) {
    d->io_offset[dim_y] = 0;
  }
  if (d->io_offset[dim_y] > 72) {
    d->io_offset[dim_y] = 72;
  }

#if 0
  uint32_t y;
  uint32_t min_diff, diff;

  min_diff = diff = abs(d->pc.position[dim_x] - 40);
  d->io_offset[dim_x] = 0;
  if ((diff = abs(d->pc.position[dim_x] - 80)) < min_diff) {
    min_diff = diff;
    d->io_offset[dim_x] = 40;
  }
  if ((diff = abs(d->pc.position[dim_x] - 120)) < min_diff) {
    min_diff = diff;
    d->io_offset[dim_x] = 80;
  }

  /* A lot more y values to deal with, so use a loop */

  for (min_diff = 96, d->io_offset[dim_y] = 0, y = 12; y <= 72; y += 12) {
    if ((diff = abs(d->pc.position[dim_y] - (y + 12))) < min_diff) {
      min_diff = diff;
      d->io_offset[dim_y] = y;
    }
  }
#endif
}

void io_update_offset(dungeon_t *d)
{
  int32_t x, y;

  x = (40 + d->io_offset[dim_x]) - d->pc.position[dim_x];
  y = (12 + d->io_offset[dim_y]) - d->pc.position[dim_y];

  if (x >= 30 && d->io_offset[dim_x]) {
    d->io_offset[dim_x] -= 40;
  }
  if (x <= -30 && d->io_offset[dim_x] != 80) {
    d->io_offset[dim_x] += 40;
  }
  if (y >= 8 && d->io_offset[dim_y]) {
    d->io_offset[dim_y] -= 12;
  }
  if (y <= -8 && d->io_offset[dim_y] != 72) {
    d->io_offset[dim_y] += 12;
  }
}

void io_display_tunnel(dungeon_t *d)
{
  uint32_t y, x;
  clear();
  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      mvprintw(y, x*2, "%02hhx",
               d->pc_tunnel[y][x] <= 255 ? d->pc_tunnel[y][x] : 255);
    }
  }
  refresh();
}

void io_display_distance(dungeon_t *d)
{
  uint32_t y, x;
  clear();
  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      mvprintw(y, x*2, "%02hhx", d->pc_distance[y][x]);
    }
  }
  refresh();
}

void io_display_huge(dungeon_t *d)
{
  uint32_t y, x;

  clear();
  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      if (d->character[y][x]) {
        attron(COLOR_PAIR(d->character[y][x]->color));
        mvaddch(y, x, d->character[y][x]->symbol);
        attroff(COLOR_PAIR(d->character[y][x]->color));
      } else if (d->object[y][x]) {
        attron(COLOR_PAIR(get_color(d->object[y][x])));
        mvaddch(y, x, get_symbol(d->object[y][x]));
        attroff(COLOR_PAIR(get_color(d->object[y][x])));
      } else {
        switch (mapxy(x, y)) {
        case ter_wall:
        case ter_wall_no_room:
        case ter_wall_no_floor:
        case ter_wall_immutable:
          mvaddch(y, x, '#');
          break;
        case ter_floor:
        case ter_floor_room:
        case ter_floor_hall:
        case ter_floor_tunnel:
          mvaddch(y, x, '.');
          break;
        case ter_debug:
          mvaddch(y, x, '*');
          break;
        case ter_stairs_up:
          mvaddch(y, x, '<');
          break;
        case ter_stairs_down:
          mvaddch(y, x, '>');
          break;
        default:
 /* Use zero as an error symbol, since it stands out somewhat, and it's *
  * not otherwise used.                                                 */
          mvaddch(y, x, '0');
        }
      }
    }
  }
  refresh();
}

//display the pc's stats so the user can see them
void print_pc_stats(dungeon_t *d){
	mvprintw(0, 81, "---PC stats---");
	mvprintw(1, 81, "HP:%d          ", d->pc.hp);
	mvprintw(2, 81, "SPD:%d          ", d->pc.speed);
}

void io_display(dungeon_t *d)
{
  uint32_t y, x;

  if (d->render_whole_dungeon) {
    io_display_huge(d);
    return;
  }

  clear();
  for (y = 0; y < 24; y++) {
    for (x = 0; x < 80; x++) {
      if (d->character[d->io_offset[dim_y] + y]
                      [d->io_offset[dim_x] + x]) {
        attron(COLOR_PAIR(d->character[d->io_offset[dim_y] + y]
                                      [d->io_offset[dim_x] + x]->color));
        mvaddch(y, x, d->character[d->io_offset[dim_y] + y]
                                  [d->io_offset[dim_x] + x]->symbol);
        attroff(COLOR_PAIR(d->character[d->io_offset[dim_y] + y]
                                       [d->io_offset[dim_x] + x]->color));
      } else if (d->object[d->io_offset[dim_y] + y][d->io_offset[dim_x] + x]) {
        attron(COLOR_PAIR(get_color(d->object[d->io_offset[dim_y] + y]
                                             [d->io_offset[dim_x] + x])));
        mvaddch(y, x, get_symbol(d->object[d->io_offset[dim_y] + y]
                                          [d->io_offset[dim_x] + x]));
        attroff(COLOR_PAIR(get_color(d->object[d->io_offset[dim_y] + y]
                                              [d->io_offset[dim_x] + x])));
      } else {
        switch (mapxy(d->io_offset[dim_x] + x,
                      d->io_offset[dim_y] + y)) {
        case ter_wall:
        case ter_wall_no_room:
        case ter_wall_no_floor:
        case ter_wall_immutable:
          mvaddch(y, x, '#');
          break;
        case ter_floor:
        case ter_floor_room:
        case ter_floor_hall:
        case ter_floor_tunnel:
          mvaddch(y, x, '.');
          break;
        case ter_debug:
          mvaddch(y, x, '*');
          break;
        case ter_stairs_up:
          mvaddch(y, x, '<');
          break;
        case ter_stairs_down:
          mvaddch(y, x, '>');
          break;
        default:
 /* Use zero as an error symbol, since it stands out somewhat, and it's *
  * not otherwise used.                                                 */
          mvaddch(y, x, '0');
        }
      }
    }
  }
  mvprintw(0, 0, "PC position is (%3d,%2d); offset is (%3d,%2d).",
           d->pc.position[dim_x], d->pc.position[dim_y],
           d->io_offset[dim_x], d->io_offset[dim_y]);
	
	//display the pc's stats so the user can see them
	print_pc_stats(d);

  refresh();
}

void io_look_mode(dungeon_t *d)
{
  int32_t key;

  do {
    if ((key = getch()) == 27 /* ESC */) {
      io_calculate_offset(d);
      io_display(d);
      return;
    }
    
    switch (key) {
    case '1':
    case 'b':
    case KEY_END:
    case '2':
    case 'j':
    case KEY_DOWN:
    case '3':
    case 'n':
    case KEY_NPAGE:
      if (d->io_offset[dim_y] != 72) {
        d->io_offset[dim_y] += 12;
      }
      break;
    case '4':
    case 'h':
    case KEY_LEFT:
    case '5':
    case ' ':
    case KEY_B2:
    case '6':
    case 'l':
    case KEY_RIGHT:
      break;
    case '7':
    case 'y':
    case KEY_HOME:
    case '8':
    case 'k':
    case KEY_UP:
    case '9':
    case 'u':
    case KEY_PPAGE:
      if (d->io_offset[dim_y]) {
        d->io_offset[dim_y] -= 12;
      }
      break;
    }
    switch (key) {
    case '1':
    case 'b':
    case KEY_END:
    case '4':
    case 'h':
    case KEY_LEFT:
    case '7':
    case 'y':
    case KEY_HOME:
      if (d->io_offset[dim_x]) {
        d->io_offset[dim_x] -= 40;
      }
      break;
    case '2':
    case 'j':
    case KEY_DOWN:
    case '5':
    case ' ':
    case KEY_B2:
    case '8':
    case 'k':
    case KEY_UP:
      break;
    case '3':
    case 'n':
    case KEY_NPAGE:
    case '6':
    case 'l':
    case KEY_RIGHT:
    case '9':
    case 'u':
    case KEY_PPAGE:
      if (d->io_offset[dim_x] != 80) {
        d->io_offset[dim_x] += 40;
      }
      break;
    }
    io_display(d);
  } while (1);
}

//updates the pc's stats whenever the equipped status changes
void update_pc_stats(dungeon_t *d){
	//variables
	int i;

	//reset to the default value
	d->pc.speed = PC_SPEED;
		
	//loop through all the equipped items and calculate new totals
	for(i=0;i<MAX_EQUIP;i++){
		//if there is an item in the slot
		if(d->pc.pc->equip[i]){
			//update the totals
			d->pc.speed += get_speed(d->pc.pc->equip[i]);		//speed
		}
	}

	//display the pc's stats so the user can see them
	print_pc_stats(d);
}

//displays all the items currently in the user's inventory
void io_display_slots(dungeon_t *d){
	//variables
	int i, j;
	
	//draw a blank square
	for(i=2;i<22;i++){
		for(j=4;j<76;j++){
			mvprintw(i, j, " ");
		}
	}
	
	//draw inventory numbers with dashes for default empty
	mvprintw(2, 7, "--Inventory--");
	mvprintw(4, 7, "0. --");
	mvprintw(5, 7, "1. --");
	mvprintw(6, 7, "2. --");
	mvprintw(7, 7, "3. --");
	mvprintw(8, 7, "4. --");
	mvprintw(9, 7, "5. --");
	mvprintw(10, 7, "6. --");
	mvprintw(11, 7, "7. --");
	mvprintw(12, 7, "8. --");
	mvprintw(13, 7, "9. --");
	
	//draw equipped numbers with dashes for default empty
	mvprintw(2, 50, "--Equipped Items--");
	mvprintw(4, 51, "a. --");
	mvprintw(5, 51, "b. --");
	mvprintw(6, 51, "c. --");
	mvprintw(7, 51, "d. --");
	mvprintw(8, 51, "e. --");
	mvprintw(9, 51, "f. --");
	mvprintw(10, 51, "g. --");
	mvprintw(11, 51, "h. --");
	mvprintw(12, 51, "i. --");
	mvprintw(13, 51, "j. --");
	mvprintw(14, 51, "k. --");
	mvprintw(15, 51, "l. --");
	
	//display the player's carry(if they have any)
	for(i=0;i<MAX_CARRY;i++){
		if(d->pc.pc->carry[i]){
			//mvprintw(4+i, 10, "%c ", get_symbol(d->pc.pc->carry[i]));
			mvprintw(4+i, 10, "%s ", get_name(d->pc.pc->carry[i]));
		}
	}
	
	//display the player's equip(if they have any)
	for(i=0;i<MAX_EQUIP;i++){
		if(d->pc.pc->equip[i]){
			//mvprintw(4+i, 54, "%c ", get_symbol(d->pc.pc->equip[i]));
			mvprintw(4+i, 54, "%s ", get_name(d->pc.pc->equip[i]));
		}
	}
	
	//display the pc's stats so the user can see them
	print_pc_stats(d);
	
}

//UI and controls if the user wants to wear items
void io_wear_item(dungeon_t *d){
	//variables
	//nt i;
	int line_selected = 4;
	int key;
	int fail_code = 1;			//exits the loop if value is zero
	  
	//get the user input
	do{		
		//show the user their items
		io_display_slots(d);
		
		//display the mode
		mvprintw(20, 5, "WEAR MODE");
	
		//select default item
		mvprintw(line_selected, 5, "*");
		
		//change selected item
		switch (key = getch()){
			case KEY_UP:
				if(line_selected-1 >= 4){
					line_selected--;
				}
				fail_code = 1;
				break;
			case KEY_DOWN:
				if(line_selected+1 <= 13){
					line_selected++;
				}
				fail_code = 1;
				break;
			case ' ':
				//if selected item exists
				if(d->pc.pc->carry[line_selected-4]){
					//if the selected item is one of the following types, swap it with the correct equip spot
					if(get_type(d->pc.pc->carry[line_selected-4])==0){
						//swap WEAPON
						d->pc.pc->temp = d->pc.pc->equip[0];
						d->pc.pc->equip[0] = d->pc.pc->carry[line_selected-4];
						d->pc.pc->carry[line_selected-4] =d->pc.pc->temp;
						d->pc.pc->temp = 0;
					}
					else if(get_type(d->pc.pc->carry[line_selected-4])==1){
						//swap OFFHAND
						d->pc.pc->temp = d->pc.pc->equip[1];
						d->pc.pc->equip[1] = d->pc.pc->carry[line_selected-4];
						d->pc.pc->carry[line_selected-4] =d->pc.pc->temp;
						d->pc.pc->temp = 0;
					}
					else if(get_type(d->pc.pc->carry[line_selected-4])==2){
						//swap RANGED
						d->pc.pc->temp = d->pc.pc->equip[2];
						d->pc.pc->equip[2] = d->pc.pc->carry[line_selected-4];
						d->pc.pc->carry[line_selected-4] =d->pc.pc->temp;
						d->pc.pc->temp = 0;
					}
					else if(get_type(d->pc.pc->carry[line_selected-4])==3){
						//swap ARMOR
						d->pc.pc->temp = d->pc.pc->equip[3];
						d->pc.pc->equip[4] = d->pc.pc->carry[line_selected-4];
						d->pc.pc->carry[line_selected-4] =d->pc.pc->temp;
						d->pc.pc->temp = 0;
					}
					else if(get_type(d->pc.pc->carry[line_selected-4])==4){
						//swap HELMET
						d->pc.pc->temp = d->pc.pc->equip[5];
						d->pc.pc->equip[5] = d->pc.pc->carry[line_selected-4];
						d->pc.pc->carry[line_selected-4] =d->pc.pc->temp;
						d->pc.pc->temp = 0;
					}
					else if(get_type(d->pc.pc->carry[line_selected-4])==5){
						//swap CLOAK
						d->pc.pc->temp = d->pc.pc->equip[5];
						d->pc.pc->equip[5] = d->pc.pc->carry[line_selected-4];
						d->pc.pc->carry[line_selected-4] =d->pc.pc->temp;
						d->pc.pc->temp = 0;
					}
					else if(get_type(d->pc.pc->carry[line_selected-4])==6){
						//swap GLOVES
						d->pc.pc->temp = d->pc.pc->equip[6];
						d->pc.pc->equip[6] = d->pc.pc->carry[line_selected-4];
						d->pc.pc->carry[line_selected-4] =d->pc.pc->temp;
						d->pc.pc->temp = 0;
					}
					else if(get_type(d->pc.pc->carry[line_selected-4])==7){
						//swap BOOTS
						d->pc.pc->temp = d->pc.pc->equip[7];
						d->pc.pc->equip[7] = d->pc.pc->carry[line_selected-4];
						d->pc.pc->carry[line_selected-4] =d->pc.pc->temp;
						d->pc.pc->temp = 0;
					}
					else if(get_type(d->pc.pc->carry[line_selected-4])==8){
						//swap AMULET
						d->pc.pc->temp = d->pc.pc->equip[8];
						d->pc.pc->equip[8] = d->pc.pc->carry[line_selected-4];
						d->pc.pc->carry[line_selected-4] =d->pc.pc->temp;
						d->pc.pc->temp = 0;
					}
					else if(get_type(d->pc.pc->carry[line_selected-4])==9){
						//swap LIGHT
						d->pc.pc->temp = d->pc.pc->equip[9];
						d->pc.pc->equip[9] = d->pc.pc->carry[line_selected-4];
						d->pc.pc->carry[line_selected-4] =d->pc.pc->temp;
						d->pc.pc->temp = 0;
					}
					else if(get_type(d->pc.pc->carry[line_selected-4])==10){
						//variables
						int needs_swap = 0;		//1 = yes, 0 = no
						int i;
						
						//put it in first or second slot (first available)
						for(i=10;i<12;i++){
							if(!d->pc.pc->equip[i]){
								d->pc.pc->equip[i] = d->pc.pc->carry[line_selected-4];
								d->pc.pc->carry[line_selected-4]=0;
								needs_swap = 0;
								break;
							}
							else{
								needs_swap = 1;
							}
						}
						//if we need to swap, just swap the first one cause I'm lazy
						if(needs_swap){
							//swap RING(1)
							d->pc.pc->temp = d->pc.pc->equip[10];
							d->pc.pc->equip[10] = d->pc.pc->carry[line_selected-4];
							d->pc.pc->carry[line_selected-4] =d->pc.pc->temp;
							d->pc.pc->temp = 0;
						}
					}
					
				}
				
				//move selected object to next open spot in equip if you can
				/*for(i=0;i<MAX_EQUIP;i++){
					if(!d->pc.pc->equip[i]){
						d->pc.pc->equip[i] = d->pc.pc->carry[line_selected-4];
						d->pc.pc->carry[line_selected-4]=0;
						break;
					}
				}*/
				
				//update pc stats
				update_pc_stats(d);
				
				fail_code = 1;
				break;
			case 27:
			case 'w':
				io_display(d);
				fail_code = 0;
				break;
			default:
			  /* Also not in the spec.  It's not always easy to figure out what *
			   * key code corresponds with a given keystroke.  Print out any    *
			   * unhandled key here.  Not only does it give a visual error      *
			   * indicator, but it also gives an integer value that can be used *
			   * for that key in this (or other) switch statements.  Printed in *
			   * octal, with the leading zero, because ncurses.h lists codes in *
			   * octal, thus allowing us to do reverse lookups.  If a key has a *
			   * name defined in the header, you can use the name here, else    *
			   * you can directly use the octal value.                          */
			  mvprintw(0, 0, "Unbound key: %#o ", key);
			  fail_code = 1;
			}
		  } while (fail_code);	 
}

void io_remove_item(dungeon_t *d){
	//variables
	int i;
	int line_selected = 4;
	int key;
	int fail_code = 1;			//exits the loop if value is zero
	  
	//get the user input
	do{		
		//show the user their items
		io_display_slots(d);
		
		//display the mode
		mvprintw(20, 5, "TAKE OFF MODE");
	
		//select default item
		mvprintw(line_selected, 49, "*");
		
		//change selected item
		switch (key = getch()){
			case KEY_UP:
				if(line_selected-1 >= 4){
					line_selected--;
				}
				fail_code = 1;
				break;
			case KEY_DOWN:
				if(line_selected+1 <= 15){
					line_selected++;
				}
				fail_code = 1;
				break;
			case ' ':
				//move selected object to next open spot in carry if you can
				for(i=0;i<MAX_EQUIP;i++){
					if(!d->pc.pc->carry[i]){
						d->pc.pc->carry[i] = d->pc.pc->equip[line_selected-4];
						
						//delete from equip
						d->pc.pc->equip[line_selected-4]=0;
						break;
					}
				}
				
				//update the stats
				update_pc_stats(d);
				
				fail_code = 1;
				break;
			case 27:
			case 't':
				io_display(d);
				fail_code = 0;
				break;
			default:
			  /* Also not in the spec.  It's not always easy to figure out what *
			   * key code corresponds with a given keystroke.  Print out any    *
			   * unhandled key here.  Not only does it give a visual error      *
			   * indicator, but it also gives an integer value that can be used *
			   * for that key in this (or other) switch statements.  Printed in *
			   * octal, with the leading zero, because ncurses.h lists codes in *
			   * octal, thus allowing us to do reverse lookups.  If a key has a *
			   * name defined in the header, you can use the name here, else    *
			   * you can directly use the octal value.                          */
			  mvprintw(0, 0, "Unbound key: %#o ", key);
			  fail_code = 1;
			}
		  } while (fail_code);
	 
}

void io_drop_item(dungeon_t *d){
		//variables
	int line_selected = 4;
	int key;
	int fail_code = 1;			//exits the loop if value is zero
	  
	//get the user input
	do{		
		//show the user their items
		io_display_slots(d);
		
		//display the mode
		mvprintw(20, 5, "DROP MODE");
	
		//select default item
		mvprintw(line_selected, 5, "*");
		
		//change selected item
		switch (key = getch()){
			case KEY_UP:
				if(line_selected-1 >= 4){
					line_selected--;
				}
				fail_code = 1;
				break;
			case KEY_DOWN:
				if(line_selected+1 <= 13){
					line_selected++;
				}
				fail_code = 1;
				break;
			case ' ':
				//move selected object to the floor spot if it is on an item
				if(d->pc.pc->carry[line_selected-4]){
					//if no item right below you & no stair
					if(!d->object[d->pc.position[dim_y]][d->pc.position[dim_x]] && 
					  mapxy(d->pc.position[dim_x],d->pc.position[dim_y])!=ter_stairs_up && 
					  mapxy(d->pc.position[dim_x],d->pc.position[dim_y])!=ter_stairs_down){
						
						//drop object
						d->object[d->pc.position[dim_y]][d->pc.position[dim_x]] = d->pc.pc->carry[line_selected-4];
						
						//delete item from carry
						d->pc.pc->carry[line_selected-4]=0;
					}
					//else look all around you for next available spot
					else{
								
						//up
						if(!d->object[d->pc.position[dim_y]-1][d->pc.position[dim_x]] && 
							mapxy(d->pc.position[dim_x],d->pc.position[dim_y]-1)!=ter_stairs_up && 
							mapxy(d->pc.position[dim_x],d->pc.position[dim_y]-1)!=ter_stairs_down &&
							mapxy(d->pc.position[dim_x],d->pc.position[dim_y]-1)!=ter_wall &&
							mapxy(d->pc.position[dim_x],d->pc.position[dim_y]-1)!=ter_wall_no_room &&
							mapxy(d->pc.position[dim_x],d->pc.position[dim_y]-1)!=ter_wall_no_floor &&
							mapxy(d->pc.position[dim_x],d->pc.position[dim_y]-1)!=ter_wall_immutable){
							
							//drop object
							d->object[d->pc.position[dim_y]-1][d->pc.position[dim_x]] = d->pc.pc->carry[line_selected-4];
							
							//delete item from carry
							d->pc.pc->carry[line_selected-4]=0;
							
						}
					  
						//down
						else if(!d->object[d->pc.position[dim_y]+1][d->pc.position[dim_x]] && 
							mapxy(d->pc.position[dim_x],d->pc.position[dim_y]+1)!=ter_stairs_up && 
							mapxy(d->pc.position[dim_x],d->pc.position[dim_y]+1)!=ter_stairs_down &&
							mapxy(d->pc.position[dim_x],d->pc.position[dim_y]+1)!=ter_wall &&
							mapxy(d->pc.position[dim_x],d->pc.position[dim_y]+1)!=ter_wall_no_room &&
							mapxy(d->pc.position[dim_x],d->pc.position[dim_y]+1)!=ter_wall_no_floor &&
							mapxy(d->pc.position[dim_x],d->pc.position[dim_y]+1)!=ter_wall_immutable){
							
							//drop object
							d->object[d->pc.position[dim_y]+1][d->pc.position[dim_x]] = d->pc.pc->carry[line_selected-4];
							
							//delete item from carry
							d->pc.pc->carry[line_selected-4]=0;
							
						}
						//left
						else if(!d->object[d->pc.position[dim_y]][d->pc.position[dim_x]-1] && 
							mapxy(d->pc.position[dim_x]-1,d->pc.position[dim_y])!=ter_stairs_up && 
							mapxy(d->pc.position[dim_x]-1,d->pc.position[dim_y])!=ter_stairs_down &&
							mapxy(d->pc.position[dim_x]-1,d->pc.position[dim_y])!=ter_wall &&
							mapxy(d->pc.position[dim_x]-1,d->pc.position[dim_y])!=ter_wall_no_room &&
							mapxy(d->pc.position[dim_x]-1,d->pc.position[dim_y])!=ter_wall_no_floor &&
							mapxy(d->pc.position[dim_x]-1,d->pc.position[dim_y])!=ter_wall_immutable){
							
							//drop object
							d->object[d->pc.position[dim_y]][d->pc.position[dim_x]-1] = d->pc.pc->carry[line_selected-4];
							
							//delete item from carry
							d->pc.pc->carry[line_selected-4]=0;
							
						}
						//right
						else if(!d->object[d->pc.position[dim_y]][d->pc.position[dim_x]+1] && 
							mapxy(d->pc.position[dim_x]+1,d->pc.position[dim_y])!=ter_stairs_up && 
							mapxy(d->pc.position[dim_x]+1,d->pc.position[dim_y])!=ter_stairs_down &&
							mapxy(d->pc.position[dim_x]+1,d->pc.position[dim_y])!=ter_wall &&
							mapxy(d->pc.position[dim_x]+1,d->pc.position[dim_y])!=ter_wall_no_room &&
							mapxy(d->pc.position[dim_x]+1,d->pc.position[dim_y])!=ter_wall_no_floor &&
							mapxy(d->pc.position[dim_x]+1,d->pc.position[dim_y])!=ter_wall_immutable){
							
							//drop object
							d->object[d->pc.position[dim_y]][d->pc.position[dim_x]+1] = d->pc.pc->carry[line_selected-4];
							
							//delete item from carry
							d->pc.pc->carry[line_selected-4]=0;
							
						}
						//up-left
						else if(!d->object[d->pc.position[dim_y]-1][d->pc.position[dim_x]-1] && 
							mapxy(d->pc.position[dim_x]-1,d->pc.position[dim_y]-1)!=ter_stairs_up && 
							mapxy(d->pc.position[dim_x]-1,d->pc.position[dim_y]-1)!=ter_stairs_down &&
							mapxy(d->pc.position[dim_x]-1,d->pc.position[dim_y]-1)!=ter_wall &&
							mapxy(d->pc.position[dim_x]-1,d->pc.position[dim_y]-1)!=ter_wall_no_room &&
							mapxy(d->pc.position[dim_x]-1,d->pc.position[dim_y]-1)!=ter_wall_no_floor &&
							mapxy(d->pc.position[dim_x]-1,d->pc.position[dim_y]-1)!=ter_wall_immutable){
							
							//drop object
							d->object[d->pc.position[dim_y]-1][d->pc.position[dim_x]-1] = d->pc.pc->carry[line_selected-4];
							
							//delete item from carry
							d->pc.pc->carry[line_selected-4]=0;
							
						}
						
						//up-right
						else if(!d->object[d->pc.position[dim_y]-1][d->pc.position[dim_x]+1] && 
							mapxy(d->pc.position[dim_x]+1,d->pc.position[dim_y]-1)!=ter_stairs_up && 
							mapxy(d->pc.position[dim_x]+1,d->pc.position[dim_y]-1)!=ter_stairs_down &&
							mapxy(d->pc.position[dim_x]+1,d->pc.position[dim_y]-1)!=ter_wall &&
							mapxy(d->pc.position[dim_x]+1,d->pc.position[dim_y]-1)!=ter_wall_no_room &&
							mapxy(d->pc.position[dim_x]+1,d->pc.position[dim_y]-1)!=ter_wall_no_floor &&
							mapxy(d->pc.position[dim_x]+1,d->pc.position[dim_y]-1)!=ter_wall_immutable){
							
							//drop object
							d->object[d->pc.position[dim_y]-1][d->pc.position[dim_x]+1] = d->pc.pc->carry[line_selected-4];
							
							//delete item from carry
							d->pc.pc->carry[line_selected-4]=0;
							
						}
						//down-left
						else if(!d->object[d->pc.position[dim_y]+1][d->pc.position[dim_x]-1] && 
							mapxy(d->pc.position[dim_x]-1,d->pc.position[dim_y]+1)!=ter_stairs_up && 
							mapxy(d->pc.position[dim_x]-1,d->pc.position[dim_y]+1)!=ter_stairs_down &&
							mapxy(d->pc.position[dim_x]-1,d->pc.position[dim_y]+1)!=ter_wall &&
							mapxy(d->pc.position[dim_x]-1,d->pc.position[dim_y]+1)!=ter_wall_no_room &&
							mapxy(d->pc.position[dim_x]-1,d->pc.position[dim_y]+1)!=ter_wall_no_floor &&
							mapxy(d->pc.position[dim_x]-1,d->pc.position[dim_y]+1)!=ter_wall_immutable){
							
							//drop object
							d->object[d->pc.position[dim_y]+1][d->pc.position[dim_x]-1] = d->pc.pc->carry[line_selected-4];
							
							//delete item from carry
							d->pc.pc->carry[line_selected-4]=0;
							
						}
						//down-right
						else if(!d->object[d->pc.position[dim_y]+1][d->pc.position[dim_x]+1] && 
							mapxy(d->pc.position[dim_x]+1,d->pc.position[dim_y]+1)!=ter_stairs_up && 
							mapxy(d->pc.position[dim_x]+1,d->pc.position[dim_y]+1)!=ter_stairs_down &&
							mapxy(d->pc.position[dim_x]+1,d->pc.position[dim_y]+1)!=ter_wall &&
							mapxy(d->pc.position[dim_x]+1,d->pc.position[dim_y]+1)!=ter_wall_no_room &&
							mapxy(d->pc.position[dim_x]+1,d->pc.position[dim_y]+1)!=ter_wall_no_floor &&
							mapxy(d->pc.position[dim_x]+1,d->pc.position[dim_y]+1)!=ter_wall_immutable){
							
							//drop object
							d->object[d->pc.position[dim_y]+1][d->pc.position[dim_x]+1] = d->pc.pc->carry[line_selected-4];
							
							//delete item from carry
							d->pc.pc->carry[line_selected-4]=0;
							
						}
					}	
				}
				
				fail_code = 1;
				break;
			case 27:
			case 'd':
				io_display(d);
				fail_code = 0;
				break;
			default:
			  /* Also not in the spec.  It's not always easy to figure out what *
			   * key code corresponds with a given keystroke.  Print out any    *
			   * unhandled key here.  Not only does it give a visual error      *
			   * indicator, but it also gives an integer value that can be used *
			   * for that key in this (or other) switch statements.  Printed in *
			   * octal, with the leading zero, because ncurses.h lists codes in *
			   * octal, thus allowing us to do reverse lookups.  If a key has a *
			   * name defined in the header, you can use the name here, else    *
			   * you can directly use the octal value.                          */
			  mvprintw(0, 0, "Unbound key: %#o ", key);
			  fail_code = 1;
			}
		  } while (fail_code);
}

void io_expunge_item(dungeon_t *d){
		//variables
	int line_selected = 4;
	int key;
	int fail_code = 1;			//exits the loop if value is zero
	  
	//get the user input
	do{		
		//show the user their items
		io_display_slots(d);
		
		//display the mode
		mvprintw(20, 5, "EXPUNGE MODE");
	
		//select default item
		mvprintw(line_selected, 5, "*");
		
		//change selected item
		switch (key = getch()){
			case KEY_UP:
				if(line_selected-1 >= 4){
					line_selected--;
				}
				fail_code = 1;
				break;
			case KEY_DOWN:
				if(line_selected+1 <= 13){
					line_selected++;
				}
				fail_code = 1;
				break;
			case ' ':
				//expunge selected object if it is on an item
				if(d->pc.pc->carry[line_selected-4]){
					destroy_this_object(d->pc.pc->carry[line_selected-4]);
					
					//delete item from carry
					d->pc.pc->carry[line_selected-4]=0;
				}
				
				fail_code = 1;
				break;
			case 27:
			case 'x':
				io_display(d);
				fail_code = 0;
				break;
			default:
			  /* Also not in the spec.  It's not always easy to figure out what *
			   * key code corresponds with a given keystroke.  Print out any    *
			   * unhandled key here.  Not only does it give a visual error      *
			   * indicator, but it also gives an integer value that can be used *
			   * for that key in this (or other) switch statements.  Printed in *
			   * octal, with the leading zero, because ncurses.h lists codes in *
			   * octal, thus allowing us to do reverse lookups.  If a key has a *
			   * name defined in the header, you can use the name here, else    *
			   * you can directly use the octal value.                          */
			  mvprintw(0, 0, "Unbound key: %#o ", key);
			  fail_code = 1;
			}
		  } while (fail_code);
}

void io_handle_input(dungeon_t *d)
{
  uint32_t fail_code;		//decides if we should exit the loop
  int key;

  do {
    switch (key = getch()) {
    case '7':
    case 'y':
    case KEY_HOME:
      fail_code = move_pc(d, 7);
      break;
    case '8':
    case 'k':
    case KEY_UP:
      fail_code = move_pc(d, 8);
      break;
    case '9':
    case 'u':
    case KEY_PPAGE:
      fail_code = move_pc(d, 9);
      break;
    case '6':
    case 'l':
    case KEY_RIGHT:
      fail_code = move_pc(d, 6);
      break;
    case '3':
    case 'n':
    case KEY_NPAGE:
      fail_code = move_pc(d, 3);
      break;
    case '2':
    case 'j':
    case KEY_DOWN:
      fail_code = move_pc(d, 2);
      break;
    case '1':
    case 'b':
    case KEY_END:
      fail_code = move_pc(d, 1);
      break;
    case '4':
    case 'h':
    case KEY_LEFT:
      fail_code = move_pc(d, 4);
      break;
    case '5':
    case ' ':
    case KEY_B2:
      fail_code = 0;
      break;
    case '>':
      fail_code = move_pc(d, '>');
      break;
    case '<':
      fail_code = move_pc(d, '<');
      break;
    case 'L':
      io_look_mode(d);
      fail_code = 0;
      break;
    case 'S':
      d->save_and_exit = 1;
      d->pc.next_turn -= (1000 / d->pc.speed);
      fail_code = 0;
      break;
    case 'Q':
      /* Extra command, not in the spec.  Quit without saving.          */
      d->quit_no_save = 1;
      fail_code = 0;
      break;
    case 'H':
      /* Extra command, not in the spec.  H is for Huge: draw the whole *
       * dungeon, the pre-curses way.  Doesn't use a player turn.       */
      io_display_huge(d);
      fail_code = 1;
      break;
    case 'T':
      /* New command.  Display the distances for tunnelers.  Displays   *
       * in hex with two characters per cell.                           */
      io_display_tunnel(d);
      fail_code = 1;
      break;
    case 'D':
      /* New command.  Display the distances for non-tunnelers.         *
       *  Displays in hex with two characters per cell.                 */
      io_display_distance(d);
      fail_code = 1;
      break;
    case 's':
      /* New command.  Return to normal display after displaying some   *
       * special screen.                                                */
      io_display(d);
      fail_code = 1;
      break;
	case 'w':
      /*Wear an item. Prompts the user for a carry slot. If an item of that type is already
		equipped, items are swapped  */
      //io_display(d);
	  io_wear_item(d);
      fail_code = 1;
      break;
	case 't':
      /*Take off an item. Prompts for equipment slot. Item goes to an open carry slot.*/
      //io_display(d);
	  io_remove_item(d);
      fail_code = 1;
      break;
	case 'd':
      /*Drop an item. Prompts user for carry slot. Item goes to floor.*/
      //io_display(d);
	  io_drop_item(d);
      fail_code = 1;
      break;
	case 'x':
	  /*Expunge an item from the game. Prompts the user for a carry slot. Item is permanently
		removed from the game*/
      //io_display(d);
	  io_expunge_item(d);
      fail_code = 1;
      break;
    default:
      /* Also not in the spec.  It's not always easy to figure out what *
       * key code corresponds with a given keystroke.  Print out any    *
       * unhandled key here.  Not only does it give a visual error      *
       * indicator, but it also gives an integer value that can be used *
       * for that key in this (or other) switch statements.  Printed in *
       * octal, with the leading zero, because ncurses.h lists codes in *
       * octal, thus allowing us to do reverse lookups.  If a key has a *
       * name defined in the header, you can use the name here, else    *
       * you can directly use the octal value.                          */
      mvprintw(0, 0, "Unbound key: %#o ", key);
      fail_code = 1;
    }
  } while (fail_code);
}

























