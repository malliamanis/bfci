#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "bfci.h"

static char *add_instruction(char *dest, size_t *dest_size, const char *src);

char *bfci_compile_c(const char *src)
{
	char current;
	uint32_t index = 0;

	size_t output_size = 512;
	char *output = calloc(output_size, sizeof(char));

	output = add_instruction(output, &output_size, "#include<stdio.h>\n"
	                                               "#include<stdlib.h>\n"
	                                               "#include<stdint.h>\n"
	                                               "#include<string.h>\n"
	                                               ""
	                                               // macros to reduce file size
	                                               "#define I ++*p;\n"          // increment
	                                               "#define D --*p;\n"          // decrement
	                                               "#define L --p;\n"           // left
	                                               "#define R d(&c,&p,&s);++p;\n" // right
	                                               "#define O putchar(*p);\n"   // output
	                                               "#define G *p=getchar();\n"  // get
	                                               "#define S while(*p){\n"     // start (loop)
	                                               "#define E }\n"              // end (loop)
	                                               ""
	                                               "void d(uint8_t**c,uint8_t**p,size_t*s)" // double cells size if needed
	                                               "{"
	                                                "if(*p-*c==*s-1){"
	                                                 "*p-=(uintptr_t)*c;"
	                                                 "*c=realloc(*c,*s*2);"
	                                                 "memset(*c+*s,0,*s-1);"
	                                                 "*s*=2;"
	                                                 "*p+=(uintptr_t)*c;"
	                                                "}"
	                                                "++p;"
	                                               "}"
	                                               ""
	                                               "int main(void)"
	                                               "{"
	                                                "size_t s=256;"                              // cells size
	                                                "uint8_t*c=calloc(s,sizeof(unsigned char));" // cells
	                                                "uint8_t*p=c;");                             // data pointer

	while ((current = src[index++]) != 0) {
		switch (current) {
			case '+':
				output = add_instruction(output, &output_size, "I ");
				break;
			case '-':
				output = add_instruction(output, &output_size, "D ");
				break;
			case '<':
				output = add_instruction(output, &output_size, "L ");
				break;
			case '>':
				output = add_instruction(output, &output_size, "R ");
				break;
			case '.':
				output = add_instruction(output, &output_size, "O ");
				break;
			case ',':
				output = add_instruction(output, &output_size, "G ");
				break;
			case '[':
				output = add_instruction(output, &output_size, "S ");
				break;
			case ']':
				output = add_instruction(output, &output_size, "E ");
				break;
		}
	}
	
	output = add_instruction(output, &output_size,  "free(c);"
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
	uint32_t loop_stack[1024] = {0};
	int32_t stack_top = 0;

	char startstr[10] = {0};
	char endstr[10] = {0};

	output = add_instruction(output, &output_size, "default rel\n"
	                                               "global _start\n"
	                                               ""
	                                               "section .bss\n"
	                                               "cells: resq 1\n"
	                                               "pointer: resq 1\n"
	                                               "size: resd 1\n"
	                                               ""
	                                               "section .text\n"
	                                               "brk:\n"
	                                               " mov eax, 12\n"
	                                               " syscall\n"
	                                               " ret\n"
	                                               ""
	                                               "D:\n"
	                                               " mov rax, [pointer]\n"
	                                               " mov rcx, [cells]\n"
	                                               " sub rax, rcx\n"
	                                               " mov rbx, [size]\n"
	                                               " dec rbx\n"
	                                               " cmp rax, rbx\n"
	                                               ""
	                                               " jne ENDIF\n"
	                                               " lea ecx, [size]\n"
	                                               " mov edi, [ecx]\n"
	                                               " shl edi, 2\n"
	                                               " call brk\n"
	                                               ""
	                                               " mov rdx, [cells]\n"
	                                               " add rbx, rcx\n"
	                                               " xor eax, eax\n"
	                                               " mov rdi, [rcx]\n"
	                                               " dec rdi\n"
	                                               " rep stosb\n"
	                                               ""
	                                               " shl dword [ecx], 2\n"
	                                               " ENDIF:\n"
	                                               " ret\n"
	                                               ""
	                                               "O:\n"
	                                               " mov eax, 1\n"
	                                               " mov edi, 1\n"
	                                               " mov rsi, [pointer]\n"
	                                               " mov edx, 1\n"
	                                               " syscall\n"
	                                               " ret\n"
	                                               ""
	                                               "I:\n"
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
	                                               " mov [size], dword 0x10\n"
	                                               " mov eax, [size]\n"
	                                               " add rdi, rax\n"
	                                               " call brk\n");

	while ((current = src[index++]) != 0) {
		switch (current) {
			case '+':
				output = add_instruction(output, &output_size, " mov rax, [pointer]\n"
				                                               " inc byte [rax]\n");
				break;
			case '-':
				output = add_instruction(output, &output_size, " mov rax, [pointer]\n"
				                                               " dec byte [rax]\n");
				break;
			case '<':
				output = add_instruction(output, &output_size, " dec qword [pointer]\n");
				break;
			case '>':
				output = add_instruction(output, &output_size, " inc qword [pointer]\n");
				break;
			case '.':
				output = add_instruction(output, &output_size, " call O\n");
				break;
			case ',':
				output = add_instruction(output, &output_size, " call G\n");
				break;
			case '[':
				loop_stack[stack_top++] = loop_counter; // push

				sprintf(startstr, "L%u:\n", loop_counter);
				sprintf(endstr, "E%u\n", loop_counter);

				++loop_counter;

				output = add_instruction(output, &output_size, startstr);
				output = add_instruction(output, &output_size, " mov rax, [pointer]\n"
				                                               " cmp byte [rax], 0\n"
				                                               " je ");
				output = add_instruction(output, &output_size, endstr);

				break;
			case ']':
				sprintf(endstr, "E%u:\n", loop_stack[stack_top - 1]);
				sprintf(startstr, "L%u\n", loop_stack[stack_top - 1]);
				--stack_top; // pop

				output = add_instruction(output, &output_size, endstr);
				output = add_instruction(output, &output_size, " mov rax, [pointer]\n"
				                                               " cmp byte [rax], 0\n"
				                                               " jne ");
				output = add_instruction(output, &output_size, startstr);

				break;
		}
	}
	
	output = add_instruction(output, &output_size, " mov rax, [cells]\n"
	                                               " call brk\n"
	                                               ""
	                                               " pop rbp\n"
	                                               ""
	                                               " mov rax, 60\n"
	                                               " mov rdi, 0\n"
	                                               " syscall\n");

	return output;
}

static char *add_instruction(char *dest, size_t *dest_size, const char *src)
{
	if (strlen(dest) + 1 + strlen(src) >= *dest_size) {
		size_t src_length = strlen(src);
		dest = realloc(dest, *dest_size * 2 + src_length + 1);
		memset(dest + *dest_size, 0, *dest_size + src_length + 1);
		*dest_size = *dest_size * 2 + src_length + 1;
	}

	strcat(dest, src);

	return dest;
}

void bfci_interpret(const char *src)
{
	char current_char = 0;
	int32_t src_index = 0;
	uint32_t loop_depth = 0;

	size_t cells_size = 100;
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
					fputs("error: data pointer out of range\n", stderr);
					exit(EXIT_FAILURE);
				}

				--ptr;
				break;
			case '>':
				if (ptr - cells == cells_size - 1) {
					ptr -= (uintptr_t)cells;

					cells = realloc(cells, cells_size * 2);
					memset(cells + cells_size, 0, cells_size - 1);
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
