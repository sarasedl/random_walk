#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "server.h"
#include "../common/common.h"

int main(){
  srand(time(NULL));
  mkfifo(FIFO_C2S, 0666);
  mkfifo(FIFO_S2C, 0666);
  ConfigMessage cfg;
  StatusMessage status;

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

    long **suma_krokov = malloc(cfg.stlpce * sizeof(long *));
    int **pocet_uspechov = malloc(cfg.stlpce * sizeof(int *));
    for (int i = 0; i < cfg.stlpce;i++){
      suma_krokov[i] = calloc(cfg.riadky, sizeof(long));
      pocet_uspechov[i] = calloc(cfg.riadky, sizeof(int));
    }

    status.replikacie = cfg.replikacie;
    status.mod = cfg.mod;
    status.koniec = 0;
    ConfigMessage prijata_sprava;

    for (int r = 1; r <= cfg.replikacie;r++){
      
      if (read(fd_in, &prijata_sprava, sizeof(ConfigMessage)) > 0) {
        status.mod = prijata_sprava.mod;
        status.koniec = prijata_sprava.ukoncit_sever;
      }
      
      if (status.koniec) {
        break;
      }

      status.koniec_replikacie = 0;
      status.aktualna_replikacia = r;
      
      for (int i = 0; i < cfg.stlpce; i++){
        for (int j = 0; j < cfg.riadky; j++) {
          if (i + stlpce_hranica_lava == 0 && j+riadky_hranica_dole ==0) continue;
          int curr_x = i + stlpce_hranica_lava;
          int curr_y = j + riadky_hranica_dole;
          int k = 0;
          
          while ((curr_x != 0 || curr_y != 0) && k < cfg.max_krokov){

		/*if (read(fd_in, &prijata_sprava, sizeof(ConfigMessage)) > 0){
			status.mod = prijata_sprava.mod;
			status.koniec = prijata_sprava.ukoncit_sever;
		}*/
		if (status.koniec){
			break;
		}

            double random = (double)rand() / RAND_MAX;

            if (random < cfg.p_hore) curr_y ++;
            else if (random < (cfg.p_hore + cfg.p_dole)) curr_y--;
            else if (random < (cfg.p_hore + cfg.p_dole + cfg.p_vpravo)) curr_x ++;
            else curr_x --;

            //prechadzanie z jedneho kraja na druhy
            
            if (curr_x < stlpce_hranica_lava){
              curr_x = stlpce_hranica_prava;
            } else if ( curr_x > stlpce_hranica_prava){
              curr_x = stlpce_hranica_lava;
            } else if (curr_y < riadky_hranica_dole) {
              curr_y = riadky_hranica_hore;
            } else if (curr_y > riadky_hranica_hore) {
              curr_y = riadky_hranica_dole;
            }

            //tu este by to chcelo odoslat nejaku spravu ak bude ten mod, kde sa to bude vykreslovat

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
          //aj tu sa mozno bude odosielat sprava
        }

      }
      if (status.mod == 2 || status.mod == 1){
        for (int i = 0; i <cfg.stlpce;i++){
          for (int j = 0; j < cfg.riadky; j++){
            status.chodec_x = i;
            status.chodec_y = j;
            status.pravdepodobnost = (double)pocet_uspechov[i][j] / status.aktualna_replikacia;
            status.priemer_krokov = (double)suma_krokov[i][j] / status.aktualna_replikacia;
            write(fd_out,&status, sizeof(StatusMessage));
          }
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
  
  status.koniec = 1;
  write(fd_out, &status, sizeof(StatusMessage));

  close(fd_in);
  close(fd_out);
  unlink(FIFO_C2S);
  unlink(FIFO_S2C);
  return 0;
  }
}
