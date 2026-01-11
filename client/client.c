#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include "client.h"
#include <sys/wait.h>
#include "../common/common.h"
#include <poll.h>


void uloz_do_suboru(ThreadData *data, double **priemery, double **pravdepodobnosti){
  int max_x = data->cfg->stlpce/2;
  int max_y = data->cfg->riadky/2;
  int min_x;
  int min_y;

  if (max_x * 2 == data->cfg->stlpce){
    min_x = - max_x + 1;
  } else {
    min_x = - max_x;
  }

  if (max_y * 2 == data->cfg->riadky){
    min_y = - max_y + 1;
  } else {
    min_y = - max_y;
  }	
	FILE *f = fopen(data->cfg->subor_vysledok,"wb");
	if (f == NULL){
		perror("\n[CHYBA] Nepodarilo sa otvorit subor na zapis");
		return;
	} 
	fwrite(data->cfg, sizeof(ConfigMessage),1,f);

	fprintf(f, "\n\n---TEXTOVY PREHLAD VYSLEDKOV ---\n");
	fprintf(f, "Svet: %d riadkov x %d stlpcov\n", data->cfg->riadky, data->cfg->stlpce);
	fprintf(f, "Pravdepodobnosti: Hore:%.2f. Dole:%.2f, Vpravo:%.2f, Vlavo:%.2f\n", data->cfg->p_hore, data->cfg->p_dole, data->cfg->p_vpravo, data->cfg->p_vlavo);
	fprintf(f,"Max. krokov (K):%d\n\n", data->cfg->max_krokov);
	fprintf(f,"Pocet vykonanych replikacii: %d",data->cfg->replikacie); 

	fprintf(f, "TABULKA PRIEMERNYCH KROKOV:\n");
	for(int x = 0; x < data->cfg->stlpce; x++){
		for (int y = 0; y <data->cfg->riadky; y++){
			if (x + min_x == 0 && y + min_y ==0) {
			fprintf(f, " [0,0] ");
			} else if (priemery[x][y] == -1){
			fprintf(f, "    |    ");
			} else {
			fprintf(f, " %8.2f ", priemery[x][y]);
			}
		}
		fprintf(f,"\n");
	}

	fprintf(f, "\nTABULKA PRAVDEPODOBNOSTI:\n");
	for (int x = 0; x < data->cfg->stlpce;x++){
		for(int y = 0;  y < data->cfg->riadky; y++){
			if (x + min_x == 0 && y + min_y == 0) {
			fprintf(f, " [0,0] ");
			} else if(pravdepodobnosti[x][y] == -1){
			fprintf(f, "   |    ");
			} else {
			fprintf(f," %8.2f ", pravdepodobnosti[x][y]);
			}
		}
		fprintf(f,"\n");
	}

	fclose(f);
	printf("\nSimulacia uspesne ulozena do %s\n",data->cfg->subor_vysledok);

}

int nacitaj_zo_suboru_cfg(const char *cesta_k_suboru, ConfigMessage *cfg) {
    FILE *f = fopen(cesta_k_suboru, "rb");
    if (f == NULL) {
        perror("[CHYBA] Nepodarilo sa otvorit subor na citanie");
        return 0;
    }

    size_t precitane = fread(cfg, sizeof(ConfigMessage), 1, f);

    fclose(f);

    if (precitane != 1) {
        fprintf(stderr, "[CHYBA] Subor je poskodeny alebo ma nespravny format.\n");
        return 0;
    }

    return 1;
}
void vykresli_svet(ConfigMessage *cfg, StatusMessage *status){
	printf("Krok cislo %d Replikacia %d/%d\n", status->krok, status->aktualna_replikacia, status->replikacie);
	for (int x = 0; x < cfg->stlpce; x++){
		int x_stred,y_stred;
		if (cfg->stlpce/2 * 2 == cfg->stlpce){
			x_stred = cfg->stlpce/2-1;
		} else {
			x_stred = cfg->stlpce/2;
		}

		if (cfg->riadky/2*2 == cfg->riadky){
			y_stred = cfg->riadky/2-1;
		} else {
			y_stred = cfg->riadky/2;
		}

		for(int y = 0; y < cfg->riadky;y++){
			if ((x== x_stred && y== y_stred) && (x ==status->chodec_x && y == status->chodec_y))printf(" O ");
			else if (x == x_stred && y == y_stred) printf(" X ");
			else if (x == status->chodec_x && y == status->chodec_y) printf(" C ");
			else printf(" . ");
		}
		printf("\n");
	}
}

void *prijimaj_vysledky(void *arg){
  ThreadData *data = (ThreadData*)arg;
  int fd_read = data->fd_read;
  StatusMessage status;
  double **pole_priemerov = malloc(data->cfg->stlpce * sizeof(double *));
  double **pole_pravdepodobnosti = malloc(data->cfg->stlpce * sizeof(double *));

  for (int i = 0; i < data->cfg->stlpce; i ++){
    pole_priemerov[i] = calloc(data->cfg->riadky, sizeof(double));
    pole_pravdepodobnosti[i] = calloc(data->cfg->riadky, sizeof(double));
  }

  while (read(fd_read, &status, sizeof(StatusMessage)) > 0){
    if (status.koniec){
	pthread_mutex_lock(data->mutex);
      data->koniec_simulacie = 1;
      pthread_mutex_unlock(data->mutex);
	uloz_do_suboru(data, pole_priemerov, pole_pravdepodobnosti);
      break;
    } else if (status.mod == 1 && status.koniec_replikacie == 1){
      printf("\n Tabulka priemerneho poctu krokov z pollicka do bodu 0,0");
      vypis_maticu(pole_priemerov, data->cfg->stlpce, data->cfg->riadky, status.aktualna_replikacia, status.replikacie);
    } else if (status.mod == 2 && status.koniec_replikacie == 1){
      printf("\n Tabulka pravdepodobnosti, ze sa chodec z bodu dostane do bodu 0,0");
      vypis_maticu(pole_pravdepodobnosti, data->cfg->stlpce, data->cfg->riadky, status.aktualna_replikacia, status.replikacie);
    } else if (status.mod == 0){
	vykresli_svet((data->cfg),&status);
	sleep(1);
    }
    pole_priemerov[status.chodec_x][status.chodec_y] = status.priemer_krokov;
      pole_pravdepodobnosti[status.chodec_x][status.chodec_y] = status.pravdepodobnost;

  }
  for (int i = 0; i < data->cfg->stlpce;i++){
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

int ysur = min_y;
  for (int x = 0; x < sirka; x++){
	printf("%9d |", ysur);
    for (int y = 0; y <vyska; y++){
      if (x + min_x == 0 && y + min_y == 0) {
        printf("  [0,0] ");
      } else if(pole[x][y] == -1) {
	printf("    |    ");
      } else {
        printf("  %3.2f  ", pole[x][y]);
      }
    }
    ysur ++;
    printf("\n");
  }
    printf("\n");
  sleep(3); 
}

void zobraz_menu(){
  printf("\n--- NAHODNA POCHODZKA MENU ---");
  printf("\n1 Nova simulacia");
  printf("\n2 Opatovne spustenie simulacie");
  printf("\n3 Koniec");
  printf("\nVolba: ");
}


void nacitaj_konfiguraciu(ConfigMessage *cfg){
      cfg->p_dole=0;
      cfg->p_hore=0;
      cfg->p_vlavo=0;
      cfg->p_vpravo=0;
      printf("\nNastavenie novej simulacie\n");
      printf("Zadajte rozmery sveta (riadky[medzera]stlce): ");
      scanf("%d %d", &cfg->riadky, &cfg->stlpce);
      printf("Pocet replikacii: ");
      scanf("%d", &cfg->replikacie);
      while (cfg->p_vpravo + cfg->p_vlavo + cfg->p_hore + cfg->p_dole != 1){
        printf("Pravdepodobnosti podla ktorych sa rozhoduje na ktore z policok sa chodec posunie (vpravo vlavo hore dole) - sucet musi byt rovny 1: ");
        scanf("%lf %lf %lf %lf",&cfg->p_vpravo, &cfg->p_vlavo, &cfg->p_hore, &cfg->p_dole);
        if (cfg->p_dole + cfg->p_hore + cfg->p_vpravo + cfg->p_vlavo != 1){
          printf("Sucet pravdepodobnosti nie je 1, zadajte este raz. ");
        }
      }
      printf("Nazov suboru pre vysledky(maximalne 255 znakov): ");
      scanf("%255s", cfg->subor_vysledok);
      printf("Maximalny pocet krokov: ");
      scanf("%d", &cfg->max_krokov);
      printf("Mod (0 - interaktivny 1 - sumarny, vypis priem. pocet posunov 2 - sumarny, vypis pravdepodobnost ze sa chodec dostane na 0,0: ");
      scanf("%d", &cfg->mod);
      cfg->ukoncit_sever = 0;
      printf("Typ sveta (0 - bez prekazok 1 - s prekazkami, ak vyberiete velkost sveta bude automaticky 10x10): ");
      scanf("%d", &cfg->typ_sveta);
      if (cfg->typ_sveta == 1){
	cfg->riadky = 10;
	cfg->stlpce = 10;
	printf("\n Svet je automaticky zmeneny na velkost 10x10");
      }
}


void spusti_simulaciu(ConfigMessage cfg) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("Fork fail");
        return;
    }

    if (pid == 0){
        execl("server/server", "server", NULL);
        perror("\nChyba pri spusteni servera");
        exit(1);
    } else {

        int fd_out ;
	while((fd_out = open(FIFO_C2S, O_WRONLY)) < 0 ){
		usleep(100000);
	}
        if (fd_out < 0) {
            perror("Klient open C2S fail");
            return;
        }

	pthread_mutex_t m;
	pthread_mutex_init(&m, NULL);
        
        
        write(fd_out, &cfg, sizeof(ConfigMessage));

        
        int fd_in = open(FIFO_S2C, O_RDONLY);
        if (fd_in < 0) {
            perror("Klient open S2C fail");
            close(fd_out);
            return;
        }

        
        pthread_t vlakno;
        ThreadData *data = malloc(sizeof(ThreadData));
        if (data == NULL) {
            close(fd_in);
            close(fd_out);
            return;
        }

        data->fd_read = fd_in;
        data->fd_write = fd_out;
        data->koniec_simulacie = 0;
	data->mutex = &m;
        data->cfg = &cfg; 

        if (pthread_create(&vlakno, NULL, prijimaj_vysledky, data) != 0) {
            perror("Nepodarilo sa vytvorit vlakno");
            free(data);
	}
        pthread_join(vlakno, NULL);
        pthread_mutex_destroy(&m);
        free(data);
        close(fd_in);
        close(fd_out); 
        
    }
}

int main(){
  int volba = 0;

  while (volba != 3){
    zobraz_menu();
    if (scanf("%d", &volba) != 1) break;

    if (volba ==1){
      ConfigMessage cfg;
	memset(&cfg, 0, sizeof(ConfigMessage));
      nacitaj_konfiguraciu(&cfg);
      fflush(stdout);
      spusti_simulaciu(cfg);

    } else if (volba == 2) {
	ConfigMessage cfg_obnova;
	memset(&cfg_obnova, 0, sizeof(ConfigMessage));
	char subor_cesta[256];

    	printf("Zadajte nazov suboru pre nacitanie: ");
    	scanf("%255s", subor_cesta);
    	if (nacitaj_zo_suboru_cfg(subor_cesta, &cfg_obnova)) {
        	printf("\nNacitane zo suboru: Svet %dx%d\n", cfg_obnova.riadky, cfg_obnova.stlpce);

        printf("Zadajte novy pocet replikacii: ");
        scanf("%d", &cfg_obnova.replikacie);
        printf("Zadajte nazov suboru pre uloženie nových výsledkov: ");
        scanf("%255s", cfg_obnova.subor_vysledok);
        cfg_obnova.ukoncit_sever = 0;

   	  }
	spusti_simulaciu(cfg_obnova);
	}
    }
   
  printf("[KLIENT] Aplikacia ukoncena\n");
  return 0;
}
