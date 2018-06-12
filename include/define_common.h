#ifndef DEFINE_COMMON_H
#define DEFINE_COMMON_H

#define PRINT_DQT(dqt) \
  for (int __i = 0; __i < 8; __i++){ \
    printf ("\n("); \
    for (int __j = 0; __j < 8; __j++){ \
      printf ("%i\t", dqt[(__i*8)+__j]); \
    } \
    printf (")"); \
  }


#define SWITCH_ENDIAN_32(x)   \
	x = ((x & 0x000000ff) << 24) | ((x & 0x0000ff00) << 8)  |  \
((x & 0x00ff0000) >> 8)  | ((x & 0xff000000) >> 24)

#define SWITCH_ENDIAN_16(x) \
	x = ((x & 0x00ff) << 8) | ((x & 0xff00) >> 8)
#define CPU_DATA_IS_BIGENDIAN(type,data) SWITCH_ENDIAN_##type(data)


#define FILE_ERROR(file, msg, expected, actual) \
        fprintf(stderr, "Error reading input file " msg " (%d elements read, expected %d)\n", \
                actual, expected); \
        if (feof(file)) fprintf(stderr, "(end of file)\n"); \
        /* exit(1) */

#define NEXT_TOKEN(res) {                                       \
	if (fread ((void *) & res, 1, 1, movie) != 1) {         \
		FILE_ERROR(movie, "in NEXT_TOKEN", 1, 1);       \
	}                                                       \
}

#define COPY_SECTION(to, size) {		\
	int elem_read = fread ((void *)to, size, 1, movie); \
	if (elem_read != 1) {			\
		FILE_ERROR(movie, "in COPY_SECTION", elem_read, 1); \
                abort();\
	}					\
}

#define SKIP(n) {                                                       \
	uint8_t * waste=(uint8_t *)malloc(n);                           \
	if (n != 0) {                                                   \
                int elem_read = fread ((void *) waste, n, 1, movie);    \
		if (elem_read != 1) {                                   \
			FILE_ERROR(movie, "in SKIP", elem_read, n);     \
		}                                                       \
        }                                                               \
	free(waste);                                                    \
}


#ifdef VERBOSE
#define VPRINTF(format, ...) printf ("[%s] " format, __FUNCTION__, ## __VA_ARGS__)
#else
#define VPRINTF(format, ...)
#endif

#ifdef INFO
#define IPRINTF(format, ...) printf ("[%s] " format, __FUNCTION__, ## __VA_ARGS__)
#else
#define IPRINTF(format, ...)
#endif

#ifdef INTER
#define APRINTF(format, ...) printf (format, ## __VA_ARGS__)
#else
#define APRINTF(format, ...)
#endif

extern const uint8_t G_ZZ[64];

#endif //DEFINE_COMMON_H


