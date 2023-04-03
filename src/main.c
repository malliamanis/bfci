#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "bfci.h"

static char *load_file(const char *name);
static void write_file(const char *name, const char *src);

static void print_help(void);

int main(int argc, char **argv)
{
	if (argc < 2) {
		print_help();
		return 1;
	}

	bool compile = false;

	for (uint32_t i = 1; i < argc; ++i) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
				case 'h':
					print_help();
					return 1;
					break;
				case 'c':
					compile = true;
					break;
				default:
					fprintf(stderr, "error: invalid option \'-%c\'", argv[i][1]);
					return 1;
					break;
			}
		}
		else {
			char *src = load_file(argv[i]);

			if (compile) {
				const char *file = argv[i];
				uint32_t file_len = strlen(file);

				char *compiled = bfci_compile(src);
				
				int32_t extension_index;
				for (extension_index = file_len; extension_index != -1 && file[extension_index] != '.'; --extension_index);

				char *out_name = NULL;

				if (extension_index == -1) {
					out_name = malloc(file_len + 2 + 1);
					strcpy(out_name, file);
					sprintf(out_name + file_len, ".c");
				}
				else {
					out_name = malloc(file_len + 1);
					strcpy(out_name, file);
					sprintf(out_name + extension_index, ".c");
				}

				write_file(out_name, compiled);
				free(compiled);
				free(out_name);
			}
			else
				bfci_interpret(src);	
			
			free(src);
		}
	}

	return 0;
}

static char *load_file(const char *name)
{
	FILE *file = fopen(name, "r");
	if (file == NULL) {
		fprintf(stderr, "error: %s: cannot open file\n", name);
		exit(EXIT_FAILURE);
	}

	fseek(file, 0, SEEK_END);
	size_t file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	char *buffer = malloc(file_size + 1);
	fread(buffer, sizeof(char), file_size, file);

	fclose(file);

	return buffer;
}

static void write_file(const char *name, const char *src)
{
	FILE *file = fopen(name, "w+");
	fwrite(src, sizeof(char), strlen(src), file);
	fclose(file);
}

static void print_help(void)
{
	printf("Usage: bfci [options] files...\n");
	printf("Options:\n");
	printf("  -h help\n");
	printf("  -c compile instead of interpreting\n");
}
