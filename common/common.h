#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>

#define FIFO_C2S "vstup_server.fifo"
#define FIFO_S2C "vyspu_klient.fifo"

typedef struct {
	int sirka;
	int vyska;
	int **prekazky;
} Svet;

//sprava od klienta pre vstup_server
typedef struct {
  int riadky;
  int stlpce;
  int replikacie;
  int max_krokov;
  double p_hore, p_dole, p_vlavo, p_vpravo;
  int ukoncit_sever;
  char subor_vysledok[256];
  int mod;
  int typ_sveta;
} ConfigMessage;

//data od servera klientovi
typedef struct {
  int aktualna_replikacia;
  int replikacie;
  int chodec_x;
  int chodec_y;
  int krok;
  double priemer_krokov;
  double pravdepodobnost;
  int koniec;
  int mod;
  int typ_sveta;
  int riadky;
  int stlpce;
  int koniec_replikacie;
} StatusMessage;

//struktura na odovzdanie dat vlaknu v klientovi
typedef struct{
  int fd_write;
  int fd_read;
  int koniec_simulacie;
  ConfigMessage *cfg;
  pthread_mutex_t *mutex;
  } ThreadData;

#endif
