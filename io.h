#ifndef IO_H
# define IO_H

# include "dungeon.h"

void io_init_terminal(void);
void io_reset_terminal(void);
void io_calculate_offset(dungeon_t *d);
void print_pc_stats(dungeon_t *d);
void io_display(dungeon_t *d);
void io_update_offset(dungeon_t *d);
void update_pc_stats(dungeon_t *d);
void io_display_slots(dungeon_t *d);
void io_wear_item(dungeon_t *d);
void io_remove_item(dungeon_t *d);
void io_drop_item(dungeon_t *d);
void io_expunge_item(dungeon_t *d);
void io_handle_input(dungeon_t *d);

#endif
