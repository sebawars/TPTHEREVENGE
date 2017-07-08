/*
 * CPU.h
 *
 *  Created on: 7/4/2017
 *      Author: utnso
 */

#ifndef SRC_CPU_H_
#define SRC_CPU_H_

#define LOG_PATH "log.txt"
#define TAMANIO_VARIABLE 4

#include <commons/collections/list.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/config.h>
#include <Sockets/socket.h>
#include <signal.h>

typedef struct{
	char* ip_kernel;
	int puerto_kernel;
	char* ip_memoria;
	int puerto_memoria;
	int puerto;
	int detallelog;
}archivoConfigCPU;

t_PCB* pcb;
t_log* ptrLog;
t_config* config;
extern bool cerrarCPU;
extern bool huboStackOver;
int socketMemoria,socketKernel;
uint32_t iTamStack,tamanioPagina;


void recibirTamanio(int socket, uint32_t tamanio, int32_t * elemento);

int recibirMensaje(socket);
int recibirMensajeInicialDeMemoria(socket);
void recibirPCB(int socket,uint32_t tamanio);
void setPCB(t_PCB * pcbDeCPU);

void comenzarEjecucionDePrograma();
char * solicitarProximaInstruccionAMemoria();
char * recibirInstruccion(socket);
int controlarConexiones(void);
void finalizarEjecucion(int8_t type);
void finalizarEjecucionPorQuantum();

void limpiarInstruccion(char * instruccion);
void revisarSigusR1(int signo);
void revisarfinalizarCPU();

#endif /* SRC_CPU_H_ */
