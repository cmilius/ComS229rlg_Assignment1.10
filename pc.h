#ifndef PC_H
# define PC_H

# include <stdint.h>

# include "dims.h"
#include "object.h"

typedef struct dungeon dungeon_t;

typedef struct pc {
  char name[40];
  char catch_phrase[80];
  //new stuff for the objects the pc is wearing
  /*holds all the equipped objects in this order:
  0. Weapon
  1. Offhand
  2. Ranged
  3. Armor
  4. Helmet
  5. Cloak
  6. Gloves
  7. Boots
  8. Amulet
  9. Light
  10. Ring1
  11. Ring2*/
  object_t* equip[12];
  
  //array for holding all the objects the pc is carrying
  object_t* carry[10];
  
  //temp object for swapping
  object_t* temp;
  
} pc_t;

void pc_delete(pc_t *pc);
uint32_t pc_is_alive(dungeon_t *d);
void config_pc(dungeon_t *d);
uint32_t pc_next_pos(dungeon_t *d, pair_t dir);
void place_pc(dungeon_t *d);

#endif
