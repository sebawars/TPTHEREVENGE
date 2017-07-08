/*
 * conexiones.h
 *
 *  Created on: 28/5/2017
 *      Author: utnso
 */

#ifndef SRC_CONEXIONES_H_
#define SRC_CONEXIONES_H_

#include "Kernel.h"

int demasiadosClientes(int socket);

void agregarCPULibre(sem_t *mutexCPU, t_list *listaCPU, int sockFd, sem_t *CPULibres);

int atiendeCpu(int socket, uint32_t tamStack);

void borrarCPU(int socketCPU);

int esFDCPU(int sockFd);

int esFDConsola(int sockFd);

void borrarConsola(int socketConsola);

void operacionPCB(int i, header_t header, uint32_t exitcode);

int tieneElPidParaColas(t_queue* cola, int pid);

t_PCB* queue_get(t_queue* cola, int pid);

int sockets_select(archivoConfigKernel *configuracion, int socketMemoria);

char *recibirProgramaAnsisop(int socketConsola,uint32_t length);

int enviarPid(int socket,int pid, int tipo);

int recvPid(int socket, uint32_t length);

int aceptarConexionConsola(int sockfdProgramas);

t_PCB * recibirPCB(int socketCPU);

void resetarCPU(int socketCPU);

int recibirMensajeMemoria(int socket);

void recibirTamanio(int socket, uint32_t tamanio, uint32_t * elemento);

int pedirPaginaAMemoria(int pid, int cant, int socketMemoria);

int enviarCodigoAMemoria(int socketMemoria , int pid , int paginasCodigo, char * codigo);

int enviarFinalizarProgramaAMemoria(int socket, uint32_t pid);

#endif /* SRC_CONEXIONES_H_ */
