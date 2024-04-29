#ifndef BFCI_H
#define BFCI_H

enum bfci_type {
	BFCI_INTERPRET,
	BFCI_COMPILE_C,
	BFCI_COMPILE_ASM
};

struct bfci_options {
	enum bfci_type type;
	bool optimise;
	bool dynamic;
};

// frees src and returns a heap allocated string
char *bfci_ci(char *src, struct bfci_options options);

#endif
