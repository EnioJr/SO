#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
 
#include <sys/types.h>
#include <sys/wait.h>


static char* args[512]; 									//ponteiro de tamanho 512
pid_t pid;													// identifica o processo pelo id
int command_pipe[2]; 
 
#define READ  0
#define WRITE 1
 
/*
 * Manipula comandos separadamente
 * input: retorna o valor do comando anterior (útil para o pipe)
 * first: 1 se o primeiro comando na sequência pipe (nenhuma entrada do pipe anterior)
 * last: 1 se o último comando na sequência de pipe (nenhuma entrada do pipe anterior)
 *
 * EXEMPLO: Se você digitar "ls | grep shell | wc" no seu shell:
 *    fd1 = command(0, 1, 0), with args[0] = "ls"
 *    fd2 = command(fd1, 0, 0), with args[0] = "grep" and args[1] = "shell"
 *    fd3 = command(fd2, 0, 1), with args[0] = "wc"
 *
 *Portanto, se 'comando' retornar um descritor de arquivo, o próximo 'comando' terá este
 *descritor como sua 'entrada'.
 */
static int command(int input, int first, int last)
{
	int pipettes[2];
 

	pipe( pipettes );											// pipe(tubo) comunicação entre os pipettes
	pid = fork();												// pid recebe id do processo  = cria um clone filho
 
	/*
	 SCHEME:
	 	STDIN --> O --> O --> O --> STDOUT
	*/
 
	if (pid == 0) {													//Execução do processo filho segue aqui 
		if (first == 1 && last == 0 && input == 0) {
			// First command
			dup2( pipettes[WRITE], STDOUT_FILENO );			//na medida em que duplica um descritor de arquivo, tornando-o alias e excluindo o descritor de arquivo antigo
		} else if (first == 0 && last == 0 && input != 0) {
			// Middle command
			dup2(input, STDIN_FILENO);
			dup2(pipettes[WRITE], STDOUT_FILENO);
		} else {
			// Last command
			dup2( input, STDIN_FILENO );
		}
 
		if (execvp( args[0], args) == -1)							//falha na execuçao do processo filho ,(exit) termina processo
			_exit(EXIT_FAILURE); // If child fails
	}
 
	// fechamento dos input
	if (input != 0) 
		close(input);
 
	// Fechamento da escrita
	close(pipettes[WRITE]);
 
	// Ultimo comando , nada mais precisa ser lido
	if (last == 1)
		close(pipettes[READ]);
 
	return pipettes[READ];
}
 

static void cleanup(int n)		// Aguarde o término dos processo FILHO e 
{								//n = número de vezes que o comando foi chamado.
	int i;
	for (i = 0; i < n; ++i)
		wait(NULL);
}
 
static int run(char* cmd, int input, int first, int last);
static char line[1024];
static int n = 0; 										//numero de parametos do 'comando' número de vezes que o comando foi chamado
 
int main()
{
	printf("SIMPLE SHELL: Type 'exit' or send EOF to exit.\n");
	while (1) {
		//Inicio da Shell
		printf("$> ");
		fflush(NULL);
 		
		if (!fgets(line, 1024, stdin)) 										// le linha de 'comando' que foi escrita
			return 0;
 
		int input = 0;
		int first = 1;
 
		char* cmd = line;													// ponteiro cmd recebe o 'comando' escrito
		char* next = strchr(cmd, '|'); 										// percore o 'comando' escrito ate achar o "|"
 
		while (next != NULL) {
			/* 'next' points to '|' */
			*next = '\0';
			input = run(cmd, input, first, 0);								//faz a leitura dos parametros do 'comando'
 
			cmd = next + 1;
			next = strchr(cmd, '|');
			first = 0;
		}
		input = run(cmd, input, first, 1);
		cleanup(n);															//AGUARDA O PROCESSO FILHO TERMINA
		n = 0;
	}
	return 0;
}
 
static void split(char* cmd);
 
static int run(char* cmd, int input, int first, int last)
{
	split(cmd);
	if (args[0] != NULL) {
		if (strcmp(args[0], "exit") == 0) 					// compara o dado do args(argumento) com exit 
			exit(0);
		/*tinha q implementar o cd AQUI
		if (strcmp(args[0], "cd") == 0){
			chdir ....
		}
		*/
		n += 1;												// incrementa n = faz a chamada do 'comando' novamente
		return command(input, first, last);
	}
	return 0;
}
 
static char* skipwhite(char* s)								//metodo que varre ate o spaço
{
	while (isspace(*s)) ++s;								//encrementa ate achar o spaço
	return s;
}
 
static void split(char* cmd)								//metodo que faz o split da linha de 'comando' e armazena em args
{
	cmd = skipwhite(cmd);
	char* next = strchr(cmd, ' ');							// percore o 'comando' escrito ate achar o " "
	int i = 0;
 
	while(next != NULL) {
		next[0] = '\0';
		args[i] = cmd;
		++i;
		cmd = skipwhite(next + 1);
		next = strchr(cmd, ' ');
	}
 
	if (cmd[0] != '\0') {
		args[i] = cmd;
		next = strchr(cmd, '\n');	
		next[0] = '\0';
		++i; 
	}
 
	args[i] = NULL;
}