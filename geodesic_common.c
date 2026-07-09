/* Shared pure helper functions used by both the 2D (C) and 3D (C++) tools. */
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include "geodesic_common.h"

int mapIndex2D(int r, int c, int nr, int nc)
{
  if (c >= nc) return -1;
  if (c < 0) return -1;
  if (r >= nr) return -1;
  if (r < 0) return -1;
  return c + r * nc;
}

int mapIndex3D(int r, int c, int z, int nr, int nc, int nz)
{
  if (c >= nc) return -1;
  if (c < 0) return -1;
  if (r >= nr) return -1;
  if (r < 0) return -1;
  if (z >= nz) return -1;
  if (z < 0) return -1;
  return c + r * nc + z * nr * nc;
}

float distance(int x1, int y1, int x2, int y2)
{
  return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

float distance3d(int x1, int y1, int z1, int x2, int y2, int z2)
{
  return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2) + (z1 - z2) * (z1 - z2));
}

int countFloatsInString(const char *fltString)
/* char *flts - character array of floats
 Return -1 on a malformed string
 Return the count of the number of floating point numbers
*/
{
  char *end;
  const char *start = fltString;
  double d;
  int count = 0;
  while ((UCHAR(*start) != '\0') && isspace(UCHAR(*start))) { start++; }
  if (UCHAR(*start) == '\0') return -1; /* Don't ask to convert empty strings */
  do {
    d = strtod(start, (char **)&end);
    if (end == start) {
      /* I want to parse strings of numbers with comments at the end */
      /* This return is executed when the next thing along can't be parsed */
      return count;
    }
    count++;   /* Count the number of floats */
    start = end;  /* Keep converting from the returned position. */
    while ((UCHAR(*start) != '\0') && isspace(UCHAR(*start))) { start++; }
  } while (UCHAR(*start) != '\0');
  return count; /* Success */
}

int getFloatString(int numFloats, const char *flts, float *tgts)
/* char *flts - character array of floats */
/* float *tgts - array to save floats */
{
  char *end;
  const char *start = flts;
  double d;
  int count = 0;
  if (countFloatsInString(flts) != numFloats) return 1;
  while ((UCHAR(*start) != '\0') && isspace(UCHAR(*start))) { start++; }
  do {
    d = strtod(start, (char **)&end);
    if (end == start) {
      /* Can't do any more conversions on this line */
      return (count == numFloats) ? 0 : 1;
    }
    tgts[count++] = d;   /* Convert from double to float */
    start = end;  /* Keep converting from the returned position. */
    while ((UCHAR(*start) != '\0') && isspace(UCHAR(*start))) { start++; }
    if (count == numFloats) return 0; /* I don't care if there are more on
                the line as long as I got what I wanted */
  } while (UCHAR(*start) != '\0');
  return 0; /* Success */
}
