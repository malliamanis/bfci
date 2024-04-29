#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stddef.h>

char *file_read(const char *name);
void  file_write(const char *name, const char *src);

void append_str(char **dest, size_t *dest_size, const char *src);

struct stack {
	uint32_t *data;
	size_t size;

	uint32_t top;
};

struct stack stack_create(size_t initial_size);
void         stack_push(struct stack *s, int32_t value);
uint32_t     stack_pop(struct stack *s);
void         stack_destroy(struct stack *s);

#endif
