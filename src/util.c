#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

char *file_read(const char *name)
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

void file_write(const char *name, const char *src)
{
	FILE *file = fopen(name, "w+");
	if (file == NULL) {
		fprintf(stderr, "error: %s: cannot write to file\n", name);
		exit(EXIT_FAILURE);
	}

	fwrite(src, sizeof(char), strlen(src), file);

	fclose(file);
}

void append_str(char **dest, size_t *dest_size, const char *src)
{
	if (strlen(*dest) + 1 + strlen(src) >= *dest_size) {
		size_t src_length = strlen(src);
		*dest = realloc(*dest, *dest_size * 2 + src_length + 1);
		*dest_size = *dest_size * 2 + src_length + 1;
	}

	strcat(*dest, src);
}

struct stack stack_create(size_t initial_size)
{
	struct stack s = {
		.data = malloc(initial_size * sizeof(*s.data)),
		.size = initial_size
	};

	return s;
}

void stack_push(struct stack *s, int32_t value)
{
	if (s->top == s->size)
		s->data = realloc(s->data, (s->size *= 2) * (sizeof *s->data));

	s->data[s->top++] = value;
}

uint32_t stack_pop(struct stack *s)
{
	if (s->top == 0) {
		fprintf(stderr, "internal error: cannot pop from stack with no data\n");
		exit(EXIT_FAILURE);
	}

	int32_t value = s->data[--s->top];
	s->data[s->top] = 0;

	return value;
}

void stack_destroy(struct stack *s)
{
	free(s->data);
	*s = (struct stack){ 0 };
}
