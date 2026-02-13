#ifndef POCOL_SYMBOL_H
#define POCOL_SYMBOL_H

#include "../pm/vm.h"

typedef enum {
	SYM_LABEL,
} SymbolKind;

typedef struct {
	Inst_Addr pc;	/* the address of program counter to go */
	int is_defined; /* is defined first? (handles forward reference)*/
} SymLabel;

typedef struct {
	char *name;
	SymbolKind kind; /* symbol kind types */

	union {
		SymLabel label;
	} as;
} SymData;

typedef struct {
	SymData *symbols;
	unsigned int symbol_count;
} PocolSymbol;

SymData *pocol_symfind(PocolSymbol *sym, SymbolKind kind, const char *name);
int pocol_sympush(PocolSymbol *sym, SymData *data);

#endif /* POCOL_SYMBOL_H */
