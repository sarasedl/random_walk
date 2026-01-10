#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "client.h"
#include "../common/common.h"

void *prijimaj_vysledky(void *arg){
  ThreadData *data = (ThreadData*)arg;
  int fd_read = data->fd_read;
  StatusMessage status;
  double **pole_priemerov = malloc(data->stlpce * sizeof(double *));
  double **pole_pravdepodobnosti = malloc(data->stlpce * sizeof(double *));

  for (int i = 0; i < data->stlpce; i ++){
    pole_priemerov[i] = calloc(data->riadky, sizeof(double));
    pole_pravdepodobnosti[i] = calloc(data->riadky, sizeof(double));
  }

  while (read(fd_read, &status, sizeof(StatusMessage)) > 0){
    if (status.koniec){
      data->koniec_simulacie = 1;
      break;}
    if (status.mod == 1 && status.koniec_replikacie == 0){
      pole_priemerov[status.chodec_x][status.chodec_y] = status.priemer_krokov;
    } else if (status.mod == 2 && status.koniec_replikacie == 0) {
      pole_pravdepodobnosti[status.chodec_x][status.chodec_y] = status.pravdepodobnost;
    } else if (status.mod == 1 && status.koniec_replikacie == 1){
      printf("\n Tabulka priemerneho poctu krokov z pollicka do bodu 0,0");
      vypis_maticu(pole_priemerov, data->stlpce, data->riadky, status.aktualna_replikacia, status.replikacie);
    } else if (status.mod == 2 && status.koniec_replikacie == 1){

      printf("\n Tabulka pravdepodobnosti, ze sa chodec z bodu dostane do bodu 0,0");
      vypis_maticu(pole_pravdepodobnosti, data->stlpce, data->riadky, status.aktualna_replikacia, status.replikacie);
    } else {

    }

  }
  for (int i = 0; i < data->stlpce;i++){
    free(pole_priemerov[i]);
    free(pole_pravdepodobnosti[i]);
  }
  free(pole_priemerov);
  free(pole_pravdepodobnosti);
  return NULL;
}

void vypis_maticu(double **pole, int sirka, int vyska, int aktualna_replikacia, int replikacie){
  int max_x = sirka/2;
  int max_y = vyska/2;
  int min_x;
  int min_y;

  if (max_x * 2 == sirka){
    min_x = - max_x + 1;
  } else {
    min_x = - max_x;
  }

  if (max_y * 2 == vyska){
    min_y = - max_y + 1;
  } else {
    min_y = - max_y;
  }

  printf("\n Replikacia %d / %d\n\n", aktualna_replikacia, replikacie);
  
  printf("      y\\x |");
  for (int x = 0; x < sirka; x++){
    printf("%7d ", x + min_x);
  }
  printf("\n----------+");
  for(int x = 0; x < sirka; x++) printf("----------");
  printf("\n");

  for (int y = vyska - 1; y >= 0; y --){
    printf("%9d |", y + min_y);
    for (int x = 0; x <sirka; x++){
      if (x + min_x == 0 && y + min_y == 0) {
        printf("  [0,0] ");
      } else {
        printf("  %3.2f  ", pole[x][y]);
      }
    }
    printf("\n");
  }
    printf("\n");
  sleep(3); 
}

void zobraz_menu(){
  printf("\n--- NAHODNA POCHODZKA MENU ---");
  printf("\n1 Nova simulacia");
  printf("\n2 Pripojit k simulacii");
  printf("\n3 Opatovne spustenie simulacie");
  printf("\n4 Koniec");
  printf("\nVolba: ");
}

int main(){
  mkfifo(FIFO_C2S, 0666);
  mkfifo(FIFO_S2C, 0666);
  int volba = 0;

  while (volba != 4){
    zobraz_menu();
    if (scanf("%d", &volba) != 1) break;

    if (volba ==1){
      ConfigMessage cfg;
      cfg.p_dole=0;
      cfg.p_hore=0;
      cfg.p_vlavo=0;
      cfg.p_vpravo=0;
      printf("Zadajte rozmery sveta (riadky[medzera]stlce): ");
      scanf("%d %d", &cfg.riadky, &cfg.stlpce);
      printf("Pocet replikacii: ");
      scanf("%d", &cfg.replikacie);
      while (cfg.p_vpravo + cfg.p_vlavo + cfg.p_hore + cfg.p_dole != 1){
        printf("Pravdepodobnosti podla ktorych sa rozhoduje na ktore z policok sa chodec posunie (vpravo vlavo hore dole) - sucet musi byt rovny 1: ");
        scanf("%lf %lf %lf %lf",&cfg.p_vpravo, &cfg.p_vlavo, &cfg.p_hore, &cfg.p_dole);
        if (cfg.p_dole + cfg.p_hore + cfg.p_vpravo + cfg.p_vlavo != 1){
          printf("Sucet pravdepodobnosti nie je 1, zadajte este raz. ");
        }
      }
      printf("Nazov suboru pre vysledky(maximalne 255 znakov): ");
      scanf("%255s", cfg.subor_vysledok);
      printf("Maximalny pocet krokov: ");
      scanf("%d", &cfg.max_krokov);
      printf("Mod (0 - interaktivny 1 - sumarny, vypis priem. pocet posunov 2 - sumarny, vypis pravdepodobnost ze sa chodec dostane na 0,0: ");
      scanf("%d", &cfg.mod);
      printf("Typ sveta (0 - bez prekazok 1 - s prekazkami): ");
      scanf("%d", &cfg.typ_sveta);
      printf("Ak budete pocas simulacie chciet zmenit mod, alebo ukoncit simulaciu stlacte cislo aky mod chcete a enter, a mod sa automaticky zmeni (0 - interaktivny 1 - sumarny, kde sa zobrazuje priemerny pocet posunov na dosiahnutie 0,0 2 - sumarny, kde sa vykresluje pravdepodobnost, s akou sa odial dostaneme do bodu 0,0   3 - koniec");
      cfg.ukoncit_sever = 0;
      sleep(5);
      pid_t pid = fork();

      if (pid == 0) {
        execl("server/server","server",NULL);
        perror("\nChyba pri spusteni servera");
        exit(1);
      } else {
        int fd_out = open(FIFO_C2S,O_WRONLY);
        if (fd_out < 0){perror("Klient open C2s fail");exit(1);}
        write(fd_out, &cfg, sizeof(ConfigMessage));
        close(fd_out);

        int fd_in = open(FIFO_S2C, O_RDONLY);
        if (fd_in <0){ perror("Klient open S2C fail"); exit(1);}

        pthread_t vlakno;
        ThreadData *data = malloc(sizeof(ThreadData));
        if (data == NULL) return 1;

        data->fd_read = fd_in;
        data->stlpce = cfg.stlpce;
        data->riadky = cfg.riadky;
        data->koniec_simulacie = 0;

        if(pthread_create(&vlakno, NULL, prijimaj_vysledky, data) != 0 ) {
          perror("Nepodarilo sa vytvorit vlakno");
        }
        
        int zmena_modu;
        while(data->koniec_simulacie == 0){
          if (scanf(" %d", &zmena_modu) == 1) {
            if( zmena_modu == 3) {
              cfg.ukoncit_sever = 1;
            } else {
              cfg.mod = zmena_modu;
            }
            write(fd_out, &cfg, sizeof(ConfigMessage));
          }
        }

        pthread_join(vlakno,NULL);
        close(fd_in);
        close(fd_out);
        unlink(FIFO_C2S);
        unlink(FIFO_S2C);
      }
    } else {

    }
  } 
  printf("[KLIENT] Aplikacia ukoncena\n");
  return 0;
}
