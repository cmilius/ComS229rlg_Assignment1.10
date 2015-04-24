#include "dice.h"
#include "utils.h"

int32_t dice::roll(void) const
{
  int32_t total;
  uint32_t sid = (uint32_t)sides;		//force it to a uint32_t to avoid floating point exception
  uint32_t i;

  total = base;

  if (sid) {
    for (i = 0; i < number; i++) {
      total += rand_range(1, sid);
    }
  }

  return total;
}

dice_t *new_dice(int32_t base, uint32_t number, uint32_t sides)
{
  return (dice_t *) new dice(base, number, sides);
}

void destroy_dice(dice_t *d)
{
  delete (dice *) d;
}

int32_t roll_dice(const dice_t *d)
{
  return ((dice *) d)->roll();
}

std::ostream &dice::print(std::ostream &o)
{
  return o << base << '+' << number << 'd' << sides;
}
