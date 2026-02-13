#include "symbol.h"
#include <string.h>

/* find symbol from sym with kind and name , returning NULL if not found */
SymData *pocol_symfind(PocolSymbol *sym, SymbolKind kind, const char *name)
{
	for (unsigned int i = 0; i < sym->symbol_count; i++)
		if (sym->symbols[i].kind == kind && strcmp(sym->symbols[i].name, name) == 0)
			return &sym->symbols[i];

	return NULL;
}

/* push symbol to sym, if found then return -1 (duplicate), otherwise 0 */
int pocol_sympush(PocolSymbol *sym, SymData *data)
{
	if (pocol_symfind(sym, data->kind, data->name) != NULL) return -1;

	sym->symbols[sym->symbol_count++] = *data;
	return 0;
}
