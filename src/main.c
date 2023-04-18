#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "bfci.h"

static char *read_file(const char *name);
static void write_file(const char *name, const char *src);

static void print_help(void);

int main(int argc, char **argv)
{
	if (argc < 2) {
		print_help();
		return 1;
	}

	bool interpret = false;
	bool compile_c = false;
	bool compile_asm = false;

	uint32_t i;
	for (i = 1; i < argc; ++i) {
		const char *arg = argv[i];
		if (arg[0] == '-') {
			if (arg[1] == 'h') {
				print_help();
				return 1;
			}
			else if (arg[1] == 'i') {
				if (interpret) {
					fprintf(stderr, "error: can't have the '-i' flag more than once\n");
					return 1;
				}
				interpret = true;
			}
			else if (arg[1] == 'c') {
				if (compile_c) {
					fprintf(stderr, "error: can't have the '-c' flag more than once\n");
					return 1;
				}
				compile_c = true;
			}
			else if (arg[1] == 'a' && arg[2] == 's' && arg[3] == 'm') {
				if (compile_asm) {
					fprintf(stderr, "error: can't have the '-asm' flag more than once\n");
					return 1;
				}
				compile_asm = true;
			}
			else {
				fprintf(stderr, "error: invalid flag '-%c'\nUse -h for help\n", arg[0]);
				return 1;
			}
		}
		else
			break;
	}

	if ((interpret && compile_c) || (interpret && compile_asm)) {
		fprintf(stderr, "error: can't have both the '-i' flag and a compilation flag at once\n");
		return 1;
	}
	else if (compile_c && compile_asm) {
		fprintf(stderr, "error: can't have both the '-c' and '-asm' options at once\n");
		return 1;
	}
	else if (i == argc) {
		fprintf(stderr, "error: no input files\n");
		return 1;
	}

	for (; i < argc; ++i) {
		char *src = read_file(argv[i]);

		if (!(compile_c || compile_asm)) {
			bfci_interpret(src);	
			continue;
		}

		const char *file_name = argv[i];
		uint32_t file_name_len = strlen(file_name);

		int32_t extension_index; for (extension_index = file_name_len - 1; extension_index != -1 && file_name[extension_index] != '.'; --extension_index);
		int32_t path_index;
		for (path_index = file_name_len - 1; path_index != -1 && file_name[path_index] != '/'; --path_index);
		++path_index;

		char *compiled = NULL;
		uint32_t out_name_offset; // for the sprintf offset
		const char *extension_str = NULL;

		if (compile_c) {
			compiled = bfci_compile_c(src);
			extension_str = ".c";
		}
		else if (compile_asm) {
			compiled = bfci_compile_asm(src);
			extension_str = ".asm";
		}

		if (extension_index == -1)
			out_name_offset = file_name_len - 1;
		else
			out_name_offset = extension_index - path_index;

		char *out_name = malloc(file_name_len - path_index + strlen(extension_str) + 1);
		strcpy(out_name, file_name + path_index);
		sprintf(out_name + out_name_offset, "%s", extension_str);

		write_file(out_name, compiled);
		free(out_name);
		free(compiled);
		
		free(src);
	}

	return 0;
}

static char *read_file(const char *name)
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
	if (file == NULL) {
		fprintf(stderr, "error: %s: cannot write to file\n", name);
	}

	fwrite(src, sizeof(char), strlen(src), file);

	fclose(file);
}

static void print_help(void)
{
	printf("Usage: bfci [options] files...\n"
	       "Options:\n"
	       "  -h   help\n"
	       "  -i   interpret\n"
	       "  -c   compile to C\n"
		   "  -asm compile to Linux x86_64 Assembly\n");
}
