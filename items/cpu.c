/*
  Display information about cpu utilization
  The line is generated using format_string
  The specifier format is %{p,cN,C<sep>}

    p - Total processor utilization, as a percentage
    c - Utilization of the N'th core (starting from 0), as a percentage
    C - Utilization of each core, as a percentage, sperated by the string sep

  For p and cN, a conditional string can be specified as
    %{p,cN}{>,<,>=,<=,=,!=}M<string>
  Where M is an integer, and string is a line to display if the comparison is
  matched, e.g. %p>90<{ff0000}> would change the colour to red if the processor
  is more than 90% utilized

  The minimum display width of the item is calculated as the width in characters
  of the status line when the processor is fully utilized
*/


#include "../item.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../format.h"
#include "../log.h"



/*
  The ammount of cores plus 1
*/
static int cpu_count = 0;

/*
  The current cpu utilization, out of 100, with the exception of the first
  element, which should be a summation of the rest
*/
static int *cpu_usage;

/*
  The previous values read from /proc/stat
*/
static long int *cpu_sums;



enum cpu_calculate_state_t
  {
    CPU_READING_NAME_,
    CPU_READING_NAME_C,
    CPU_READING_NAME_CP
  };



static int
_cpu_initialize (void)
{
  FILE *fp;
  int state, c;

  if (cpu_count > 0 && cpu_usage && cpu_sums)
    return 1;

  // Run a DFA that just counts how often cpu appears

  state = CPU_READING_NAME_;

  fp = fopen ("/proc/stat", "r");
  if (!fp)
    {
      log_perror ("fopen: /proc/stat");
      return 0;
    }

  while ((c = fgetc (fp)) > 0)
    switch (state)
      {
        case CPU_READING_NAME_:
          if (c == 'c')
            state = CPU_READING_NAME_C;
          break;

        case CPU_READING_NAME_C:
          if (c == 'p')
            state = CPU_READING_NAME_CP;
          else
            state = CPU_READING_NAME_;
          break;

        case CPU_READING_NAME_CP:
          if (c == 'u')
            cpu_count++;
          state = CPU_READING_NAME_;
          break;
      }

  fclose (fp);

  if (cpu_count == 0)
    {
      log_error ("_cpu_initialize: cannot see you processors\n");
      return 0;
    }

  // Allocate the array's used
  if (!cpu_usage)
    {
      cpu_usage = malloc (sizeof (int) * (cpu_count));
      if (!cpu_usage)
        {
          log_perror ("_cpu_initialize: malloc");
          return 0;
        }
    }

  if (!cpu_sums)
    {
      cpu_sums = malloc (sizeof (long int) * (cpu_count));
      if (!cpu_sums)
        {
          log_perror ("_cpu_initialize: malloc");
          return 0;
        }

      // Initialize the sums to -1 so utilizations wont be calculated
      for (c = 0; c < cpu_count; c++)
        cpu_sums[c] = -1;
    }

  return 1;
}



static void
_cpu_recalculate (int id, int delay)
{
  static int previous_id = -1;
  static int jiffies = 0;
  FILE *fp;
  int state, i, c;
  int user, userl, system;

  if (!jiffies)
    {
      jiffies = sysconf (_SC_CLK_TCK);
      if (jiffies <= 0)
        jiffies = 100; // Just to ensure, default to 100
    }

  if (!_cpu_initialize ())
    return;

  if (id <= previous_id)
    return;

  previous_id = id;

  fp = fopen ("/proc/stat", "r");
  if (!fp)
    {
      log_perror ("fopen: /proc/stat");
      return;
    }

  state = CPU_READING_NAME_;

  while ((c = fgetc (fp)) > 0)
    switch (state)
      {
      case CPU_READING_NAME_:
        if (c == 'c')
          state = CPU_READING_NAME_C;
        break;

      case CPU_READING_NAME_C:
        if (c == 'p')
          state = CPU_READING_NAME_CP;
        else
          state = CPU_READING_NAME_;
        break;

      case CPU_READING_NAME_CP:
        if (c == 'u')
          {
            // Read and increment the integral suffix

            i = -1;
            while ((c = fgetc (fp)) > 0)
              {
                if (c < '0' || c > '9')
                  break;

                if (i < 0)
                  i = 0;

                i *= 10;
                i += c - '0';
              }
            i++;

            // Read the stats and put them in their place
            if (i >= 0 && i < cpu_count && fscanf (fp, " %d %d %d",
                                                   &user, &userl, &system) == 3)
              {
                // If this is the first time reading, set utilization to 100

                if (cpu_sums[i] < 0)
                  {
                    cpu_sums[i] = user + userl + system;
                    if (i > 0)
                      cpu_usage[i] = 100;
                    else
                      cpu_usage[i] = ((cpu_count == 1) ? 100
                                      : (cpu_count - 1) * 100);
                  }
                else
                  {
                    cpu_usage[i] = (user + userl + system) - cpu_sums[i];
                    cpu_sums[i] = (user + userl + system);
                    cpu_usage[i] = cpu_usage[i] * 100000 / delay / jiffies;
                  }
              }
          }
        state = CPU_READING_NAME_;
        break;
      }

  fclose (fp);
}



/*
  Callback used with format_string
*/
static const char*
_cpu_format_callback (const char **spec)
{
  static char buf[128];
  char *buf_pos;
  const char *str;
  int i, printed;
  size_t len, remain;
  float f;

  if (!spec || !*spec || !cpu_usage)
    return NULL;

  buf[0] = 0;

  switch (**spec)
    {
      case 'p': // Total percentage utilization
        f = cpu_usage[0] / ((cpu_count == 1) ? 1.0f : (float) (cpu_count - 1));
        (*spec)++;

        str = format_read_comparison (spec, (long int) f);
        if (str)
          return str;

        snprintf (buf, sizeof (buf), "%.1f", f);

        return buf;
        break;

      case 'c': // Utilization on a core given
        (*spec)++;

        for (i = 0; **spec >= '0' && **spec <= '9'; (*spec)++)
          {
            i *= 10;
            i += **spec - '0';
          }
        i++;

        if (i > 0 && i < cpu_count)
          {
            str = format_read_comparison (spec, cpu_usage[i]);
            if (str)
              return str;

            snprintf (buf, sizeof (buf), "%d", cpu_usage[i]);
            return buf;
          }

        break;

      case 'C': // Utilization on all cores
        (*spec)++;
        len = 0;
        if (**spec == '<')
          {
            (*spec)++;
            str = *spec;
            while (**spec && **spec != '>')
              {
                (*spec)++;
                len++;
              }

            if (**spec)
              (*spec)++;
          }
        else
          {
            len = 3;
            str = " | ";
          }

        remain = sizeof (buf) - 1;
        buf_pos = buf;
        for (i = 1; i < cpu_count; i++)
          {
            if (i > 1 && len)
              {
                strncat (buf_pos, str, (len > remain) ? remain : len);
                remain = (len > remain) ? 0 : (remain - len);
                buf_pos += (len > remain) ? remain : len;
              }

            printed = snprintf (buf_pos, remain + 1, "%d", cpu_usage[i]);

            if (printed > 0)
              {
                remain -= printed;
                buf_pos += printed;
              }
          }

        return buf;
        break;
    }

  return NULL;
}



static int
_cpu_update (item_t *item, int id, int delay)
{
  _cpu_recalculate (id, delay);

  if (item && cpu_usage)
    format_string (&item->text, item->line, _cpu_format_callback);

  return 500 - (item_get_time (0)->msec % 500);
}



static void
_cpu_finish (item_t *item, int free_item)
{
  item_finish (item, free_item);

  if (cpu_usage)
    free (cpu_usage);

  if (cpu_sums)
    free (cpu_sums);

  cpu_usage = NULL;
  cpu_sums  = NULL;
  cpu_count = 0;
}



item_t*
item_driver_cpu (char *line, char *block)
{
  item_t *item;

  item = item_alloc (sizeof (item_t), line, block);
  if (!item)
    return NULL;

  item->update = _cpu_update;
  item->finish = _cpu_finish;

  _cpu_update (item, 0, 0);
  item->min_length = item_current_length (item, 0);

  return item;
}
