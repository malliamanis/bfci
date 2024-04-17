#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "bfci.h"
#include "util.h"

#define CELLS_FIXED_SIZE 30000
#define CELLS_FIXED_SIZE_STR "30000" 

#define CELLS_INITIAL_DYNAMIC_SIZE 128
#define CELLS_INITIAL_DYNAMIC_SIZE_STR "128"

enum operation_type {
	OPERATION_LEFT,
	OPERATION_RIGHT,
	OPERATION_INC,
	OPERATION_DEC,
	OPERATION_IN,
	OPERATION_OUT,
	OPERATION_JMP_FWD,
	OPERATION_JMP_BACK,
	OPERATION_EOF
};

struct operation {
	enum operation_type operator;
	uint32_t operand;
};

// TODO: add optimisation
static struct operation *compile_generic(const char *src, bool optimise);

static void  interpret(const struct operation *ops, bool dynamic);
static char *compile_c(const struct operation *ops, bool dynamic);
static char *compile_asm(const struct operation *ops);

char *bfci_ci(char *src, enum bfci_option option, bool optimise, bool dynamic)
{
	struct operation *ops = compile_generic(src, optimise);
	free(src);

	if (option == BFCI_INTERPRET) {
		interpret(ops, dynamic);
		return NULL;
	}

	char *output = NULL;
	if (option == BFCI_COMPILE_C)
		output = compile_c(ops, dynamic);
	else if (option == BFCI_COMPILE_ASM) {
		if (dynamic) {
			fprintf(stderr, "error: dynamic memory isn't supported for assembly\n");
			exit(EXIT_FAILURE);
		}

		output = compile_asm(ops);
	}

	free(ops);

	return output;
}

static char *compile_c(const struct operation *ops, bool dynamic)
{
	size_t output_size = 64;
	char *output = calloc(output_size, sizeof(char));

	char indentation[100] = {0};
	uint32_t indentation_last = 0;

	append_str(
		&output,
		&output_size,
		"#include <stdio.h>\n"
		"#include <stdlib.h>\n"
		"#include <stdint.h>\n"
	);

	if (dynamic) {
		append_str(
			&output,
			&output_size,
			"#include <string.h>\n"
			"void double_size(uint8_t **cells, uint8_t **ptr, size_t *size)\n"
			"{\n"
			"\tif (*ptr - *cells == *size - 1) {\n"
			"\t\t*ptr -= (uintptr_t)(*cells);\n"
			"\t\t*cells = realloc(*cells, *size * 2);\n"
			"\t\tmemset(*cells + *size, 0, *size);\n"
			"\t\t*size *= 2;\n"
			"\t\t*ptr += (uintptr_t)(*cells);\n"
			"\t}\n"
			"\t++ptr;\n"
			"}\n"
			"\n"
			"int main(void)\n"
			"{\n"
			"\tsize_t size = " CELLS_INITIAL_DYNAMIC_SIZE_STR ";\n"
			"\tuint8_t *cells = calloc(size, sizeof(uint8_t));\n"
			"\tuint8_t *ptr = cells;\n"
		);
	}
	else {
		append_str(
			&output,
			&output_size,
			"int main(void)\n"
			"{\n"
			"\tsize_t size = " CELLS_FIXED_SIZE_STR ";\n"
			"\tuint8_t *cells = calloc(size, sizeof(uint8_t));\n"
			"\tuint8_t *ptr = cells;\n"
		);
	}

	uint32_t index = 0;
	struct operation current;

	while ((current = ops[index++]).operator != OPERATION_EOF) {
		append_str(&output, &output_size, indentation);

		switch (current.operator) {
			case OPERATION_LEFT:
				append_str(&output, &output_size, "\t--ptr;\n");
				break;
			case OPERATION_RIGHT:
				if (dynamic)
					append_str(&output, &output_size, "\tdouble_size(&cells, &ptr, &size);\n");

				append_str(&output, &output_size, "\t++ptr;\n");
				break;
			case OPERATION_INC:
				append_str(&output, &output_size, "\t++(*ptr);\n");
				break;
			case OPERATION_DEC:
				append_str(&output, &output_size, "\t--(*ptr);\n");
				break;
			case OPERATION_OUT:
				append_str(&output, &output_size, "\tputchar(*ptr);\n");
				break;
			case OPERATION_IN:
				append_str(&output, &output_size, "\t*ptr = getchar();\n");
				break;
			case OPERATION_JMP_FWD:
				append_str(&output, &output_size, "\twhile (*ptr) {\n");

				indentation[indentation_last++] = '\t';
				continue;
			case OPERATION_JMP_BACK:
				output[strlen(output) - 1] = 0; // the closing '}' of a loop is indented one tab less than the body 
				indentation[--indentation_last] = 0;

				append_str(&output, &output_size, "\t}\n");
				break;
			default:
				break;
		}
	}

	append_str(
		&output,
		&output_size,
		"\n\tfree(cells);\n"
		"}\n"
	);

	return output;
}

static char *compile_asm(const struct operation *ops)
{
	size_t output_size = 512;
	char *output = calloc(output_size, sizeof(char));

	uint32_t temp;
	uint32_t loop_counter = 0;

	struct stack loop_stack = stack_create(16);

	char startstr[16] = {0};
	char endstr[16] = {0};

	append_str(
		&output,
		&output_size,
		"default rel\n"
		"global _start\n"
		""
		"section .bss\n"
		"cells: resq " CELLS_FIXED_SIZE_STR "\n"
		""
		"section .text\n"
	);

	append_str(
		&output,
		&output_size,
		"O:\n" // output a byte
		" mov rax, 1\n"
		" mov rdi, 1\n"
		" mov rsi, rbx\n"
		" mov rdx, 1\n"
		" syscall\n"
		" ret\n"
		""
		"G:\n" // get a byte from the user
		" xor rax, rax\n"
		" xor rdi, rdi\n"
		" mov rsi, rbx\n"
		" mov rdx, 1\n"
		" syscall\n"
		" ret\n"
		""
		"_start:\n"
		" push rbp\n"
		" mov rbp, rsp\n"
		" lea rbx, [cells]\n" // rbx is the data pointer
	);

	uint32_t index = 0;
	struct operation current;

	while ((current = ops[index++]).operator != OPERATION_EOF) {
		switch (current.operator) {
			case OPERATION_LEFT:
				append_str(&output, &output_size, " dec rbx\n");
				break;
			case OPERATION_RIGHT:
				append_str(&output, &output_size, " inc rbx\n");
				break;
			case OPERATION_INC:
				append_str(&output, &output_size, " inc byte [rbx]\n");
				break;
			case OPERATION_DEC:
				append_str(&output, &output_size, " dec byte [rbx]\n");
				break;
			case OPERATION_OUT:
				append_str(&output, &output_size, " call O\n");
				break;
			case OPERATION_IN:
				append_str(&output, &output_size, " call G\n");
				break;
			case OPERATION_JMP_FWD:
				stack_push(&loop_stack, loop_counter);

				sprintf(startstr, "L%u:\n", loop_counter);
				sprintf(endstr, "E%u\n", loop_counter);

				++loop_counter;

				append_str(&output, &output_size, startstr);
				append_str(
					&output, &output_size,
					" cmp byte [rbx], 0\n"
					" je "
				);
				append_str(&output, &output_size, endstr);

				break;
			case OPERATION_JMP_BACK:
				temp = stack_pop(&loop_stack);

				sprintf(endstr, "E%u:\n", temp);
				sprintf(startstr, "L%u\n", temp);

				append_str(&output, &output_size, endstr);
				append_str(
					&output,
					&output_size,
					" cmp byte [rbx], 0\n"
					" jne "
				);
				append_str(&output, &output_size, startstr);

				break;
			default:
				continue;
		}
	}

	append_str(
		&output,
		&output_size,
		" pop rbp\n"
		""
		" mov rax, 60\n"
		" xor rdi, rdi\n"
		" syscall\n"
	);

	stack_destroy(&loop_stack);

	return output;
}

static void interpret(const struct operation *ops, bool dynamic)
{
	uint8_t *cells;
	size_t cells_size;

	if (dynamic)
		cells_size = CELLS_INITIAL_DYNAMIC_SIZE;
	else
		cells_size = CELLS_FIXED_SIZE;
	cells = calloc(cells_size, sizeof *cells);

	uint8_t *ptr = cells;

	uint32_t pg_counter = 0;
	struct operation current;

	while ((current = ops[pg_counter]).operator != OPERATION_EOF) {
		switch (current.operator) {
			case OPERATION_LEFT:
				if (ptr == cells) {
					fprintf(stderr, "error: data pointer below 0\n");
					exit(EXIT_FAILURE);
				}

				--ptr;
				break;
			case OPERATION_RIGHT:
				if ((uintptr_t)(ptr - cells) == cells_size - 1) {
					if (dynamic) {
						ptr -= (uintptr_t)cells;

						cells = realloc(cells, cells_size * 2);
						memset(cells + cells_size, 0, cells_size);
						cells_size *= 2;

						ptr += (uintptr_t)cells;
					}
					else {
						fprintf(stderr, "error: data pointer exceeds %u, use '-d' flag for dynamic memory\n", CELLS_FIXED_SIZE);
						exit(EXIT_FAILURE);
					}
				}

				++ptr;
				break;
			case OPERATION_INC:
				++(*ptr);
				break;
			case OPERATION_DEC:
				--(*ptr);
				break;
			case OPERATION_OUT:
				putchar(*ptr);
				break;
			case OPERATION_IN:
				*ptr = getchar();
				break;
			case OPERATION_JMP_FWD:
				if (*ptr == 0)
					pg_counter = current.operand;

				break;
			case OPERATION_JMP_BACK:
				if (*ptr != 0)
					pg_counter = current.operand;
				break;
			default:
				break;
		}
		
		++pg_counter;
	}

	free(cells);	
}

static struct operation *compile_generic(const char *src, bool optimise)
{
	size_t ops_size = 32;
	uint32_t inst_index = 0;
	struct operation *ops = malloc(ops_size * (sizeof *ops));

	uint32_t pop;
	struct stack jump_stack = stack_create(32);

	char current;
	for (uint32_t i = 0; (current = src[i]) != 0; ++i) {
		switch (current) {
			case '<':
				ops[inst_index].operator = OPERATION_LEFT;
				break;
			case '>':
				ops[inst_index].operator = OPERATION_RIGHT;
				break;
			case '+':
				ops[inst_index].operator = OPERATION_INC;
				break;
			case '-':
				ops[inst_index].operator = OPERATION_DEC;
				break;
			case '.':
				ops[inst_index].operator = OPERATION_OUT;
				break;
			case ',':
				ops[inst_index].operator = OPERATION_IN;
				break;
			case '[':
				stack_push(&jump_stack, inst_index);
				ops[inst_index].operator = OPERATION_JMP_FWD;
				break;
			case ']':
				pop = stack_pop(&jump_stack);

				// I think it's self explanatory
				ops[inst_index].operator = OPERATION_JMP_BACK;
				ops[inst_index].operand = pop;
				ops[pop].operand = inst_index;
				break;
			default:
				// if current doesn't match a operation then don't try to double the size of ops
				// also don't increment the inst_index
				continue;
		}

		++inst_index;

		if (inst_index == ops_size)
			ops = realloc(ops, (ops_size *= 2) * (sizeof *ops));
	}

	stack_destroy(&jump_stack);

	ops[inst_index++].operator = OPERATION_EOF;
	ops = realloc(ops, inst_index * (sizeof *ops));

	return ops;
}
