/*============================================================================
  
  fbclock 
  fbanalogclock.h 

============================================================================*/

#pragma once

#include "defs.h"
#include "region.h"

typedef struct _CustomTime {
  int hr;
  int min;
  int sec;
} CustomTime;

BEGIN_DECLS

void program_draw_clock_in_region(Region* r, BOOL seconds, BOOL date, CustomTime* customTime);

END_DECLS
