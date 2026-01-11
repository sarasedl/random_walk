#ifndef SERVER_H
#define SERVER_H

#include "../common/common.h"

/**
 * Načíta mapu prekážok z textového súboru.
 * Používa štruktúru Svet definovanú v common.h.
 */
Svet *nacitaj_svet(const char *nazov_suboru, int sirka, int vyska);

/**
 * Vytvorí dynamicky alokovanú štruktúru sveta bez prekážok.
 */
Svet *vytvor_prazdny_svet(int riadky, int stlpce);

#endif // SERVER_H
