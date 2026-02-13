#include "symbol.h"
#include <string.h>

PocolSymbol *pocol_symfind(PocolSymbol *sym, SymbolKind kind, const char *name)
{
	for (unsigned int i = 0; i < sym->symbol_count; i++)
		if (sym[i].kind == kind && strcmp(sym[i].name, name) == 0)
			return sym[i];

	return NULL;
}

int pocol_sympush(PocolSymbol *sym, SymData *data)
{
	sym[sym->symbol_count++] = data;
}
