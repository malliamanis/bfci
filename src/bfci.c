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

enum token_type {
	TOKEN_LEFT,
	TOKEN_RIGHT,
	TOKEN_INC,
	TOKEN_DEC,
	TOKEN_IN,
	TOKEN_OUT,
	TOKEN_JMP_FWD,
	TOKEN_JMP_BACK,
	TOKEN_EOF
};

struct token {
	enum token_type type;
	uint32_t jump_matching_index;
};

static struct token *tokenise(const char *src);

char *bfci_compile_c(char *src, bool dynamic)
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
	struct token current;
	struct token *tokens = tokenise(src);
	free(src);

	while ((current = tokens[index++]).type != TOKEN_EOF) {
		append_str(&output, &output_size, indentation);

		switch (current.type) {
			case TOKEN_LEFT:
				append_str(&output, &output_size, "\t--ptr;\n");
				break;
			case TOKEN_RIGHT:
				if (dynamic)
					append_str(&output, &output_size, "\tdouble_size(&cells, &ptr, &size);\n");

				append_str(&output, &output_size, "\t++ptr;\n");
				break;
			case TOKEN_INC:
				append_str(&output, &output_size, "\t++(*ptr);\n");
				break;
			case TOKEN_DEC:
				append_str(&output, &output_size, "\t--(*ptr);\n");
				break;
			case TOKEN_OUT:
				append_str(&output, &output_size, "\tputchar(*ptr);\n");
				break;
			case TOKEN_IN:
				append_str(&output, &output_size, "\t*ptr = getchar();\n");
				break;
			case TOKEN_JMP_FWD:
				append_str(&output, &output_size, "\twhile (*ptr) {\n");

				indentation[indentation_last++] = '\t';
				continue;
			case TOKEN_JMP_BACK:
				output[strlen(output) - 1] = 0; // the closing '}' of a loop is indented one tab less than the body 
				indentation[--indentation_last] = 0;

				append_str(&output, &output_size, "\t}\n");
				break;
			default:
				break;
		}
	}

	free (tokens);
	
	append_str(
		&output,
		&output_size,
		"\n\tfree(cells);\n"
		"}\n"
	);

	return output;
}

char *bfci_compile_asm(char *src)
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
		"cells: resq 1\n"
		"pointer: resq 1\n"
		"size: resq 1\n"
		""
		"section .text\n"
		"brk:\n"
		" mov rax, 12\n"
		" syscall\n"
		" ret\n"
	);

	append_str(
		&output,
		&output_size,
		"O:\n" // output a byte
		" mov rax, 1\n"
		" mov rdi, 1\n"
		" mov rsi, qword [pointer]\n"
		" mov rdx, 1\n"
		" syscall\n"
		" ret\n"
		""
		"G:\n" // get a byte from the user
		" xor rax, rax\n"
		" xor rdi, rdi\n"
		" mov rsi, qword [pointer]\n"
		" mov rdx, 1\n"
		" syscall\n"
		" ret\n"
		""
		"_start:\n"
		" push rbp\n"
		" mov rbp, rsp\n"
		""
		" xor rdi, rdi\n"
		" call brk\n"
		" mov qword [cells], rax\n"
		" mov qword [pointer], rax\n"
		" mov rdi, rax\n"
	);

	append_str(
		&output,
		&output_size,
		" mov qword [size], " CELLS_FIXED_SIZE_STR "\n"
		" mov rax, qword [size]\n"
		" add rdi, rax\n"
		" call brk\n"
	);

	uint32_t index = 0;
	struct token current;
	struct token *tokens = tokenise(src);
	free(src);

	while ((current = tokens[index++]).type != TOKEN_EOF) {
		switch (current.type) {
			case TOKEN_LEFT:
				append_str(&output, &output_size, " dec qword [pointer]\n");
				break;
			case TOKEN_RIGHT:
				append_str(&output, &output_size, " inc qword [pointer]\n");
				break;
			case TOKEN_INC:
				append_str(
					&output,
					&output_size,
					" mov rax, qword [pointer]\n"
					" inc byte [rax]\n"
				);
				break;
			case TOKEN_DEC:
				append_str(
					&output,
					&output_size,
					" mov rax, qword [pointer]\n"
					" dec byte [rax]\n"
				);
				break;
			case TOKEN_OUT:
				append_str(&output, &output_size, " call O\n");
				break;
			case TOKEN_IN:
				append_str(&output, &output_size, " call G\n");
				break;
			case TOKEN_JMP_FWD:
				stack_push(&loop_stack, loop_counter);

				sprintf(startstr, "L%u:\n", loop_counter);
				sprintf(endstr, "E%u\n", loop_counter);

				++loop_counter;

				append_str(&output, &output_size, startstr);
				append_str(
					&output, &output_size,
					" mov rax, qword [pointer]\n"
					" cmp byte [rax], 0\n"
					" je "
				);
				append_str(&output, &output_size, endstr);

				break;
			case TOKEN_JMP_BACK:
				temp = stack_pop(&loop_stack);

				sprintf(endstr, "E%u:\n", temp);
				sprintf(startstr, "L%u\n", temp);

				append_str(&output, &output_size, endstr);
				append_str(
					&output,
					&output_size,
					" mov rax, qword [pointer]\n"
					" cmp byte [rax], 0\n"
					" jne "
				);
				append_str(&output, &output_size, startstr);

				break;
			default:
				continue;
		}
	}

	free(tokens);
	
	append_str(
		&output,
		&output_size,
		" mov rax, qword [cells]\n"
		" call brk\n"
		""
		" pop rbp\n"
		""
		" mov rax, 60\n"
		" xor rdi, rdi\n"
		" syscall\n"
	);

	stack_destroy(&loop_stack);

	return output;
}

void bfci_interpret(char *src, bool dynamic)
{
	uint8_t *cells;
	size_t cells_size;

	if (dynamic)
		cells_size = CELLS_INITIAL_DYNAMIC_SIZE;
	else
		cells_size = CELLS_FIXED_SIZE;
	cells = calloc(cells_size, sizeof *cells);

	uint8_t *ptr = cells;

	struct token current;
	struct token *tokens = tokenise(src);
	free(src);

	uint32_t pg_counter = 0;
	while ((current = tokens[pg_counter]).type != TOKEN_EOF) {
		switch (current.type) {
			case TOKEN_LEFT:
				if (ptr == cells) {
					fprintf(stderr, "error: data pointer below 0\n");
					exit(EXIT_FAILURE);
				}

				--ptr;
				break;
			case TOKEN_RIGHT:
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
			case TOKEN_INC:
				++(*ptr);
				break;
			case TOKEN_DEC:
				--(*ptr);
				break;
			case TOKEN_OUT:
				putchar(*ptr);
				break;
			case TOKEN_IN:
				*ptr = getchar();
				break;
			case TOKEN_JMP_FWD:
				if (*ptr == 0)
					pg_counter = current.jump_matching_index;

				break;
			case TOKEN_JMP_BACK:
				if (*ptr != 0)
					pg_counter = current.jump_matching_index;
				break;
			default:
				break;
		}
		
		++pg_counter;
	}

	free(tokens);
	free(cells);	
}

static struct token *tokenise(const char *src)
{
	size_t tokens_size = 32;
	uint32_t tokens_index = 0;
	struct token *tokens = malloc(tokens_size * (sizeof *tokens));

	uint32_t pop;
	struct stack jump_stack = stack_create(32);

	char current;
	for (uint32_t i = 0; (current = src[i]) != 0; ++i) {
		switch (current) {
			case '<':
				tokens[tokens_index].type = TOKEN_LEFT;
				break;
			case '>':
				tokens[tokens_index].type = TOKEN_RIGHT;
				break;
			case '+':
				tokens[tokens_index].type = TOKEN_INC;
				break;
			case '-':
				tokens[tokens_index].type = TOKEN_DEC;
				break;
			case '.':
				tokens[tokens_index].type = TOKEN_OUT;
				break;
			case ',':
				tokens[tokens_index].type = TOKEN_IN;
				break;
			case '[':
				stack_push(&jump_stack, tokens_index);
				tokens[tokens_index].type = TOKEN_JMP_FWD;
				break;
			case ']':
				pop = stack_pop(&jump_stack);

				// I think it's self explanatory
				tokens[tokens_index].type = TOKEN_JMP_BACK;
				tokens[tokens_index].jump_matching_index = pop;
				tokens[pop].jump_matching_index = tokens_index;
				break;
			default:
				// if current doesn't match a token then don't try to double the size of tokens
				// also don't increment the tokens_index
				continue;
		}

		++tokens_index;

		if (tokens_index == tokens_size)
			tokens = realloc(tokens, (tokens_size *= 2) * (sizeof *tokens));
	}

	stack_destroy(&jump_stack);

	tokens[tokens_index++].type = TOKEN_EOF;
	tokens = realloc(tokens, tokens_index * (sizeof *tokens));

	return tokens;
}
