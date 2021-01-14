/*
Implementada la parte de asignación estática de recursos
Implementada la parte de asignación dinámica de recursos
	SIGTERM para aumentar el espacio en la cola de pacientes
	SIGALRM para aumentar el número de enfermeros senior
*/

#include <pthread.h>
#include<sys/wait.h>
#include<unistd.h>
#include<time.h>
#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>

#define MAXPAC 15
#define MAXENF 3

//Estructuras necesarias para la correcta ejecución del programa
struct paciente{
	int id; 
	int grupo; //grupo sanguíneo
	bool alergia; //reacción alergica
	bool vacunado; //está vacunado
	bool seroprevalencia; 
	pthread_t victima; //Crea un hilo
};

struct enfermero{
	int id;
	int grupoEdad;
	int nPacientes; //cuantos pacientes vacuna
	bool descansando; //está en el descanso
	pthread_t fermero; //Crea un hilo
};

struct medico{
	bool ocupado; //está ocupado
	pthread_t medic;
};

struct estadistico{
	bool ocupado; //está ocupado
	pthread_t stat; //Crea un hilo
};

struct paciente* cola; //Crea una cola de pacientes
struct enfermero* ats; //Crea una cola de Enfermeros
struct medico galeno; //Crea un medico
struct estadistico numerologo; //por definir

//Para abrir un archivo
char logFileName[] = "log.txt";
FILE * logFile;


//Funciones
void iniciaSenhales();
void iniciaMutex();
void iniciaHilos();
void iniciaEnfermeros();
void iniciaCola();
int comprobarHueco();
int buscarHueco();
void aumentaCola(int s);
void aumentaEnfermeros(int s);
void nuevoPaciente(int s);
void finaliza(int s);
void *handlerMedico(void *identificador);
void *handlerStat(void *identificador);
void *handlerPaciente(void *identificador);
void *handlerEnfermero(void *identificador);
void writeLogMessage(char *id , char *msg);


//variables globales
bool ejecucion; 
int maxP;
int maxE;
char interlocutor[256]; //
char mensaje[256]; //
int enfermitos = 0; //Número de enfermitos

//mutex
pthread_mutex_t mEscritura; //Para bloquear la escritura de datos
pthread_mutex_t mCola; //Para bloquear la cola y poder intercambiar datos


int main(int argc, char *argv[]){
	maxP = MAXPAC; //Está definido como 15 
	maxE = MAXENF; //Está definido como 3
	
	ejecucion = true; 
	
	//Asignacion estática de recursos
	if(argc>1){
		printf("Asignación estática de recursos I\n");
		maxP = atoi(argv[1]); //Parsea los pacientes
		if(argc==3){
			printf("Asignacion estática de recursos II\n");
			maxE = atoi(argv[2]); //Parsea los enfermeros
		}
	}
	printf("Para pacientes menores de 16 años introduzca kill -SIGUSR1 %d\n", getpid()); //Enviar señal sigusr1 al propio programa
	printf("Para pacientes de rango de edad media introduzca kill -SIGUSR2 %d\n", getpid()); //Enviar señal sigusr1 al propio programa
	printf("Para pacientes mayores de 60 años introduzca kill -SIGPIPE %d\n", getpid()); //Enviar señal sigusr1 al propio programa
	logFile = fopen(logFileName, "w"); //Abre el archivo en modo escritura
	fclose(logFile); //Lo cierra
	
	//Llamamos a la función que escribe en el registro
	writeLogMessage("Hospital nacional estación término ", "Abiertas las puertas, pueden pasar, sean bienvenidos\n"); 
	
	cola = (struct paciente*)malloc(maxP * sizeof(struct paciente));
	ats = (struct enfermero*)malloc(maxE * sizeof(struct enfermero));
	
	iniciaSenhales();
	iniciaMutex();
	iniciaHilos();
	iniciaEnfermeros();
	
	while(ejecucion){
		if(ejecucion==true) pause();
	}
}

void iniciaSenhales(){
	struct sigaction sigPac;
	sigPac.sa_handler= nuevoPaciente;
	sigemptyset(&sigPac.sa_mask);
	sigPac.sa_flags = 0;
	if(sigaction (SIGUSR1, &sigPac, NULL)!=0) exit(0);
	if(sigaction (SIGUSR2, &sigPac, NULL)!=0) exit(0);
	if(sigaction (SIGPIPE, &sigPac, NULL)!=0) exit(0);
	
	struct sigaction sigEnd;
	sigemptyset(&sigEnd.sa_mask);
	sigEnd.sa_flags = 0;
	sigEnd.sa_handler= finaliza;
	if(sigaction (SIGINT, &sigEnd, NULL)!=0) exit(0);
	
	
	struct sigaction sigCol;
	sigemptyset(&sigCol.sa_mask);
	sigCol.sa_flags = 0;
	sigCol.sa_handler= aumentaCola;
	if(sigaction (SIGTERM, &sigCol, NULL)!=0) exit(0);
	
	struct sigaction sigEnf;
	sigemptyset(&sigEnf.sa_mask);
	sigEnf.sa_flags = 0;
	sigEnf.sa_handler= aumentaEnfermeros;
	if(sigaction (SIGALRM, &sigEnf, NULL)!=0) exit(0);
	
}

void iniciaMutex(){
	pthread_mutex_init(&mEscritura, NULL);
	pthread_mutex_init(&mCola, NULL);
}

void iniciaHilos(){
	int id = 0;
	pthread_create(&galeno.medic, NULL, handlerMedico, &id);
	pthread_create(&numerologo.stat, NULL, handlerStat, &id);
}

void iniciaEnfermeros(){
	int i;
	for(i=0; i<maxE; i++){
		int grupo=3;
		if(i==0) grupo= 1;
		else if(i==1) grupo= 2;
		ats[i].id = i;
		ats[i].grupoEdad= grupo;
		ats[i].nPacientes= 0;
		ats[i].descansando= false;
		pthread_create(&ats[i].fermero, NULL, handlerEnfermero, &ats[i].id);
	}
}

void iniciaCola(){
	int i;
	for(i=0; i<maxP; i++){
		cola[i].id= 0;
		cola[i].grupo= 0;
		cola[i].alergia= false;
		cola[i].vacunado= false;
		cola[i].seroprevalencia= false;
	}
}

int comprobarHueco(){
	int i;
	for(i=0; i<maxP; i++){
		if(cola[i].id==0) return i;
	}
	return -1;
}

int buscarHueco(){
    return 0;
}
void aumentaCola(int s){
	maxP++;
	cola = (struct paciente*)realloc(cola, maxP);
	printf("Megafonia: Debido al exceso de pacientes, la cola de entrada para vacunación ahora es de %d pacientes\n", maxP);
	pthread_mutex_lock(&mEscritura);
		sprintf(mensaje, "Debido al exceso de pacientes, la cola de entrada para vacunación ahora es de %d pacientes\n", maxP);
		writeLogMessage("Megafonía", mensaje);
	pthread_mutex_unlock(&mEscritura);
	
}

void aumentaEnfermeros(int s){
	maxE++;
	ats = (struct enfermero*)realloc(ats, maxE);
	printf("Megafonía: Cómo nuestros enfermeros están saturados por un exceso de pacientes mayores, ahora atenderán %d enfermeros\n", maxE);
	pthread_mutex_lock(&mEscritura);
		sprintf(mensaje, "Cómo nuestros enfermeros están saturados por un exceso de pacientes mayores, ahora atenderán %d enfermeros\n", maxE);
		writeLogMessage("Megafonía", mensaje);
	pthread_mutex_unlock(&mEscritura);
	
	ats[maxE-1].id = maxE-1;
	ats[maxE-1].grupoEdad= 3;
	ats[maxE-1].nPacientes= 0;
	ats[maxE-1].descansando= false;
	pthread_create(&ats[maxE-1].fermero, NULL, handlerEnfermero, &ats[maxE-1].id);
}

void nuevoPaciente(int s){
	int posicion;
	int grupo;
	switch(s){
		case SIGUSR1:
		    sprintf(mensaje, "¡Hala cuánta gente! vamos a ver que reparten\n", maxP);
			grupo=0;
			break;
		case SIGUSR2: 
			grupo=1;
			break;
		case SIGPIPE:
			grupo=2;
			break;
	}
	
	enfermitos++;
	pthread_mutex_lock(&mCola);
		pthread_mutex_lock(&mEscritura);
			sprintf(interlocutor, "enfermito_%d", enfermitos);
			sprintf(mensaje, "¡Hala cuánta gente! vamos a ver que reparten\n", maxP);
			writeLogMessage(interlocutor, mensaje);
		pthread_mutex_unlock(&mEscritura);
		
		posicion = buscarHueco();
		if(posicion>=0){
			cola[posicion].id= enfermitos;
			cola[posicion].grupo= grupo;
			cola[posicion].alergia= false;
			cola[posicion].vacunado= false;
			cola[posicion].seroprevalencia= false;
			pthread_mutex_lock(&mEscritura);
				sprintf(mensaje, "¡Uy que bien una vacuna gratis, con lo necesitado que iba!\n", maxP);
				writeLogMessage("interlocutor", "mensaje");
			pthread_mutex_unlock(&mEscritura);
			pthread_create(&cola[posicion].victima, NULL, handlerPaciente, &ats[posicion].id);
		}else{
			pthread_mutex_lock(&mEscritura);
				sprintf(mensaje, "¡Serán... que no hay sitio, que no hay sitio, pues no se tarda tanto en vacunar!\n", maxP);
				writeLogMessage("interlocutor", "mensaje");
			pthread_mutex_unlock(&mEscritura);
		}
	pthread_mutex_unlock(&mCola);
	
}

void finaliza(int s){
	ejecucion = false;
	printf("Megafonía: Desde este momento el acceso a las colas de vacunación, queda cerrada, será un placer saludar mañana a los supervivientes\n");	
	pthread_mutex_lock(&mEscritura);
		sprintf(mensaje, "Desde este momento el acceso a las colas de vacunación, queda cerrada, será un placer saludar mañana a los supervivientes\n");
		writeLogMessage("Megafonía", mensaje);
	pthread_mutex_unlock(&mEscritura);
}

void *handlerMedico(void *identificador){
	int *id = (int*) identificador;
	printf("Médico: Gracias por sus aplausos, las estrella acaba de entrar en la sala, gracias, gracias\n");
	pthread_mutex_lock(&mEscritura);
		sprintf(mensaje, "Gracias por sus aplausos, las estrella acaba de entrar en la sala, gracias, gracias\n");
		writeLogMessage("Médico", mensaje);
	pthread_mutex_unlock(&mEscritura);
}

void *handlerStat(void *identificador){
	int *id = (int*) identificador;
	printf("Estadístico: Datos, datos, hmmmmmm qué ricos los datos, sobre todo después de la venta\n");
	pthread_mutex_lock(&mEscritura);
		sprintf(mensaje, "Datos, datos, hmmmmmm qué ricos los datos, sobre todo después de la venta\n");
		writeLogMessage("Estadístico", mensaje);
	pthread_mutex_unlock(&mEscritura);
}

void *handlerPaciente(void *identificador){
    
}

void *handlerEnfermero(void *identificador){
	int id = *(int*)identificador;//coincide con la posicion en el puntero
	int grupo = ats[id].grupoEdad;
	char g[256];
	switch (grupo){
		case 1:
			sprintf(g, "pacientes menores de 16 años consulta izquierda");
			break;
		case 2:
			sprintf(g, "pacientes mayores de 16 y menores de 60 años consulta central");
			break;
		case 3:
			sprintf(g, "pacientes mayores de 60 años consulta derecha");
			break;
	}
	printf("Enfermero %d: Empecemos el día, ¡atención!, %s\n", (id+1), g);
	pthread_mutex_lock(&mEscritura);
		sprintf(mensaje, "Empecemos el día, ¡atención!, %s\n", g);
		sprintf(interlocutor, "Enfermero %d", (id+1));
		writeLogMessage(interlocutor, mensaje);
	pthread_mutex_unlock(&mEscritura);
}

 void writeLogMessage ( char * id , char * msg ) {
 // Calculamos la hora actual
 time_t now = time (0) ;
 struct tm * tlocal = localtime (& now );
 char stnow [25];
 strftime ( stnow , 25 , " %d/ %m/ %y %H: %M: %S", tlocal );

 // Escribimos en el log
 logFile = fopen ( logFileName , "a");
 fprintf ( logFile , "[ %s] %s: %s\n", stnow , id , msg ) ;
 fclose ( logFile );
 }

