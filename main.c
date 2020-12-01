#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>

#include <unicode/ucnv.h>
#include <unicode/utypes.h>

#define UTF8 "UTF-8"
#define UTF16_PLATFORM_ENDIAN "UTF16_PlatformEndian"
#define UTF16 UTF16_PLATFORM_ENDIAN
#define CP1252 "ibm-5348_P100-1997"
#define FROM_CHARSET UTF16
#define TO_CHARSET UTF8

void convert(const char *argv[], UConverter *to_cnv, UConverter *from_cnv);

int main(int argc, char *argv[])
{
	UErrorCode uerrc = 0;
	UConverter *to_cnv;
	UConverter *from_cnv;

#if 0
	/* This block just lists available converters and their aliases. */

	const int32_t n = ucnv_countAvailable();
	int32_t to_charset_id  = -1;
	int32_t from_charset_id = -1;
	int32_t i = 0;

	for (; i < n /* && !(from_charset_id+1 && to_charset_id+1)*/; ++i) {
		int32_t j = 0;
		const char *converter_name = ucnv_getAvailableName(i);

		if (strcmp(converter_name, TO_CHARSET) == 0)
			from_charset_id = i;
		else if (strcmp(converter_name, FROM_CHARSET) == 0)
			to_charset_id = i;

		printf("%3d: %s\n", i, converter_name);
		for (; !U_FAILURE(uerrc) && j < ucnv_countAliases(converter_name, &uerrc); ++j) {
			const char *alias = ucnv_getAlias(converter_name, j, &uerrc);
			if (!U_FAILURE(uerrc))
				printf("\t-> alias: %s\n", alias);
		}
	}

	/*
	 *if (i == n) {
	 *        fputs("I didn't find both converters. Sayonara.\n", stderr);
	 *        exit(EXIT_FAILURE);
	 *}
	 */

	printf(TO_CHARSET ":  %d\n" FROM_CHARSET ": %d\n", to_charset_id, from_charset_id);
#endif /* 0 */

	to_cnv = ucnv_open(TO_CHARSET, &uerrc);
	if (U_FAILURE(uerrc)) {
		fprintf(stderr, "ucnv_open(%s), failed: %s\n", TO_CHARSET, u_errorName(uerrc));
		exit(EXIT_FAILURE);
	}

	from_cnv = ucnv_open(FROM_CHARSET, &uerrc);
	if (U_FAILURE(uerrc)) {
		fprintf(stderr, "ucnv_open(%s), failed: %s\n", FROM_CHARSET, u_errorName(uerrc));
		exit(EXIT_FAILURE);
	}

	ucnv_setToUCallBack(to_cnv, UCNV_TO_U_CALLBACK_STOP, NULL, NULL, NULL, &uerrc);
	if (U_FAILURE(uerrc))
		exit(EXIT_FAILURE);

	ucnv_setToUCallBack(from_cnv, UCNV_TO_U_CALLBACK_STOP, NULL, NULL, NULL, &uerrc);
	if (U_FAILURE(uerrc))
		exit(EXIT_FAILURE);

	fprintf(stderr, "Opened converters:\n"
			"\t-> To   ("  TO_CHARSET "):  %p\n"
			"\t-> From (" FROM_CHARSET "): %p\n",
			(void *) to_cnv,
			(void *) from_cnv);

	/* Use stdin if the user gave no arguments */
	if (argc == 1) {
		const char *new_argv[] = { argv[0], "-", NULL };
		convert(new_argv, to_cnv, from_cnv);
	}
	else {
		convert((const char **) argv, to_cnv, from_cnv);
	}

	ucnv_close(to_cnv);
	ucnv_close(from_cnv);

	return 0;
}

void convert(const char *argv[], UConverter *to_cnv, UConverter *from_cnv)
{
	char *buf = NULL;
	char *outbuf = NULL;
	long blksize = 0;
	long outblksize = 0;
	const char *filename;
	const uint8_t tomaxcharsize = ucnv_getMaxCharSize(to_cnv);
	const uint8_t frommincharsize_minus1 = ucnv_getMinCharSize(from_cnv) - 1;

#define CALCULATE_OUTBLKSIZE(blksize) ( \
	tomaxcharsize * \
	((blksize) + frommincharsize_minus1 ) / (frommincharsize_minus1 + 1))

	while ((filename = *++argv) != NULL) {
		int fd;
		size_t r;
		struct stat st;
		UErrorCode err = 0;

		/* Treat '-' as stdin */

		FILE *f = strcmp("-", filename) ? fopen(filename, "rb") : stdin;

		if (!f) {
			fprintf(stderr, "Failed to open file %s: %m\n", filename);
			break;
		}

		if ((fd = fileno(f)) == -1) {
			fprintf(stderr, "fileno(%s) returned -1: %m\n", filename);
			if (f != stdin && fclose(f) == EOF)
				fprintf(stderr, "Failed to close file %s: %m\n", filename);
			break;
		}

		if (fstat(fd, &st) == -1) {
			fprintf(stderr, "stat(%s) failed: %m\n", filename);
			if (f != stdin && fclose(f) == EOF)
				fprintf(stderr, "Failed to close file %s: %m\n", filename);
			break;
		}

		/* If block size is greater than the size of the memory
		 * currently allocated then allocate more. */

		if (blksize < st.st_blksize) {
			blksize = st.st_blksize;
			if ((buf = realloc(buf, blksize)) == NULL) {
				fprintf(stderr, "malloc(%ld) failed: %m\n", blksize);
				if (f != stdin && fclose(f) == EOF)
					fprintf(stderr, "Failed to close file %s: %m\n", filename);
				break;
			}
		}


		if (outblksize < CALCULATE_OUTBLKSIZE(blksize)) {
			outblksize = CALCULATE_OUTBLKSIZE(blksize);
			if ((outbuf = realloc(outbuf, outblksize)) == NULL) {
				fprintf(stderr, "malloc(%ld) failed: %m\n", outblksize);
				if (f != stdin && fclose(f) == EOF)
					fprintf(stderr, "Failed to close file %s: %m\n", filename);
				break;
			}
		}

		while ((r = fread(buf, 1, blksize, f)) > 0) {
			const char *source = buf;
			char *target = outbuf;

			if (ferror(f)) {
				fprintf(stderr, "ferror(%s) = true: %m\n", filename);
				break;
			}

			ucnv_convertEx(
					to_cnv,                 /* Target converter */
					from_cnv,               /* Source converter */
					&target,                /* Target */
					target + outblksize,    /* Target limit */
					&source,                /* Source */
					source + r,             /* Source limit */
					NULL, NULL, NULL, NULL,
					0, 1,
					&err);

			if (U_FAILURE(err)) {
				fprintf(stderr, "ucnv_convertEx failed: %s\n", u_errorName(err));
				break;
			}

			fwrite(outbuf, 1, target - outbuf, stdout);
		}

		if (ferror(f))
			fprintf(stderr, "ferror(%s) = true: %m\n", filename);


		if (f != stdin && fclose(f) == EOF)
			fprintf(stderr, "Failed to close file %s: %m\n", filename);
	}

#undef CALCULATE_OUTBLKSIZE

	free(outbuf);
	free(buf);
}
