#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "bfci.h"

static void add_instruction(char **dest, size_t *dest_size, const char *src);

char *bfci_compile_c(const char *src)
{
	char current;
	uint32_t index = 0;

	size_t output_size = 64;
	char *output = calloc(output_size, sizeof(char));

	char indentation[50] = {0};
	uint32_t indentation_last = 0;

	add_instruction(&output, &output_size, "#include <stdio.h>\n"
	                                       "#include <stdlib.h>\n"
	                                       "#include <stdint.h>\n"
	                                       "#include <string.h>\n"
	                                       "\n"
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
	                                       "\tsize_t size = 128;\n"
	                                       "\tuint8_t *cells = calloc(size, sizeof(uint8_t));\n"
	                                       "\tuint8_t *ptr = cells;\n");

	while ((current = src[index++]) != 0) {
		switch (current) {
			case '+':
				add_instruction(&output, &output_size, indentation);
				add_instruction(&output, &output_size, "\t++(*ptr);\n");
				break;
			case '-':
				add_instruction(&output, &output_size, indentation);
				add_instruction(&output, &output_size, "\t--(*ptr);\n");
				break;
			case '<':
				add_instruction(&output, &output_size, indentation);
				add_instruction(&output, &output_size, "\t--ptr;\n");
				break;
			case '>':
				add_instruction(&output, &output_size, indentation);
				add_instruction(&output, &output_size, "\tdouble_size(&cells, &ptr, &size);\n");

				add_instruction(&output, &output_size, indentation);
				add_instruction(&output, &output_size, "\t++ptr;\n");
				break;
			case '.':
				add_instruction(&output, &output_size, indentation);
				add_instruction(&output, &output_size, "\tputchar(*ptr);\n");
				break;
			case ',':
				add_instruction(&output, &output_size, indentation);
				add_instruction(&output, &output_size, "\t*ptr = getchar();\n");
				break;
			case '[':
				add_instruction(&output, &output_size, indentation);
				add_instruction(&output, &output_size, "\twhile (*ptr) {\n");

				indentation[indentation_last++] = '\t';
				break;
			case ']':
				indentation[--indentation_last] = 0;

				add_instruction(&output, &output_size, indentation);
				add_instruction(&output, &output_size, "\t}\n");
				break;
			default:
				break;
		}
	}
	
	add_instruction(&output, &output_size, "\n\tfree(cells);\n"
	                                       "}\n");

	return output;
}

char *bfci_compile_asm(const char *src)
{
	char current;
	uint32_t index = 0;

	size_t output_size = 512;
	char *output = calloc(output_size, sizeof(char));

	uint32_t loop_counter = 1;

	size_t loop_stack_size = 16;
	uint32_t *loop_stack = calloc(loop_stack_size, sizeof(uint32_t));

	int32_t stack_top = 0;

	char startstr[16] = {0};
	char endstr[16] = {0};

	add_instruction(&output, &output_size, "default rel\n"
	                                       "global _start\n"
	                                       ""
	                                       "section .bss\n"
	                                       "cells: resq 1\n"
	                                       "pointer: resq 1\n"
	                                       "size: resq 1\n"
	                                       ""
	                                       "section .text\n"
	                                       "brk:\n"
	                                       " mov eax, 12\n"
	                                       " syscall\n"
	                                       " ret\n"
	                                       ""
	                                       "D:\n" // double memory size if needed
	                                       " mov rax, [pointer]\n"
	                                       " mov rcx, [cells]\n"
	                                       " sub rax, rcx\n"
	                                       " mov rbx, [size]\n"
	                                       " dec rbx\n"
	                                       " cmp rax, rbx\n"
	                                       ""
	                                       " jne ENDIF\n"
	                                       " lea rcx, [size]\n"
	                                       " mov rdi, [ecx]\n"
	                                       " shl rdi, 2\n"
	                                       " call brk\n"
	                                       ""
	                                       " mov rdx, [cells]\n"
	                                       " add rdx, qword [rcx]\n"
	                                       " xor eax, eax\n"
	                                       " mov rdi, [rcx]\n"
	                                       " dec rdi\n"
	                                       " rep stosb\n"
	                                       ""
	                                       " shl qword [rcx], 2\n"
	                                       " ENDIF:\n"
	                                       " ret\n"
	                                       ""
	                                       "O:\n" // output a byte
	                                       " mov eax, 1\n"
	                                       " mov edi, 1\n"
	                                       " mov rsi, [pointer]\n"
	                                       " mov edx, 1\n"
	                                       " syscall\n"
	                                       " ret\n"
	                                       ""
	                                       "G:\n" // get a byte from the user
	                                       " xor eax, eax\n"
	                                       " xor edi, edi\n"
	                                       " mov rsi, [pointer]\n"
	                                       " mov edx, 1\n"
	                                       " syscall\n"
	                                       " ret\n"
	                                       ""
	                                       "_start:\n"
	                                       " push rbp\n"
	                                       " mov rbp, rsp\n"
	                                       ""
	                                       " xor edi, edi\n"
	                                       " call brk\n"
	                                       " mov [cells], rax\n"
	                                       " mov [pointer], rax\n"
	                                       " mov rdi, rax\n"
	                                       " mov [size], dword 0x10000\n"
	                                       " mov eax, [size]\n"
	                                       " add rdi, rax\n"
	                                       " call brk\n");

	// TODO: fix memory doubling bug
	while ((current = src[index++]) != 0) {
		switch (current) {
			case '+':
				add_instruction(&output, &output_size, " mov rax, [pointer]\n"
				                                       " inc byte [rax]\n");
				break;
			case '-':
				add_instruction(&output, &output_size, " mov rax, [pointer]\n"
				                                       " dec byte [rax]\n");
				break;
			case '<':
				add_instruction(&output, &output_size, " dec qword [pointer]\n");
				break;
			case '>':
				add_instruction(&output, &output_size, " inc qword [pointer]\n");
				// add_instruction(&output, &output_size, " call D\n");
				break;
			case '.':
				add_instruction(&output, &output_size, " call O\n");
				break;
			case ',':
				add_instruction(&output, &output_size, " call G\n");
				break;
			case '[':
				if (stack_top == loop_stack_size) {
					loop_stack = realloc(loop_stack, loop_stack_size * 2);
					memset(loop_stack + loop_stack_size, 0, loop_stack_size);
					loop_stack_size *= 2;
				}

				loop_stack[stack_top++] = loop_counter; // push

				sprintf(startstr, "L%u:\n", loop_counter);
				sprintf(endstr, "E%u\n", loop_counter);

				++loop_counter;

				add_instruction(&output, &output_size, startstr);
				add_instruction(&output, &output_size, " mov rax, [pointer]\n"
				                                       " cmp byte [rax], 0\n"
				                                       " je ");
				add_instruction(&output, &output_size, endstr);

				break;
			case ']':
				sprintf(endstr, "E%u:\n", loop_stack[stack_top - 1]);
				sprintf(startstr, "L%u\n", loop_stack[stack_top - 1]);
				--stack_top; // pop

				add_instruction(&output, &output_size, endstr);
				add_instruction(&output, &output_size, " mov rax, [pointer]\n"
				                                       " cmp byte [rax], 0\n"
				                                       " jne ");
				add_instruction(&output, &output_size, startstr);

				break;
		}
	}
	
	add_instruction(&output, &output_size, " mov rax, [cells]\n"
	                                       " call brk\n"
	                                       ""
	                                       " pop rbp\n"
	                                       ""
	                                       " mov rax, 60\n"
	                                       " mov rdi, 0\n"
	                                       " syscall\n");

	return output;
}

static void add_instruction(char **dest, size_t *dest_size, const char *src)
{
	if (strlen(*dest) + 1 + strlen(src) >= *dest_size) {
		size_t src_length = strlen(src);
		*dest = realloc(*dest, *dest_size * 2 + src_length + 1);
		*dest_size *= 2;
		*dest_size += src_length + 1;
	}

	strcat(*dest, src);
}

void bfci_interpret(const char *src)
{
	char current_char = 0;
	int32_t src_index = 0;
	uint32_t loop_depth = 0;

	size_t cells_size = 128;
	uint8_t *cells = calloc(cells_size, sizeof(uint8_t));
	uint8_t *ptr = cells;

	while ((current_char = src[src_index++]) != 0) {
		switch (current_char) {
			case '+':
				++(*ptr);
				break;
			case '-':
				--(*ptr);
				break;
			case '<':
				if (ptr - cells == 0) {
					fprintf(stderr, "error: data pointer out of range\n");
					exit(EXIT_FAILURE);
				}

				--ptr;
				break;
			case '>':
				if (ptr - cells == cells_size - 1) {
					ptr -= (uintptr_t)cells;

					cells = realloc(cells, cells_size * 2);
					memset(cells + cells_size, 0, cells_size);
					cells_size *= 2;

					ptr += (uintptr_t)cells;
				}

				++ptr;
				break;
			case '.':
				putchar(*ptr);
				break;
			case ',':
				*ptr = getchar();
				break;
			case '[':
				if (*ptr != 0)
					continue;
				
				while ((current_char = src[src_index++]) != 0) {
					if (current_char == '[') 
						++loop_depth;
					else if (current_char == ']') {
						if (loop_depth == 0) 
							break;
						else
							--loop_depth;
					}
				}
				break;
			case ']':
				if (*ptr == 0)
					continue;
				
				while (1) {
					src_index -= 2;
					if (src_index < 0) {
						fputs("error: unbalanced brackets\n", stderr);
						exit(EXIT_FAILURE);
					}
					current_char = src[src_index++];
					
					if (current_char == ']') 
						++loop_depth;
					else if (current_char == '[') {
						if (loop_depth == 0) 
							break;
						else 
							--loop_depth;
					}
				}
				break;
			default:
				continue;
		}
	}

	free(cells);	
}
