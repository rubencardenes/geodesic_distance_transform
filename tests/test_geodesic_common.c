/* Unit tests for the pure helper functions in geodesic_common.c.
 * No external test framework: keeps the build dependency-free. */
#include <stdio.h>
#include <math.h>
#include "../geodesic_common.h"

static int tests_run = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) \
  do { \
    tests_run++; \
    if (!(cond)) { \
      tests_failed++; \
      printf("FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
    } \
  } while (0)

#define CHECK_FLOAT_EQ(a, b, msg) CHECK(fabs((a) - (b)) < 1e-4, msg)

static void test_mapIndex2D(void) {
  CHECK(mapIndex2D(0, 0, 10, 10) == 0, "mapIndex2D origin");
  CHECK(mapIndex2D(2, 3, 10, 10) == 23, "mapIndex2D interior point");
  CHECK(mapIndex2D(9, 9, 10, 10) == 99, "mapIndex2D last valid point");
  CHECK(mapIndex2D(-1, 0, 10, 10) == -1, "mapIndex2D negative row is out of bounds");
  CHECK(mapIndex2D(0, -1, 10, 10) == -1, "mapIndex2D negative col is out of bounds");
  CHECK(mapIndex2D(10, 0, 10, 10) == -1, "mapIndex2D row == nr is out of bounds");
  CHECK(mapIndex2D(0, 10, 10, 10) == -1, "mapIndex2D col == nc is out of bounds");
}

static void test_mapIndex3D(void) {
  CHECK(mapIndex3D(0, 0, 0, 4, 5, 6) == 0, "mapIndex3D origin");
  CHECK(mapIndex3D(1, 2, 3, 4, 5, 6) == 2 + 1 * 5 + 3 * 4 * 5, "mapIndex3D interior point");
  CHECK(mapIndex3D(-1, 0, 0, 4, 5, 6) == -1, "mapIndex3D negative row is out of bounds");
  CHECK(mapIndex3D(0, 0, 6, 4, 5, 6) == -1, "mapIndex3D slice == nz is out of bounds");
}

static void test_distance(void) {
  CHECK_FLOAT_EQ(distance(0, 0, 3, 4), 5.0, "distance 3-4-5 triangle");
  CHECK_FLOAT_EQ(distance(2, 2, 2, 2), 0.0, "distance to self is zero");
  CHECK_FLOAT_EQ(distance3d(0, 0, 0, 1, 2, 2), 3.0, "distance3d matches pythagorean triple");
}

static void test_countFloatsInString(void) {
  CHECK(countFloatsInString("1 2 3") == 3, "countFloatsInString three numbers");
  CHECK(countFloatsInString("  1.5   -2.25 ") == 2, "countFloatsInString handles whitespace/signs");
  CHECK(countFloatsInString("") == -1, "countFloatsInString empty string is malformed");
  CHECK(countFloatsInString("   ") == -1, "countFloatsInString whitespace-only string is malformed");
  CHECK(countFloatsInString("1 2 abc") == 2, "countFloatsInString stops at trailing garbage");
}

static void test_getFloatString(void) {
  float vals[3] = {0};
  CHECK(getFloatString(3, "1 2 3", vals) == 0, "getFloatString parses three numbers");
  CHECK_FLOAT_EQ(vals[0], 1.0, "getFloatString first value");
  CHECK_FLOAT_EQ(vals[1], 2.0, "getFloatString second value");
  CHECK_FLOAT_EQ(vals[2], 3.0, "getFloatString third value");

  /* getFloatString requires an exact count match: it rejects a string that
   * contains more (or fewer) floats than requested. */
  CHECK(getFloatString(2, "1 2 3", vals) == 1, "getFloatString fails when more values than requested");

  CHECK(getFloatString(3, "1 2", vals) == 1, "getFloatString fails when fewer values than requested");
}

int main(void) {
  test_mapIndex2D();
  test_mapIndex3D();
  test_distance();
  test_countFloatsInString();
  test_getFloatString();

  printf("%d/%d tests passed\n", tests_run - tests_failed, tests_run);
  return tests_failed == 0 ? 0 : 1;
}
