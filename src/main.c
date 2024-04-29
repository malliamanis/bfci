#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "bfci.h"
#include "util.h"

// takes a file [file_name] (e.g. hello.bf) and changes the extension to [extension] (e.g. .asm)
static char *file_name_gen(const char *file_name, const char *extension);

static void print_help(void);

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "error: no arguments, use -h for help\n");
		return 1;
	}

	struct bfci_options options = (struct bfci_options) { .type = BFCI_INTERPRET };

	int i;
	for (i = 1; i < argc; ++i) {
		const char *arg = argv[i];
		
		// if it doesn't start with a dash it's a file
		if (arg[0] != '-')
			break;

		if (strcmp(arg, "-h") == 0) {
			print_help();
			return 0;
		}
		else if (strcmp(arg, "-int") == 0) {
			options.type = BFCI_INTERPRET;
		}
		else if (strcmp(arg, "-c") == 0) {
			options.type = BFCI_COMPILE_C;
		}
		else if (strcmp(arg, "-asm") == 0) {
			options.type = BFCI_COMPILE_ASM;
		}
		else if (strcmp(arg, "-o") == 0) {
			options.optimise = true;
		}
		else if (strcmp(arg, "-d") == 0) {
			options.dynamic = true;
		}
		else {
			fprintf(stderr, "error: invalid flag '%s'\nUse -h for help\n", arg);
			return 1;
		}
	}

	if (i == argc) {
		fprintf(stderr, "error: no input files\n");
		return 1;
	}

	for (; i < argc; ++i) {
		char *src = file_read(argv[i]);

		char *compiled = bfci_ci(src, options);

		const char *extension = NULL;
		switch (options.type) {
			case BFCI_INTERPRET:   continue;
			case BFCI_COMPILE_C:   extension = ".c"; break;
			case BFCI_COMPILE_ASM: extension = ".asm"; break;
		}

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

static void print_help(void)
{
	printf(
		"Usage: bfci [options] files...\n"
		"Options:\n"
		"  -h\thelp\n"
		"  -int\tinterpret\n"
		"  -c\tcompile to C\n"
		"  -asm\tcompile to Linux x86_64 NASM Assembly\n"
		"  -o\tturn on optimisations\n"
		"  -d\tdynamically allocated cells\n"
	);
}
