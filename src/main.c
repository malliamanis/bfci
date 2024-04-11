#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "bfci.h"

static char *file_read(const char *name);
static void file_write(const char *name, const char *src);

static char *file_name_gen(const char *file_name, const char *extension);

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
	bool dynamic = false;

	int i;
	for (i = 1; i < argc; ++i) {
		const char *arg = argv[i];
		
		// if it doesn't start with a dash it's a file
		if (arg[0] != '-')
			break;

		if (strcmp(arg + 1, "int") == 0) {
			interpret = true;
			continue;
		}
		else if (strcmp(arg + 1, "asm") == 0) {
			compile_asm = true;
			continue;
		}

		switch (arg[1]) {
			case 'h':
				print_help();
				return 0;
			case 'c':
				compile_c = true;
				continue;
			case 'd':
				dynamic = true;
				continue;
			default:
				break;
		}

		// if the argument didn't match any flags
		fprintf(stderr, "error: invalid flag '%s'\nUse -h for help\n", arg);
		return 1;
	}

	if (interpret && (compile_c || compile_asm)) {
		fprintf(stderr, "error: can't have both the '-int' flag and a compilation flag at once\n");
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

	if (!compile_c && !compile_asm)
		interpret = true;

	for (; i < argc; ++i) {
		char *src = file_read(argv[i]);

		if (interpret) {
			bfci_interpret(src, dynamic);	
			continue;
		}

		char *compiled = NULL;
		const char *extension = NULL;

		if (compile_c) {
			compiled = bfci_compile_c(src, dynamic);
			extension = ".c";
		}
		else if (compile_asm) {
			compiled = bfci_compile_asm(src, dynamic);
			extension = ".asm";
		}
		free(src);

		char *out_name = file_name_gen(argv[i], extension);

		file_write(out_name, compiled);

		free(out_name);
		free(compiled);
	}

	return 0;
}

static char *file_name_gen(const char *file_name, const char *extension)
{
	if (!file_name || !extension)
		return NULL;

	uint32_t file_name_len = strlen(file_name);

	int32_t extension_index;
	for (extension_index = file_name_len - 1; extension_index != -1 && file_name[extension_index] != '.'; --extension_index);

	int32_t path_index;
	for (path_index = file_name_len - 1; path_index != -1 && file_name[path_index] != '/'; --path_index);
	++path_index;

	uint32_t out_name_offset; // for the sprintf offset

	if (extension_index == -1)
		out_name_offset = file_name_len - 1;
	else
		out_name_offset = extension_index - path_index;

	char *out_name = malloc(file_name_len - path_index + strlen(extension) + 1);
	strcpy(out_name, file_name + path_index);
	sprintf(out_name + out_name_offset, "%s", extension);

	return out_name;
}

static char *file_read(const char *name)
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

static void file_write(const char *name, const char *src)
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
	printf(
		"Usage: bfci [options] files...\n"
		"Options:\n"
		"  -h\thelp\n"
		"  -int\tinterpret\n"
		"  -c\tcompile to C\n"
		"  -asm\tcompile to Linux x86_64 NASM Assembly\n"
		"  -d\tdynamically allocated cells\n"
	);
}
