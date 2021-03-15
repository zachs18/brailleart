#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "debug.h"
#include "safelib.h"
#include "pnmlib.h"

enum color {
	BLACK   = 0,
	RED     = 1,
	GREEN   = 2,
	YELLOW  = 3,

	BLUE    = 4,
	MAGENTA = 5,
	CYAN    = 6,
	WHITE   = 7,

	BRIGHT_BLACK   = 8,
	BRIGHT_RED     = 9,
	BRIGHT_GREEN   = 10,
	BRIGHT_YELLOW  = 11,

	BRIGHT_BLUE    = 12,
	BRIGHT_MAGENTA = 13,
	BRIGHT_CYAN    = 14,
	BRIGHT_WHITE   = 15
};

__m256d colors[] = {
	[BLACK  ] = {  0 / 255.,   0 / 255.,   0 / 255., 0.0},
	[RED    ] = {170 / 255.,   0 / 255.,   0 / 255., 0.0},
	[GREEN  ] = {  0 / 255., 170 / 255.,   0 / 255., 0.0},
	[YELLOW ] = {170 / 255.,  85 / 255.,   0 / 255., 0.0},
	[BLUE   ] = {  0 / 255.,   0 / 255., 170 / 255., 0.0},
	[MAGENTA] = {170 / 255.,   0 / 255., 170 / 255., 0.0},
	[CYAN   ] = {  0 / 255., 170 / 255., 170 / 255., 0.0},
	[WHITE  ] = {170 / 255., 170 / 255., 170 / 255., 0.0},

	[BRIGHT_BLACK  ] = { 85 / 255.,  85 / 255.,  85 / 255., 0.0},
	[BRIGHT_RED    ] = {255 / 255.,  85 / 255.,  85 / 255., 0.0},
	[BRIGHT_GREEN  ] = { 85 / 255., 255 / 255.,  85 / 255., 0.0},
	[BRIGHT_YELLOW ] = {255 / 255., 255 / 255.,  85 / 255., 0.0},
	[BRIGHT_BLUE   ] = { 85 / 255.,  85 / 255., 255 / 255., 0.0},
	[BRIGHT_MAGENTA] = {255 / 255.,  85 / 255., 255 / 255., 0.0},
	[BRIGHT_CYAN   ] = { 85 / 255., 255 / 255., 255 / 255., 0.0},
	[BRIGHT_WHITE  ] = {255 / 255., 255 / 255., 255 / 255., 0.0},
};

typedef uint64_t color_closeness_sequence; // 16 * 4 bits

double distance(const __m256d v1, const __m256d v2) {
	__m256d diff = v1 - v2;
	__m256d sq_diff = diff * diff;
	double ret = sq_diff[3] + sq_diff[2] + sq_diff[1] + sq_diff[0];
	return ret;
}

color_closeness_sequence closenesses(const __m256d pixel) {
	struct color_and_distance {
		enum color color;
		double distance;
	};
	int compare(const void *cd1, const void *cd2) {
		double d1 = ((struct color_and_distance*)cd1)->distance;
		double d2 = ((struct color_and_distance*)cd2)->distance;
		return (d1 > d2) - (d1 < d2);
	}
	struct color_and_distance colors_and_distances[16];
	for (int i = 0; i < 16; ++i) {
		colors_and_distances[i].color = i;
		colors_and_distances[i].distance = distance(pixel, colors[i]);
	}
	
	qsort(colors_and_distances, 16, sizeof(*colors_and_distances), compare);
	
	color_closeness_sequence ret = 0;
	for (int i = 0; i < 16; ++i) {
		ret |= ((unsigned long)colors_and_distances[i].color) << (4 * i);
	}
	return ret;
}

int main(int argc, char **argv) {
	(void)argc;
	(void)argv;

	struct pnmdata pnmdata;
	initpnm(&pnmdata);
	if (!freadpnm(&pnmdata, stdin)) {
		fprintf(stderr, "Couldn't read pnm from stdin.\n");
		return EXIT_FAILURE;
	}

	const int dimx = pnmdata.dimx;
	const int dimy = pnmdata.dimy;
	__m256d (*data)[dimx] = (__m256d(*)[dimx])pnmdata.rawdata;

	const int full_dimx = ((dimx-1) & -2) + 4;
	const int full_dimy = ((dimy-1) & -4) + 4;
	color_closeness_sequence (*colordata)[full_dimx] = scalloc(full_dimy, full_dimx * sizeof(**colordata));
	// TODO: lookup dithering algorithms.
	for (int y = 0; y < dimy; ++y) {
		for (int x = 0; x < dimx; ++x) {
			colordata[y][x] = closenesses(data[y][x]);
		}
	}

	const int braille_dimx = full_dimx / 2;
	const int braille_dimy = full_dimy / 4;

	unsigned char (*brailledata)[braille_dimx] = scalloc(braille_dimy, braille_dimx * sizeof(**brailledata));
/*
	for (int y = 0; y < braille_dimy; ++y) {
		for (int x = 0; x < braille_dimx; ++x) {
			brailledata[y][x] = 
				    1 * bindata[y*4+0][x*2+0]
				+   2 * bindata[y*4+1][x*2+0]
				+   4 * bindata[y*4+2][x*2+0]
				+   8 * bindata[y*4+0][x*2+1]
				+  16 * bindata[y*4+1][x*2+1]
				+  32 * bindata[y*4+2][x*2+1]
				+  64 * bindata[y*4+3][x*2+0]
				+ 128 * bindata[y*4+3][x*2+1];
		}
	}

	// Each braille unicode character is 3 bytes, plus 1 byte for newline, plus one byte for terminating NUL
	char *brailleart_line = smalloc((3 * braille_dimx + 2) * sizeof(*brailleart_line));
	for (int x = 0; x < braille_dimx; ++x) {
		brailleart_line[3*x] = 0xe2;
	}
	brailleart_line[3*braille_dimx] = '\0';
	// 0xe2's and NUL never change
	for (int y = 0; y < braille_dimy; ++y) {
		for (int x = 0; x < braille_dimx; ++x) {
			brailleart_line[3*x+1] = 0xa0 + (brailledata[y][x] >> 6);
			brailleart_line[3*x+2] = 0x80 + (brailledata[y][x] & 0x3f);
		}
		puts(brailleart_line);
	}
*/
	free(colordata);
	free(brailledata);
	reinitpnm(&pnmdata);
}

