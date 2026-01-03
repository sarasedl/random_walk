#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../common/protocol.h"
#include "../common/types.h"
#include "../common/config.h"

int main(){
  printf("Server spusteny\n");
  
  while(1){
    sleep(1);
  }
  return 0;
}


