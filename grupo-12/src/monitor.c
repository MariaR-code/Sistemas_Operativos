#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define MAXBUFFER 512

typedef struct programas{
	char nr_pid[10];
	char nome[100];
	long int duracao;
	struct programas *next;
}Prog;


Prog *write_struct(char *linha,Prog *percorr){
	char *delim = " ";
	char *token;

	Prog *temp = malloc(sizeof(struct programas));

	token = strtok(linha,delim);
	int spaces = 0;
	token=strtok(NULL,delim);

	while(token != NULL ){
		if(spaces==0){
			strcpy(temp->nr_pid,token);
		}

		if(spaces==1){
			strcpy(temp->nome,token);
		}

		if(spaces==2){
			temp->duracao = atol(token);
			temp->next = NULL;
			spaces = -1;
		}

		spaces++;
		token=strtok(NULL,delim);
	}

	if(percorr==NULL)
		percorr = temp;
	else{
		Prog *aux= percorr;

		while(aux->next){
			aux=aux->next;
		}
		aux->next=temp;
	}

 	return percorr;
}

void read_pid(char *linha,Prog **temp){
	char *delim = " ";
	char *token;
	
	token = strtok(linha,delim);
	token=strtok(NULL,delim);

	while(strcmp((*temp)->nr_pid,token)!=0){
		*temp=(*temp)->next;
	}

	token=strtok(NULL,delim);
	(*temp)->duracao=atol(token)-(*temp)->duracao;
}

void remove_terminados(Prog **terminados,Prog **running,char *pid){

	Prog *ant=NULL;
	Prog *temp;

	temp = *running;

	while(temp != NULL && strcmp(pid,temp->nr_pid)){
		ant=temp;
		temp=temp->next;
	}
	
	Prog *aux = malloc(sizeof(struct programas));
	strcpy(aux->nr_pid,temp->nr_pid);
	strcpy(aux->nome,temp->nome);
	aux->duracao=temp->duracao;
	aux->next=NULL;

	if(*terminados==NULL){
		*terminados = aux;
	}else{
		Prog *temp = *terminados;

		while(temp->next!=NULL){
			temp=temp->next;
		}
		temp->next = aux;
	}

	if(ant==NULL){
		ant=temp;
		*running=temp->next;
		free(ant);
	}

	else{
		Prog *lixo=temp;
		ant->next=temp->next;
		free(lixo);
	}

}

int main(int argc, char *argv[]){
	
	if(mkfifo("monitor_ler" ,0666) == -1 ) 
		perror("Erro na criação do pipe 'monitor_ler'\n");

	if(mkfifo("monitor_escrever" ,0666) == -1 ) 
		perror("Erro na criação do pipe 'monitor_escrever'\n");

	int leitura = open("monitor_ler",O_RDONLY);
	int manter = open("monitor_ler",O_WRONLY);

	char buffer[MAXBUFFER];
	int bytesr;// Ver tamanho
	Prog *running = NULL;
	Prog *terminados = NULL;

	while((bytesr= read(leitura,&buffer,MAXBUFFER))>0){
		char *delim = "\n";
		char *token;
		token = strtok(buffer,delim);
		int flag = 1;
		
		while(token != NULL){
			
			if(token[0]=='f'){
				running = write_struct(token,running);
			}

			if(token[0]=='p'){
				Prog *atualiza = running;
				
				read_pid(token,&atualiza);
				int escrita = open("monitor_escrever",O_WRONLY);

				//printf("%ld\n",(anterior->next)->duracao);

				char tempo[8];
				sprintf(tempo, "%ld",atualiza->duracao);
				int we = write(escrita,tempo,8);

				if(we==-1){
					perror("Não foi possível escrever.");
				}

				remove_terminados(&terminados,&running,atualiza->nr_pid);

				close(escrita);
						
			}

			if(token[0]=='s'){

				int escrita = open("monitor_escrever",O_WRONLY);
				int n;
				Prog *status = running;

				token = strtok(buffer," ");
				token = strtok(NULL, " ");
				

				long int dif;
				while (status != NULL) {
					
					dif =  atol(token) - (status->duracao);


					n = strlen(status->nr_pid) + strlen(status->nome) +14;
					char corre[n];
					
					sprintf(corre,"%s %s %ld ms\n", status->nr_pid, status->nome, dif);
					int we1 = write(escrita,corre,n);

					if(we1==-1){
						perror("Não foi possível escrever.");
					}

					status = status->next;
				}
				close(escrita);
			}
			if(token[0]=='k'){
				flag = 0;
				break;
			}
			
			token=strtok(NULL,delim);

		}
		if(!flag)
			break;
	}
	
	close(leitura);
	close(manter);
	
	unlink("monitor_ler");
	unlink("monitor_escrever");

	return 0;
}
