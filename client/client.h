#ifndef CLIENT_H
#define CLIENT_H
#include <pthread.h>
#include "../common/common.h"

/**
 * Načíta uloženú konfiguráciu zo súboru pre opätovné spustenie (Voľba 2).
 */
int nacitaj_zo_suboru_cfg(const char *cesta_k_suboru, ConfigMessage *cfg);

/**
 * Vykreslí textovú reprezentáciu sveta a pozíciu chodca (Interaktívny mód).
 */
void vykresli_svet(ConfigMessage *cfg, StatusMessage *status);

/**
 * Funkcia vlákna, ktorá spracováva prichádzajúce správy zo servera.
 */
void *prijimaj_vysledky(void *arg);

/**
 * Formátovaný výpis matice výsledkov do konzoly.
 */
void vypis_maticu(double **pole, int sirka, int vyska, int aktualna_replikacia, int replikacie);

/**
 * Zobrazí hlavné menu v konzole.
 */
void zobraz_menu();

/**
 * Načíta od používateľa vstupné parametre pre novú simuláciu.
 */
void nacitaj_konfiguraciu(ConfigMessage *cfg);

/**
 * Vykoná fork(), spustí server a inicializuje komunikáciu a vlákno.
 */
void spusti_simulaciu(ConfigMessage cfg);
#endif
