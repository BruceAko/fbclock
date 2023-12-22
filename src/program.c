/*==========================================================================

  fbclock 
  program.c
  Distributed under the terms of the GPL v3.0

  This file contains the main body of the program.

==========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <signal.h>
#include <math.h>
#include "program_context.h" 
#include "feature.h" 
#include "program.h" 
#include "string.h" 
#include "file.h" 
#include "list.h" 
#include "numberformat.h" 
#include "framebuffer.h"
#include "region.h"
#include "fbanalogclock.h"

#define DEF_WIDTH 300
#define DEF_HEIGHT 300
#define DEF_POSITION_X 100 
#define DEF_POSITION_Y 30 
#define DEF_TRANSPARENCY 50

// All these variables have to be global, because they are
//  used by the signal handler
FrameBuffer *fb = NULL; 
Region* old_region = NULL;
int transparency = 40;
static Region *wallpaper_region = NULL; 
static int position_x = -1;
static int position_y = -1;

/*==========================================================================

  program_signal_usr2

  When a URS2 is received, it means that the background has been
  redrawn, so we must redraw also, using the new background. So
  we have to sample the background from the framebuffer, and then
  draw on top of this sample

==========================================================================*/
void program_signal_usr2 (int dummy)
  {
  region_from_fb (wallpaper_region, fb, position_x, position_y);
  region_darken (wallpaper_region, transparency);
  }


/*==========================================================================

  program_check_context

==========================================================================*/
BOOL program_check_context (const ProgramContext *context)
  {
  LOG_IN
  BOOL ret = TRUE;
 
  if (ret)
    {
    position_x = program_context_get_integer (context, "x", DEF_POSITION_X);
    position_y = program_context_get_integer (context, "y", DEF_POSITION_Y);
    int w = program_context_get_integer (context, "width", DEF_WIDTH);
    int h = program_context_get_integer (context, "height", DEF_HEIGHT);

    if (position_x + w > framebuffer_get_width (fb)
        || position_y + h > framebuffer_get_height (fb)
        || position_x < 0 || position_y < 0)
      {
      log_error ("Position is out of bounds, compared to framebuffer size");
      ret = FALSE;
      }
    }
  
  if (ret)
    {
    int t = program_context_get_integer 
      (context, "transparency", DEF_TRANSPARENCY);
    if (t < 0 || t > 100)
      {
      log_error ("Transparency is a percentage, 0-100");
      ret = FALSE;
      }
    }

  LOG_OUT
  return ret;
  }


/*==========================================================================

  program_run

==========================================================================*/
int program_run (ProgramContext *context)
  {
  //char ** const argv = program_context_get_nonswitch_argv (context);
  //int argc = program_context_get_nonswitch_argc (context);

  log_set_level (program_context_get_integer (context, "log-level", 
      LOG_WARNING));
  const char *fbdev = "/dev/fb0";
  const char *arg_fbdev = program_context_get (context, "fbdev");
  if (arg_fbdev) fbdev = arg_fbdev;

  fb = framebuffer_create (fbdev);
  char *error = NULL;
  framebuffer_init (fb, &error);
  if (error == NULL)
    {
    if (program_check_context (context))
      {
      int width = program_context_get_integer (context, "width", DEF_WIDTH);
      int height = program_context_get_integer (context, "height", DEF_HEIGHT);
      // Position was already set in check_context

      int transparency = program_context_get_integer 
        (context, "transparency", DEF_TRANSPARENCY);
      int speed = program_context_get_integer(context, "speed", 1);
      BOOL seconds = program_context_get_boolean 
         (context, "seconds", TRUE); 
      BOOL date = program_context_get_boolean 
         (context, "date", TRUE);

      CustomTime* customTime = NULL;
      if(program_context_get_boolean(context, "customTimeEnabled", FALSE)){
        CustomTime customTimeStruct = {
          .hr = program_context_get_integer(context, "customTimeHr", DEF_TRANSPARENCY),
          .min = program_context_get_integer(context, "customTimeMin", DEF_TRANSPARENCY),
          .sec = program_context_get_integer(context, "customTimeSec", DEF_TRANSPARENCY),
        };
        customTime = &customTimeStruct;
      }
     
      log_debug ("Clock area width is %d", width); 
      log_debug ("Clock TL corner is (%d, %d)", position_x, position_y);
      log_debug ("Clock background transparency is %d%%", transparency); 
      wallpaper_region = region_create (width, height);
      region_from_fb (wallpaper_region, fb, position_x, position_y);
      region_darken (wallpaper_region, transparency);

      signal (SIGUSR2, program_signal_usr2); 
      BOOL stop = FALSE;
      while (!stop)
      {
        Region *r = region_clone (wallpaper_region);

        program_draw_clock_in_region (r, seconds, date, customTime);

        region_to_fb (r, fb, position_x, position_y, old_region);

        if (old_region != NULL) region_destroy (old_region);

        old_region = r;
      
        int time;
        if (seconds){
          if(customTime!=NULL){
              customTime->sec++;
              if(customTime->sec==60){
                  customTime->sec=0;
                  customTime->min++;
              }
          }
          time = 1000000 / speed;
          usleep(time);
        }
        else{
          if(customTime!=NULL){
              customTime->min++;
              if(customTime->min==60){
                  customTime->min=0;
                  customTime->hr++;
              }
          }
          time = 60000000 / speed;
          usleep(time);
        }
      }

      region_destroy (wallpaper_region);
      framebuffer_deinit (fb);
      }
    else
      {
      // Do nothing -- error already reported
      }
    framebuffer_destroy (fb);
    }
  else
    {
    log_error (error);
    free (error);
    }

  return 0;
  }

