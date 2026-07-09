/* Shared pure helper functions used by both the 2D (C) and 3D (C++) tools.
 * Extracted so they are defined once and can be unit tested in isolation. */
#ifndef GEODESIC_COMMON_H
#define GEODESIC_COMMON_H

#ifndef UCHAR
#define UCHAR(c) ((unsigned char)(c))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Maps 2D row/col coordinates to a flat index, or -1 if out of bounds. */
int mapIndex2D(int r, int c, int nr, int nc);

/* Maps 3D row/col/slice coordinates to a flat index, or -1 if out of bounds. */
int mapIndex3D(int r, int c, int z, int nr, int nc, int nz);

/* Euclidean distance between two 2D points. */
float distance(int x1, int y1, int x2, int y2);

/* Euclidean distance between two 3D points. */
float distance3d(int x1, int y1, int z1, int x2, int y2, int z2);

/* Counts the floating point numbers parseable from the start of fltString.
 * Returns -1 on an empty/whitespace-only string. */
int countFloatsInString(const char *fltString);

/* Parses numFloats floating point numbers from flts into tgts.
 * Returns 0 on success, 1 on failure (including a count mismatch). */
int getFloatString(int numFloats, const char *flts, float *tgts);

#ifdef __cplusplus
}
#endif

#endif /* GEODESIC_COMMON_H */
