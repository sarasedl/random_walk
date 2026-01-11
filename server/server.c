#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "server.h"
#include "../common/common.h"

Svet *nacitaj_svet(const char *nazov_suboru, int sirka, int vyska) {
    FILE *f = fopen(nazov_suboru, "r");
    if (!f) {
        perror("Nepodarilo sa otvorit subor s prekazkami");
        return NULL;
    }

    Svet *svet = malloc(sizeof(Svet));
    if (!svet) {
        perror("malloc");
        fclose(f);
        return NULL;
    }

    svet->sirka = sirka;
    svet->vyska = vyska;

    svet->prekazky = malloc(vyska * sizeof(int *));
    if (!svet->prekazky) {
        perror("malloc");
        fclose(f);
        free(svet);
        return NULL;
    }

    for (int y = 0; y < vyska; y++) {
        svet->prekazky[y] = malloc(sirka * sizeof(int));
        if (!svet->prekazky[y]) {
            perror("malloc");
            fclose(f);
            return NULL;
        }

        for (int x = 0; x < sirka; x++) {
            if (fscanf(f, "%d", &svet->prekazky[y][x]) != 1) {
                fprintf(stderr, "Chyba pri citani mapy\n");
                fclose(f);
                return NULL;
            }
        }
    }

    fclose(f);


    if (svet->prekazky[0][0] == 1) {
        fprintf(stderr, "(0,0) nemoze byt prekazka!\n");
        return NULL;
    }

    return svet;
}

Svet *vytvor_prazdny_svet(int riadky, int stlpce) {
    Svet *svet = malloc(sizeof(Svet));
    if (!svet) return NULL;

    svet->vyska = riadky;
    svet->sirka = stlpce;

    // alokacia riadkov
    svet->prekazky = malloc(riadky * sizeof(int *));
    if (!svet->prekazky) {
        free(svet);
        return NULL;
    }

    // alokacia stlpcov + nulovanie
    for (int i = 0; i < riadky; i++) {
        svet->prekazky[i] = calloc(stlpce, sizeof(int));
        if (!svet->prekazky[i]) {
            // cleanup pri chybe
            for (int j = 0; j < i; j++) {
                free(svet->prekazky[j]);
            }
            free(svet->prekazky);
            free(svet);
            return NULL;
        }
    }

    return svet;
}


int main(){
  srand(time(NULL));
  mkfifo(FIFO_C2S, 0666);
  mkfifo(FIFO_S2C, 0666);
  ConfigMessage cfg;
  StatusMessage status;
  memset(&status,0,sizeof(StatusMessage));

  int fd_in = open(FIFO_C2S, O_RDONLY);
  int fd_out = open(FIFO_S2C, O_WRONLY);
  if (read(fd_in, &cfg, sizeof(ConfigMessage)) > 0 ){
    //prepkutie rury na nebolukujucu  
    int flags = fcntl(fd_in, F_GETFL,0); //ziska aktualne nastavenia;
    fcntl(fd_in, F_SETFL, flags | O_NONBLOCK); //prida sa priznak neblokovania;
    int stred_x = cfg.stlpce / 2;
    int stred_y = cfg.riadky / 2;
    int riadky_hranica_hore, riadky_hranica_dole, stlpce_hranica_lava, stlpce_hranica_prava;

    if (stred_y * 2 != cfg.riadky){
      riadky_hranica_dole = -stred_y;
      riadky_hranica_hore = stred_y;
    } else {
      riadky_hranica_dole = - stred_y +1;
      riadky_hranica_hore = stred_y;
    }
    if (stred_x * 2 != cfg.stlpce){
      stlpce_hranica_lava = -stred_x;
      stlpce_hranica_prava = stred_x;
    } else {
      stlpce_hranica_lava = -stred_x +1;
      stlpce_hranica_prava = stred_x;
    }
    Svet *svet;
	if (cfg.typ_sveta == 1) {
		svet = nacitaj_svet("prekazky.txt", 10,10);
	 } else {
		svet = vytvor_prazdny_svet(cfg.riadky, cfg.stlpce);
	 }
    long **suma_krokov = malloc(cfg.stlpce * sizeof(long *));
    int **pocet_uspechov = malloc(cfg.stlpce * sizeof(int *));
    for (int i = 0; i < cfg.stlpce;i++){
      suma_krokov[i] = calloc(cfg.riadky, sizeof(long));
      pocet_uspechov[i] = calloc(cfg.riadky, sizeof(int));
    }

    status.replikacie = cfg.replikacie;
    status.mod = cfg.mod;
    status.koniec = 0;

    for (int r = 1; r <= cfg.replikacie;r++){
      

      status.koniec_replikacie = 0;
      status.aktualna_replikacia = r;
      
      for (int i = 0; i < cfg.stlpce; i++){
        for (int j = 0; j < cfg.riadky; j++) {
          if (i + stlpce_hranica_lava == 0 && j+riadky_hranica_dole ==0) continue;
	  if (cfg.typ_sveta == 1 && svet->prekazky[i][j] == 1) continue;
          int curr_x = i + stlpce_hranica_lava;
          int curr_y = j + riadky_hranica_dole;
          int k = 0;
          
          while ((curr_x != 0 || curr_y != 0) && k < cfg.max_krokov){
		if (k == 0){
	    status.chodec_x = curr_x - stlpce_hranica_lava;
            status.chodec_y = curr_y - riadky_hranica_dole;
            status.krok = k;
	    write(fd_out, &status, sizeof(StatusMessage));
		}
            double random = (double)rand() / RAND_MAX;
		int next_x = curr_x;
		int next_y = curr_y;
            if (random < cfg.p_hore) next_y ++;
            else if (random < (cfg.p_hore + cfg.p_dole)) next_y--;
            else if (random < (cfg.p_hore + cfg.p_dole + cfg.p_vpravo)) next_x ++;
            else next_x --;

            //prechadzanie z jedneho kraja na druhy
            
            if (next_x < stlpce_hranica_lava){
              next_x = stlpce_hranica_prava;
            } else if ( next_x > stlpce_hranica_prava){
              next_x = stlpce_hranica_lava;
            } else if (next_y < riadky_hranica_dole) {
              next_y = riadky_hranica_hore;
            } else if (next_y > riadky_hranica_hore) {
              next_y = riadky_hranica_dole;
            }
		if ((cfg.typ_sveta == 1 && svet->prekazky[next_x - stlpce_hranica_lava][next_y - riadky_hranica_dole] != 1) || cfg.typ_sveta ==0 ){
			curr_x = next_x;
			curr_y = next_y;
		} 

            k ++;
            status.chodec_x = curr_x - stlpce_hranica_lava;
            status.chodec_y = curr_y - riadky_hranica_dole;
            status.krok = k;
	    write(fd_out, &status, sizeof(StatusMessage));
          }
          suma_krokov[i][j] += k;
          if (curr_x == 0 && curr_y == 0){
            pocet_uspechov[i][j]++;
          }
        }

      }
        for (int i = 0; i <cfg.stlpce;i++){
          for (int j = 0; j < cfg.riadky; j++){
            status.chodec_x = i;
            status.chodec_y = j;
	    if (cfg.typ_sveta == 1 && svet->prekazky[i][j] == 1){
		    status.pravdepodobnost = -1;
		    status.priemer_krokov = -1;
	     } else {
            status.pravdepodobnost = (double)pocet_uspechov[i][j] / status.aktualna_replikacia * 100;
            status.priemer_krokov = (double)suma_krokov[i][j] / status.aktualna_replikacia;
	     }
            write(fd_out,&status, sizeof(StatusMessage));
          }
        }
      
      status.koniec_replikacie = 1;
      write(fd_out, &status, sizeof(StatusMessage));
    }
    


  
  //treba poslat signal klientovi, ze sme skoncili
    for (int i = 0; i < cfg.stlpce; i++){
    free(suma_krokov[i]);
    free (pocet_uspechov[i]);
  }
  free (suma_krokov);
  free(pocet_uspechov);
  for (int i = 0; i < svet->vyska;i++){
	free(svet->prekazky[i]);
  }
  free(svet->prekazky);
  free(svet);
  status.koniec = 1;
  write(fd_out, &status, sizeof(StatusMessage));

  close(fd_in);
  close(fd_out);
  unlink(FIFO_C2S);
  unlink(FIFO_S2C);
  }

  return 0;


}
