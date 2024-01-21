#include <string.h>
#include <time.h>

#include "rle/rle.h"
#include "lz77/lz77.h"
#include "lz78/lz78.h"
#include "huffman/huffman.h"
#include "deflate/deflate.h"
#include "utils/mem.h"

#define COMPRESSION_RLE		1
#define COMPRESSION_LZ77	2
#define COMPRESSION_LZ78	3
#define COMPRESSION_HUFFMAN	4
#define COMPRESSION_DEFLATE	5

/**
 * @brief Read input file.
 * 
 * @param input_file 		input file
 * @param len 			file/buffer length
 * 
 * @return content of file
 */
static uint8_t *read_input_file(const char *input_file, uint32_t *len)
{
	uint8_t *buf = NULL;
	long size;
	FILE *fp;

	/* open input file */
	fp = fopen(input_file, "r");
	if (!fp) {
		fprintf(stderr, "Can't open input file \"%s\"\n", input_file);
		goto err;
	}

	/* get file size */
	fseek(fp, 0, SEEK_END);
	size = *len = ftell(fp);
	rewind(fp);

	/* check size */
	if (size > UINT32_MAX) {
		fprintf(stderr, "Input file \"%s\" is too big\n", input_file);
		goto err;
	}

	/* allocate buffer */
	buf = (uint8_t *) xmalloc(*len);

	/* read file */
	if (fread(buf, sizeof(uint8_t), *len, fp) != *len) {
		fprintf(stderr, "Can't read input file \"%s\"\n", input_file);
		goto err;
	}

	/* close input file */
	fclose(fp);

	return buf;
err:
	xfree(buf);
	if (fp)
		fclose(fp);
	return NULL;
}

/**
 * @brief Compression test.
 * 
 * @param src 				input buffer
 * @param src_len 			input buffer length
 * @param compression_algorithm 	compression algorithm
 * @param compression_name 		compression name
 */
static void compression_test(uint8_t *src, uint32_t src_len, int compression_algorithm, const char *compression_name)
{
	double zip_time, unzip_time;
	uint32_t zip_len, unzip_len;
	uint8_t *zip, *unzip;
	clock_t start, end;

	/* print start message */
	printf("********************** %s **********************\n", compression_name);

	/* compress */
	start = clock();
	switch (compression_algorithm) {
		case COMPRESSION_RLE:
			zip = rle_compress(src, src_len, &zip_len);
			break;
		case COMPRESSION_LZ77:
			zip = lz77_compress(src, src_len, &zip_len);
			break;
		case COMPRESSION_LZ78:
			zip = lz78_compress(src, src_len, &zip_len);
			break;
		case COMPRESSION_HUFFMAN:
			zip = huffman_compress(src, src_len, &zip_len);
			break;
		case COMPRESSION_DEFLATE:
			zip = deflate_compress(src, src_len, &zip_len);
			break;
		default:
			fprintf(stderr, "Unknown compression algorithm\n");
			return;
	}
	end = clock();
	zip_time = (double) (end - start) / CLOCKS_PER_SEC;

	/* uncompress */
	start = clock();
	switch (compression_algorithm) {
		case COMPRESSION_RLE:
			unzip = rle_uncompress(zip, zip_len, &unzip_len);
			break;
		case COMPRESSION_LZ77:
			unzip = lz77_uncompress(zip, zip_len, &unzip_len);
			break;
		case COMPRESSION_LZ78:
			unzip = lz78_uncompress(zip, zip_len, &unzip_len);
			break;
		case COMPRESSION_HUFFMAN:
			unzip = huffman_uncompress(zip, zip_len, &unzip_len);
			break;
		case COMPRESSION_DEFLATE:
			unzip = deflate_uncompress(zip, zip_len, &unzip_len);
			break;
		default:
			fprintf(stderr, "Unknown compression algorithm\n");
			return;
	}
	end = clock();
	unzip_time = (double) (end - start) / CLOCKS_PER_SEC;

	/* print statistics */
	printf("Compresstion status : %s\n", src_len == unzip_len && memcmp(src, unzip, unzip_len) == 0 ? "OK" : "ERROR");
	printf("Compression time : %f sec\n", zip_time);
	printf("Uncompression time : %f sec\n", unzip_time);
	printf("Compression ratio : %f\n", (double) src_len / (double) zip_len);

	/* free memory */
	xfree(zip);
	xfree(unzip);
}

int main(int argc, char **argv)
{
	uint32_t src_len;
	uint8_t *src;

	/* check arguments */
	if (argc != 2) {
		fprintf(stderr, "%s input_file\n", argv[0]);
		return 1;
	}

	/* read input file */
	src = read_input_file(argv[1], &src_len);
	if (!src)
		return 1;

	/* compression test */
	//compression_test(src, src_len, COMPRESSION_RLE, "RLE");
	//compression_test(src, src_len, COMPRESSION_LZ77, "LZ77");
	//compression_test(src, src_len, COMPRESSION_LZ78, "LZ78");
	compression_test(src, src_len, COMPRESSION_HUFFMAN, "HUFFMAN");
	//compression_test(src, src_len, COMPRESSION_DEFLATE, "DEFLATE");

	return 0;
}