/*
 * Consola.h
 *
 *  Created on: 11/4/2017
 *      Author: utnso
 */


#ifndef SRC_CONSOLA_H_
#define SRC_CONSOLA_H_

#define LOG_PATH "log.txt"
#include <pthread.h>
#include <Sockets/socket.h>
#include <Sockets/SocketsCliente.h>

#include <Sockets/StructsUtils.h>
#include <Sockets/Serializer.h>


#include <commons/collections/list.h>
#include <commons/config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <commons/log.h>
#include <commons/temporal.h>
#include <commons/string.h>
#include <commons/config.h>
#include <semaphore.h>
#include <arpa/inet.h>


#define MAX_LINEA 200 //máximo de linea ansisop a ejecutar para poder usar fgets()

pthread_t hiloU;
t_list * lista_programas;
t_log *ptrLog;
sem_t mutexCantProgramas;
sem_t mutexLista;
sem_t recibirPid;
sem_t ProxPaquete;


//Administracion Hilos
//sem_t HiloPrograma[MAX_HILOS];
pthread_mutex_t mutex;


typedef struct{
	char* ip_kernel;
	int puerto_kernel;
	int detallelog;
}archivoConfig; //de consola

typedef struct{
	int hora;
	int minuto;
	int segundo;
}tiempot;

typedef struct{
	pthread_t *hilo;
	//int idprograma;
	sem_t sem_hilo;
	sem_t finDehilo;
	int pid;
	char*ruta;
	int tamanio;
	tiempot tiempoInicial;
	tiempot tiempoFinal;
	tiempot tiempoTotal;
	int cantPrintF;
}hiloPrograma_t;


typedef struct{
	uint32_t pid;
	int operacion;
	char* buffer;
}paquete_t;

tiempot obtenerTiempo(char* tiempo);

tiempot get_tiempo_total(tiempot in,tiempot fin);

void finalizarPrograma(int pid, int operacion);

void desconectarConsola(void);

void gestionarConsola(void);

void crearHiloUsuario(void);

int getTamanioArchivo(char * ruta);

char * getContenidoArchivo(char * ruta, int size);

//int recvPid(int socket);
int recvPid(int socket, uint32_t length);

int enviarPid(int socket, int pid);

void recibirPidKernel(int socket, hiloPrograma_t *prog);

int recibirMensajeKernel(int socket);
//hacerla ^ para poder recibir mensajes genericos de pid o imprimir

void hiloPrograma(hiloPrograma_t * programa);

int enviarProgramaAnsisop(int socketFd, char* datos );

void imprimirDatosPrograma(hiloPrograma_t *prog);

void crearHiloPrograma(char *ruta);

hiloPrograma_t * obtenerHiloPrograma(uint32_t pid);

void finalizarThread(hiloPrograma_t * programa);

void liberarHilo(hiloPrograma_t * programa);

bool estaPidEnEjecion(int pid);

#endif /* SRC_CONSOLA_H_ */

