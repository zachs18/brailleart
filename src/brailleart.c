#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "debug.h"
#include "safelib.h"
#include "pnmlib.h"

double intensity(const __m256d pixel) {
	const __m256d multiplier = {0.2989, 0.5870, 0.1140, 0.0};
	const __m256d intensities = pixel * multiplier;
	return intensities[0] + intensities[1] + intensities[2] + intensities[3];
}

int main(int argc, char **argv) {
	bool invert = false;
	double threshold = 0.5;
	if (argc >= 2) {
		invert = !strcmp("invert", argv[1]);
	}
	if (argc >= 2) {
		threshold = atof(argv[argc-1]);
	}

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
	bool (*bindata)[full_dimx] = scalloc(full_dimy, full_dimx * sizeof(**bindata));

	for (int y = 0; y < dimy; ++y) {
		for (int x = 0; x < dimx; ++x) {
			bindata[y][x] = invert ^ (intensity(data[y][x]) > threshold);
		}
	}

	const int braille_dimx = full_dimx / 2;
	const int braille_dimy = full_dimy / 4;

	unsigned char (*brailledata)[braille_dimx] = scalloc(braille_dimy, braille_dimx * sizeof(**brailledata));

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
	free(brailleart_line);
	free(brailledata);
	free(bindata);
	reinitpnm(&pnmdata);
}

