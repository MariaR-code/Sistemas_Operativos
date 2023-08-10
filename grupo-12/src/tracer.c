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

struct timeval tv;

long int gettime(){

	int ret;
	ret = gettimeofday(&tv,NULL);
	
	if(ret == 0){
		long int ms = ((tv.tv_usec)/1000)+((tv.tv_sec)*1000);
		return ms;
	} else {
		fprintf(stderr, "Erro: gettimeofday()");
		return -1;
	}
}

int count_param(char* string){
	int counter=0;
	char *spaces= strchr(string, ' ');

	while(spaces!=NULL){
		counter++;
		spaces = strchr(spaces+1, ' ');
	}

	return counter+1;
}

int count_progs(char* string){
	int counter=0;
	char *pipes= strchr(string, '|');

	while(pipes!=NULL){
		counter++;
		pipes = strchr(pipes+1, '|');
	}

	return counter+1;
}

void listar_arguments(char* token,char** l_strings){
	int i=0;
	while(token != NULL){

		l_strings[i]= malloc(sizeof(char)*(strlen(&token[0])+1));
		strcpy(l_strings[i],&token[0]);
		token=strtok(NULL," ");
		i++;
	}
	l_strings[i]= NULL;
}

char *replace(char toda[],char substr[], char sub[], int N){
	//printf("%s\n\n",toda);

	int i = 0, j = 0, start = 0, i2=0, start2=0;
	char *output = malloc(sizeof(char)*(strlen(toda)-(N-1)));
	for(int n=0;n<N;n++){

	    while (i2<strlen(toda) && j<strlen(substr)){     //"cat notas | grep "Olá" | wc -l" <- output
	    							 //"cat notas|grep "Olá" | wc -l" <-toda

	        if(toda[i2] == substr[j] && j!=strlen(substr)) {
                start = i2;
            	j++;
	        }
	        else if(j==strlen(substr)){ break;}
	        else{
	        	
	        	j=0;

	        }
	        i2++;
	    }

	    
	    if (j==strlen(substr)){

	        start2=(start-2);
	       	
	       	for (i = 0; i < start2; i++){
	            output[i] = toda[i];
	           	
	        }

		    for (j = 0; j < strlen(sub); j++){
	            output[i] = sub[j];
	            
	            i++;
		    }

		        
		         
		    for (j = i+2; j < strlen(toda); j++){
	            output[i] = toda[j];
	            
	            i++;
		    }
		    output[i]='\0';
		    toda=output;	        
	        
		}
	    
	    j=0;

	}
	return toda;
	
}



int main(int argc, char * argv[]){

	if(argc < 2){
		fprintf(stderr,"Não foi inserida a ação para ser efetuada.\n");
		return 1;
	}

	
	int pipe1 = open("monitor_ler",O_WRONLY);
	gettimeofday(&tv,NULL);
	
	
	if(strcmp(argv[1],"execute")==0){
		
		if(argc < 4){
			fprintf(stderr, "Não foi inserido todos os dados de forma a ser possível proseguir com a operação.\n");
			return 1;
		}


		if(strcmp(argv[2],"-u")==0){
			
			char *prog = malloc(sizeof(char)*(strlen(argv[3])+1));
			char *delim = " ";
			char *token;

			//printf("%s\n",argv[3]);
			int pal = count_param(argv[3]);
			char **cmd = (char **) malloc(sizeof(char*)*pal);

			strcpy(prog,argv[3]);
			token = strtok(prog,delim);
			strcpy(prog, token);
			
			
			
			int fd[2];
			int pe = pipe(fd);

			if(pe==-1){
				perror("Não foi possível criar o pipe anónimo.");
			}

			pid_t f = fork();

			if(f==-1){
				perror("Não foi possível criar o filho.");
			}			

			if (f == 0){
				
				close(fd[0]);
				listar_arguments(token,cmd);

				pid_t pidfilho = getpid();
				printf("RUNING PID %d\n", pidfilho);
				
				int we = write(fd[1],&pidfilho,sizeof(pid_t));

				if(we==-1){
					perror("Não foi possível escrever.");
				}

				int tam = strlen(prog)+17;
				char info_fork[tam];

				long int tempo_i;
				tempo_i = gettime();

				sprintf(info_fork,"f %d %s %ld\n",pidfilho,prog,tempo_i); 
				int we1 = write(pipe1,&info_fork,strlen(info_fork)); //envia info_fork para o servidor

				if(we1==-1){
					perror("Não foi possível ecsrever.");
				}

				execvp(prog,cmd);

				_exit(0);
				close(fd[1]);
			}
				
			else{
				wait(NULL);
				close(fd[1]);
				
				pid_t filho;
				long int tempo_f;
				tempo_f = gettime();
				int tam = sizeof(tempo_f)+17;
				char info_pai[tam];

				int re = read(fd[0],&filho,sizeof(pid_t));

				if(re==-1){
					perror("Não foi possível ler.");
				}
				sprintf(info_pai, "p %d %ld\n",filho,tempo_f); 
				int we2 = write(pipe1,&info_pai,tam); //envia info_pai para o servidor
				
				if(we2==-1){
					perror("Não foi possível escrever.");
				}
				
				int pipe2 = open("monitor_escrever",O_RDONLY);//ver isto
				char ms[12];
				int bytesr=read(pipe2,&ms,12);
				
				if(bytesr==-1){
					perror("Não foi possível ler.");
				}

				if(bytesr>0){
					printf("Demorou %s ms\n",ms);
				}

				close(fd[0]);
				close(pipe2);

				
			}
		}
	}

	if(strcmp(argv[1],"kill")==0){
		char kill[2]="k\n";
		write(pipe1,&kill,2);
	}

	if(strcmp(argv[1],"status")==0){

		long int agora;
		agora = gettime();

		int tam = sizeof(agora)+8;
		char info_s[tam];

		sprintf(info_s,"s %ld\n",agora); 
		write(pipe1,&info_s,tam);
		
		int pipe2 = open("monitor_escrever",O_RDONLY);

		char bufstatus[MAXBUFFER];
		int bytesr;
		
		while((bytesr=read(pipe2,&bufstatus,MAXBUFFER))>0){
			write(1,&bufstatus,bytesr);

		}
		close(pipe2);
	}

	close(pipe1);
	return 0;
}