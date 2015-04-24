#include <unistd.h>
#include <stdlib.h>

#include "dungeon.h"
#include "heap.h"
#include "move.h"
#include "npc.h"
#include "pc.h"
#include "character.h"
#include "io.h"
#include "dice.h"
#include "object.h"

/* Ugly hack: There is no way to pass a pointer to the dungeon into the *
 * heap's comparitor funtion without modifying the heap.  Copying the   *
 * pc_distance array is a possible solution, but that doubles the       *
 * bandwidth requirements for dijkstra, which would also be bad.        *
 * Instead, make a global pointer to the dungeon in this file,          *
 * initialize it in dijkstra, and use it in the comparitor to get to    *
 * pc_distance.  Otherwise, pretend it doesn't exist, because it really *
 * is ugly.                                                             */
static dungeon_t *dungeon;

typedef struct path {
  heap_node_t *hn;
  uint8_t pos[2];
} path_t;

static int32_t dist_cmp(const void *key, const void *with) {
  return ((int32_t) dungeon->pc_distance[((path_t *) key)->pos[dim_y]]
                                        [((path_t *) key)->pos[dim_x]] -
          (int32_t) dungeon->pc_distance[((path_t *) with)->pos[dim_y]]
                                        [((path_t *) with)->pos[dim_x]]);
}

static int32_t tunnel_cmp(const void *key, const void *with) {
  return ((int32_t) dungeon->pc_tunnel[((path_t *) key)->pos[dim_y]]
                                      [((path_t *) key)->pos[dim_x]] -
          (int32_t) dungeon->pc_tunnel[((path_t *) with)->pos[dim_y]]
                                      [((path_t *) with)->pos[dim_x]]);
}

void dijkstra(dungeon_t *d)
{
  /* Currently assumes that monsters only move on floors.  Will *
   * need to be modified for tunneling and pass-wall monsters.  */

  heap_t h;
  uint32_t x, y;
  static path_t p[DUNGEON_Y][DUNGEON_X], *c;
  static uint32_t initialized = 0;

  if (!initialized) {
    initialized = 1;
    dungeon = d;
    for (y = 0; y < DUNGEON_Y; y++) {
      for (x = 0; x < DUNGEON_X; x++) {
        p[y][x].pos[dim_y] = y;
        p[y][x].pos[dim_x] = x;
      }
    }
  }

  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      d->pc_distance[y][x] = 255;
    }
  }
  d->pc_distance[d->pc.position[dim_y]][d->pc.position[dim_x]] = 0;

  heap_init(&h, dist_cmp, NULL);

  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      if (mapxy(x, y) >= ter_floor) {
        p[y][x].hn = heap_insert(&h, &p[y][x]);
      }
    }
  }

  while ((c = heap_remove_min(&h))) {
    c->hn = NULL;
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x] - 1].hn) &&
        (d->pc_distance[c->pos[dim_y] - 1][c->pos[dim_x] - 1] >
         d->pc_distance[c->pos[dim_y]][c->pos[dim_x]] + 1)) {
      d->pc_distance[c->pos[dim_y] - 1][c->pos[dim_x] - 1] =
        d->pc_distance[c->pos[dim_y]][c->pos[dim_x]] + 1;
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x]    ].hn) &&
        (d->pc_distance[c->pos[dim_y] - 1][c->pos[dim_x]    ] >
         d->pc_distance[c->pos[dim_y]][c->pos[dim_x]] + 1)) {
      d->pc_distance[c->pos[dim_y] - 1][c->pos[dim_x]    ] =
        d->pc_distance[c->pos[dim_y]][c->pos[dim_x]] + 1;
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x]    ].hn);
    }
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x] + 1].hn) &&
        (d->pc_distance[c->pos[dim_y] - 1][c->pos[dim_x] + 1] >
         d->pc_distance[c->pos[dim_y]][c->pos[dim_x]] + 1)) {
      d->pc_distance[c->pos[dim_y] - 1][c->pos[dim_x] + 1] =
        d->pc_distance[c->pos[dim_y]][c->pos[dim_x]] + 1;
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x] + 1].hn);
    }
    if ((p[c->pos[dim_y]    ][c->pos[dim_x] - 1].hn) &&
        (d->pc_distance[c->pos[dim_y]    ][c->pos[dim_x] - 1] >
         d->pc_distance[c->pos[dim_y]][c->pos[dim_x]] + 1)) {
      d->pc_distance[c->pos[dim_y]    ][c->pos[dim_x] - 1] =
        d->pc_distance[c->pos[dim_y]][c->pos[dim_x]] + 1;
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y]    ][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y]    ][c->pos[dim_x] + 1].hn) &&
        (d->pc_distance[c->pos[dim_y]    ][c->pos[dim_x] + 1] >
         d->pc_distance[c->pos[dim_y]][c->pos[dim_x]] + 1)) {
      d->pc_distance[c->pos[dim_y]    ][c->pos[dim_x] + 1] =
        d->pc_distance[c->pos[dim_y]][c->pos[dim_x]] + 1;
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y]    ][c->pos[dim_x] + 1].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x] - 1].hn) &&
        (d->pc_distance[c->pos[dim_y] + 1][c->pos[dim_x] - 1] >
         d->pc_distance[c->pos[dim_y]][c->pos[dim_x]] + 1)) {
      d->pc_distance[c->pos[dim_y] + 1][c->pos[dim_x] - 1] =
        d->pc_distance[c->pos[dim_y]][c->pos[dim_x]] + 1;
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x]    ].hn) &&
        (d->pc_distance[c->pos[dim_y] + 1][c->pos[dim_x]    ] >
         d->pc_distance[c->pos[dim_y]][c->pos[dim_x]] + 1)) {
      d->pc_distance[c->pos[dim_y] + 1][c->pos[dim_x]    ] =
        d->pc_distance[c->pos[dim_y]][c->pos[dim_x]] + 1;
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x]    ].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x] + 1].hn) &&
        (d->pc_distance[c->pos[dim_y] + 1][c->pos[dim_x] + 1] >
         d->pc_distance[c->pos[dim_y]][c->pos[dim_x]] + 1)) {
      d->pc_distance[c->pos[dim_y] + 1][c->pos[dim_x] + 1] =
        d->pc_distance[c->pos[dim_y]][c->pos[dim_x]] + 1;
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x] + 1].hn);
    }
  }
  heap_delete(&h);
}

void dijkstra_tunnel(dungeon_t *d)
{
  /* Currently assumes that monsters only move on floors.  Will *
   * need to be modified for tunneling and pass-wall monsters.  */

  heap_t h;
  uint32_t x, y;
  int size;
  static path_t p[DUNGEON_Y][DUNGEON_X], *c;
  static uint32_t initialized = 0;

  if (!initialized) {
    initialized = 1;
    dungeon = d;
    for (y = 0; y < DUNGEON_Y; y++) {
      for (x = 0; x < DUNGEON_X; x++) {
        p[y][x].pos[dim_y] = y;
        p[y][x].pos[dim_x] = x;
      }
    }
  }

  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      d->pc_tunnel[y][x] = 65535;
    }
  }
  d->pc_tunnel[d->pc.position[dim_y]][d->pc.position[dim_x]] = 0;

  heap_init(&h, tunnel_cmp, NULL);

  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      if (mapxy(x, y) != ter_wall_immutable) {
        p[y][x].hn = heap_insert(&h, &p[y][x]);
      }
    }
  }

  size = h.size;
  while ((c = heap_remove_min(&h))) {
    if (--size != h.size) {
      exit(1);
    }
    c->hn = NULL;
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x] - 1].hn) &&
        (d->pc_tunnel[c->pos[dim_y] - 1][c->pos[dim_x] - 1] >
         d->pc_tunnel[c->pos[dim_y]][c->pos[dim_x]] + 1 +
         (d->hardness[c->pos[dim_y]][c->pos[dim_x]] / 60))) {
      d->pc_tunnel[c->pos[dim_y] - 1][c->pos[dim_x] - 1] =
        (d->pc_tunnel[c->pos[dim_y]][c->pos[dim_x]] + 1 +
         (d->hardness[c->pos[dim_y]][c->pos[dim_x]] / 60));
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x]    ].hn) &&
        (d->pc_tunnel[c->pos[dim_y] - 1][c->pos[dim_x]    ] >
         d->pc_tunnel[c->pos[dim_y]][c->pos[dim_x]] + 1 +
         (d->hardness[c->pos[dim_y]][c->pos[dim_x]] / 60))) {
      d->pc_tunnel[c->pos[dim_y] - 1][c->pos[dim_x]    ] =
        (d->pc_tunnel[c->pos[dim_y]][c->pos[dim_x]] + 1 +
         (d->hardness[c->pos[dim_y]][c->pos[dim_x]] / 60));
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x]    ].hn);
    }
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x] + 1].hn) &&
        (d->pc_tunnel[c->pos[dim_y] - 1][c->pos[dim_x] + 1] >
         d->pc_tunnel[c->pos[dim_y]][c->pos[dim_x]] + 1 +
         (d->hardness[c->pos[dim_y]][c->pos[dim_x]] / 60))) {
      d->pc_tunnel[c->pos[dim_y] - 1][c->pos[dim_x] + 1] =
        (d->pc_tunnel[c->pos[dim_y]][c->pos[dim_x]] + 1 +
         (d->hardness[c->pos[dim_y]][c->pos[dim_x]] / 60));
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x] + 1].hn);
    }
    if ((p[c->pos[dim_y]    ][c->pos[dim_x] - 1].hn) &&
        (d->pc_tunnel[c->pos[dim_y]    ][c->pos[dim_x] - 1] >
         d->pc_tunnel[c->pos[dim_y]][c->pos[dim_x]] + 1 +
         (d->hardness[c->pos[dim_y]][c->pos[dim_x]] / 60))) {
      d->pc_tunnel[c->pos[dim_y]    ][c->pos[dim_x] - 1] =
        (d->pc_tunnel[c->pos[dim_y]][c->pos[dim_x]] + 1 +
         (d->hardness[c->pos[dim_y]][c->pos[dim_x]] / 60));
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y]    ][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y]    ][c->pos[dim_x] + 1].hn) &&
        (d->pc_tunnel[c->pos[dim_y]    ][c->pos[dim_x] + 1] >
         d->pc_tunnel[c->pos[dim_y]][c->pos[dim_x]] + 1 +
         (d->hardness[c->pos[dim_y]][c->pos[dim_x]] / 60))) {
      d->pc_tunnel[c->pos[dim_y]    ][c->pos[dim_x] + 1] =
        (d->pc_tunnel[c->pos[dim_y]][c->pos[dim_x]] + 1 +
         (d->hardness[c->pos[dim_y]][c->pos[dim_x]] / 60));
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y]    ][c->pos[dim_x] + 1].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x] - 1].hn) &&
        (d->pc_tunnel[c->pos[dim_y] + 1][c->pos[dim_x] - 1] >
         d->pc_tunnel[c->pos[dim_y]][c->pos[dim_x]] + 1 +
         (d->hardness[c->pos[dim_y]][c->pos[dim_x]] / 60))) {
      d->pc_tunnel[c->pos[dim_y] + 1][c->pos[dim_x] - 1] =
        (d->pc_tunnel[c->pos[dim_y]][c->pos[dim_x]] + 1 +
         (d->hardness[c->pos[dim_y]][c->pos[dim_x]] / 60));
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x]    ].hn) &&
        (d->pc_tunnel[c->pos[dim_y] + 1][c->pos[dim_x]    ] >
         d->pc_tunnel[c->pos[dim_y]][c->pos[dim_x]] + 1 +
         (d->hardness[c->pos[dim_y]][c->pos[dim_x]] / 60))) {
      d->pc_tunnel[c->pos[dim_y] + 1][c->pos[dim_x]    ] =
        (d->pc_tunnel[c->pos[dim_y]][c->pos[dim_x]] + 1 +
         (d->hardness[c->pos[dim_y]][c->pos[dim_x]] / 60));
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x]    ].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x] + 1].hn) &&
        (d->pc_tunnel[c->pos[dim_y] + 1][c->pos[dim_x] + 1] >
         d->pc_tunnel[c->pos[dim_y]][c->pos[dim_x]] + 1 +
         (d->hardness[c->pos[dim_y]][c->pos[dim_x]] / 60))) {
      d->pc_tunnel[c->pos[dim_y] + 1][c->pos[dim_x] + 1] =
        (d->pc_tunnel[c->pos[dim_y]][c->pos[dim_x]] + 1 +
         (d->hardness[c->pos[dim_y]][c->pos[dim_x]] / 60));
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x] + 1].hn);
    }
  }
  heap_delete(&h);
}

//handles the attacking of characters in the dungeon
void attack_character(dungeon_t *d, character_t *c, pair_t next){
	//variables
	int i;
	int32_t total_damage = 0;
	
	//roll base damage
	total_damage += roll_dice(c->damage);
	
	//if pc, calculate the total damage based on equipped items' rolls
	if(c == &d->pc){
		//loop through all the equipped items and calculate new totals
		for(i=0;i<MAX_EQUIP;i++){
			//if there is an item in the slot
			if(c->pc->equip[i]){
				//roll the damage, and add it to the total damage
				total_damage += get_damage(c->pc->equip[i]);
			}
		}
	}
	
	//apply damage to the character at the next position (subtract from hp)
	d->character[next[dim_y]][next[dim_x]]->hp -= total_damage;
	
	//if the hp fell below zero, mark it as dead
	if(d->character[next[dim_y]][next[dim_x]]->hp <= 0){
		d->character[next[dim_y]][next[dim_x]]->alive = 0;
		
		//if the thing that died was an npc, decrease monster number
		if(d->character[next[dim_y]][next[dim_x]] != &d->pc) {
			d->num_monsters--;
		}
	}
		
	//profit
}

//takes care of moving characters in the dungeon
void move_character(dungeon_t *d, character_t *c, pair_t next){
	//empty they current spot the character is standing
	d->character[c->position[dim_y]][c->position[dim_x]] = NULL;
  
	//if there is something at the next spot, you might fight it
	if(d->character[next[dim_y]][next[dim_x]]){
		//if the current character is the pc, then you need to fight
		if(c == &d->pc){
			attack_character(d, c, next);		
		}
		//else, you are a npc, so only fight if the next spot is the pc (prevents npc civil war)
		else if(d->character[next[dim_y]][next[dim_x]]==&d->pc){
			attack_character(d, c, next);
		}
	}
	//else you can change the position of the character
	else{
		//change the position of the character to the next spot
		c->position[dim_y] = next[dim_y];
		c->position[dim_x] = next[dim_x];
	}
  
	//add the character to the new spot in the dungeon
	d->character[c->position[dim_y]][c->position[dim_x]] = c;
}

/*OLD*/
//takes care of moving characters in the dungeon
/*void move_character(dungeon_t *d, character_t *c, pair_t next){
  //empty they current spot the character is standing
  d->character[c->position[dim_y]][c->position[dim_x]] = NULL;
    
  //change the position of the character to the next spot
  c->position[dim_y] = next[dim_y];
  c->position[dim_x] = next[dim_x];
  
  //if there is something at the next spot, you need to fight it
  if (d->character[c->position[dim_y]][c->position[dim_x]]) {
    d->character[c->position[dim_y]][c->position[dim_x]]->alive = 0;
	//if the character dies and is a monster, decrease the number of monsters
    if (d->character[c->position[dim_y]][c->position[dim_x]] != &d->pc) {
      d->num_monsters--;
    }
  }
  
  //add the character to the new spot in the dungeon
  d->character[c->position[dim_y]][c->position[dim_x]] = c;
}*/

void do_moves(dungeon_t *d)
{
  pair_t next;
  character_t *c;

  /* Remove the PC when it is PC turn.  Replace on next call.  This allows *
   * us to completely uninit the heap when generating a new level without  *
   * worrying about deleting the PC.                                       */

  if (pc_is_alive(d)) {
    heap_insert(&d->next_turn, &d->pc);
  }

  while (pc_is_alive(d) && ((c = heap_remove_min(&d->next_turn)) != &d->pc)) {
    if (!c->alive) {
      if (d->character[c->position[dim_y]][c->position[dim_x]] == c) {
        d->character[c->position[dim_y]][c->position[dim_x]] = NULL;
      }
      if (c != &d->pc) {
        character_delete(c);
      }
      continue;
    }

    c->next_turn += (1000 / c->speed);

    npc_next_pos(d, c, next);
    move_character(d, c, next);

    heap_insert(&d->next_turn, c);
  }

  //if it is the pc's turn
  if (pc_is_alive(d) && c == &d->pc) {
	  int i;
	  
	  //if the pc is standing on an item, pick it up
	  if(d->object[c->position[dim_y]][c->position[dim_x]]){
	      //find the next available inventory slot, if unavailable, don't pick up the item
		  for(i=0;i<10;i++){
		    if(!d->pc.pc->carry[i]){
				d->pc.pc->carry[i] = d->object[c->position[dim_y]][c->position[dim_x]];
				d->object[c->position[dim_y]][c->position[dim_x]] = 0;
				break;
			}
		  }
	  }
	  
	  //pc's stats are updated based on equipped items immediately in io.c
	  //update the speed of the pc
	  c->next_turn += (1000 / c->speed);
  }
}

void dir_nearest_wall(dungeon_t *d, character_t *c, pair_t dir)
{
  dir[dim_x] = dir[dim_y] = 0;

  if (c->position[dim_x] != 1 && c->position[dim_x] != DUNGEON_X - 2) {
    dir[dim_x] = (c->position[dim_x] > DUNGEON_X - c->position[dim_x] ? 1 : -1);
  }
  if (c->position[dim_y] != 1 && c->position[dim_y] != DUNGEON_Y - 2) {
    dir[dim_y] = (c->position[dim_y] > DUNGEON_Y - c->position[dim_y] ? 1 : -1);
  }
}

uint32_t in_corner(dungeon_t *d, character_t *c)
{
  uint32_t num_immutable;

  num_immutable = 0;

  num_immutable += (mapxy(c->position[dim_x] - 1,
                          c->position[dim_y]    ) == ter_wall_immutable);
  num_immutable += (mapxy(c->position[dim_x] + 1,
                          c->position[dim_y]    ) == ter_wall_immutable);
  num_immutable += (mapxy(c->position[dim_x]    ,
                          c->position[dim_y] - 1) == ter_wall_immutable);
  num_immutable += (mapxy(c->position[dim_x]    ,
                          c->position[dim_y] + 1) == ter_wall_immutable);

  return num_immutable > 1;
}

static void new_dungeon_level(dungeon_t *d, uint32_t dir)
{
  /* Eventually up and down will be independantly meaningful. *
   * For now, simply generate a new dungeon.                  */

  switch (dir) {
  case '<':
  case '>':
    new_dungeon(d);
    break;
  default:
    break;
  }
}

uint32_t move_pc(dungeon_t *d, uint32_t dir)
{
  pair_t next;
  uint32_t was_stairs = 0;

  next[dim_y] = d->pc.position[dim_y];
  next[dim_x] = d->pc.position[dim_x];

  switch (dir) {
  case 1:
  case 2:
  case 3:
    next[dim_y]++;
    break;
  case 4:
  case 5:
  case 6:
    break;
  case 7:
  case 8:
  case 9:
    next[dim_y]--;
    break;
  }
  switch (dir) {
  case 1:
  case 4:
  case 7:
    next[dim_x]--;
    break;
  case 2:
  case 5:
  case 8:
    break;
  case 3:
  case 6:
  case 9:
    next[dim_x]++;
    break;
  case '<':
    if (mappair(d->pc.position) == ter_stairs_up) {
      was_stairs = 1;
      new_dungeon_level(d, '<');
    }
    break;
  case '>':
    if (mappair(d->pc.position) == ter_stairs_down) {
      was_stairs = 1;
      new_dungeon_level(d, '>');
    }
    break;
  }

  if (was_stairs) {
    return 0;
  }

  if ((dir != '>') && (dir != '<') && (mappair(next) >= ter_floor)) {
    move_character(d, &d->pc, next);
    io_update_offset(d);
    dijkstra(d);

    return 0;
  }

  return 1;
}
