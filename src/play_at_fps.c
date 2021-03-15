#define _POSIX_C_SOURCE 200112L

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <time.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

const char clear_screen[] = {0x1b, '[', 'H', 0x1b, '[', '2', 'J', 0x1b, '[', '3', 'J'};
const char reset_cursor[] = {0x1b, '[', 'H'};

enum arg_type {
	INT,
	UINT,
	LONG,
	ULONG,
	LLONG,
	ULLONG,
};

struct timespec add_timespecs(struct timespec t1, struct timespec t2) {
	t1.tv_nsec += t2.tv_nsec;
	t1.tv_sec += t2.tv_sec;
	if (t1.tv_nsec > 999999999) {
		t1.tv_nsec -= 1000000000;
		t1.tv_sec += 1;
	}
	return t1;
}

bool verify_format(const char *format, enum arg_type *arg_type_ret) {
	bool is_signed;
	int l_count = 0;
	if (!format) return false; // NULL format string
	while (*format && *format != '%') ++format; // find first '%' or EOS
	if (!*format) return false; // No '%' found
	++format; // skip '%'
	while (*format >= '0' && *format <= '9') ++format; // skip field width and zero padding
	while (true) {
		if (*format == 'l') ++l_count;
		else if (*format == 'L') l_count += 2;
		else if (*format == 'q') l_count += 2;
		else if (!*format) return false; // invalid format specifier (no conversion specifier)
		else break; // not a length modifier
		++format;
	}
	if (*format == 'd' || *format == 'i') {
		is_signed = true;
	} else if (*format == 'u' || *format == 'o' || *format == 'x' || *format == 'X') {
		is_signed = false;
	} else { // invalid format specifier
		return false;
	}

	while (*format && *format != '%') ++format; // find next '%' or EOS
	if (*format) return false; // Extra '%' found

	if (l_count > 2) return false;

	if (arg_type_ret) {
		if (is_signed) {
			if (l_count == 0) *arg_type_ret = INT;
			else if (l_count == 1) *arg_type_ret = LONG;
			else if (l_count == 2) *arg_type_ret = LLONG;
		} else {
			if (l_count == 0) *arg_type_ret = UINT;
			else if (l_count == 1) *arg_type_ret = ULONG;
			else if (l_count == 2) *arg_type_ret = ULLONG;
		}
	}
	return true;
}

int main(int argc, char **argv) {
	int fps, check;
	const char *format;
	const size_t buf_sz = 2 * 1024 * 1024;
	char *buf;
	bool any_found = false;
	int fd;
	char filename[256];
	ssize_t bytes_read, bytes_written;
	struct timespec current, interval, remain;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <fps> <filename format>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	check = sscanf(argv[1], "%d", &fps);
	if (check != 1) {
		fprintf(stderr, "Invalid fps: '%s'\nUsage: %s <fps> <filename format>\n", argv[1], argv[0]);
		exit(EXIT_FAILURE);
	}

	format = argv[2];
	if (!verify_format(format, NULL)) {
		fprintf(stderr, "Invalid format: '%s'\nUsage: %s <fps> <filename format>\n", argv[2], argv[0]);
		exit(EXIT_FAILURE);
	}

	buf = malloc(buf_sz);
	if (!buf) {
		fprintf(stderr, "Allocation failed\n");
		exit(EXIT_FAILURE);
	}

	check = clock_gettime(CLOCK_MONOTONIC, &current);
	if (check != 0) {
		fprintf(stderr, "Failed to get current time.\n");
		exit(EXIT_FAILURE);
	}
	interval.tv_sec = 0;
	interval.tv_nsec = 1000000000 / fps;


	bytes_written = write(STDOUT_FILENO, clear_screen, sizeof(clear_screen));
	if (bytes_written < 0 || (size_t)bytes_written < sizeof(clear_screen)) {
		fprintf(stderr, "Failed to clear screen.\n");
		exit(EXIT_FAILURE);
	}
	for (unsigned long index = 0; true; ++index) {
		check = snprintf(filename, 255, format, index);
		if (check >= 255) { // filename was truncated
			fprintf(stderr, "Filename too long.\n");
			exit(EXIT_FAILURE);
		}
		fd = open(filename, O_RDONLY);
		if (fd < 0 && index == 0) {
			// Ignore errors for index 0, in case data starts at 1
			continue;
		} else if (fd < 0 && !any_found) {
			fprintf(stderr, "Failed to open any files.\n");
			exit(EXIT_FAILURE);
		} else if (fd < 0) {
			// Assume we have reached the end of the files
			break;
		}
		any_found = true;
		bytes_read = read(fd, buf, buf_sz); // TODO: handle larger files than buf_sz and partial reads
		if (bytes_read < 0) { // Failed to read
			fprintf(stderr, "Failed to read from file number %lu.\n", index);
			exit(EXIT_FAILURE);
		}
		bytes_written = write(STDOUT_FILENO, reset_cursor, sizeof(reset_cursor));
		if (bytes_written < 0 || (size_t)bytes_written < sizeof(reset_cursor)) {
			fprintf(stderr, "Failed to write to stdout on file number %lu.\n", index);
			exit(EXIT_FAILURE);
		}
		bytes_written = write(STDOUT_FILENO, buf, (size_t)bytes_read);// TODO: handle partial writes
		if (bytes_written < 0) {
			fprintf(stderr, "Failed to write to stdout on file number %lu.\n", index);
			exit(EXIT_FAILURE);
		}
		close(fd);
		current = add_timespecs(current, interval);
		while (true) {
			check = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &current, &remain);
			if (check == 0) break; // sleeping succeeded
			else if (check == EINTR) continue; // sleeping was interrupted
			else {
				fprintf(stderr, "Failed to sleep on file number %lu.\n", index);
				exit(EXIT_FAILURE);
			}
		}
	}
	return EXIT_SUCCESS;
}
