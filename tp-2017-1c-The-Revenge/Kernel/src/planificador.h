/*
 * planificador.h
 *
 *  Created on: 23/6/2017
 *      Author: utnso
 */

#ifndef SRC_PLANIFICADOR_H_
#define SRC_PLANIFICADOR_H_

#include <Sockets/SocketsCliente.h>
#include <Sockets/socket.h>
#include <Sockets/StructsUtils.h>

#include "Kernel.h"
#include "consolaKernel.h"

int crearHiloPlanificacion(int socketMemoria);

void algoritmo();

void planificarBloqueados();

void planificarNuevos(int socketMemoria);

void replanificarQuantum();

void planificarASerEliminados();

int planificar(int socketMemoria);

void recorrerYMandar(t_cpu *cpuLista);

void envioPCBACpuDisponible(int socketCPU, t_PCB * unPCB);

void terminacionIrregular(t_PCB* pcb, uint32_t exitcode);

void pcbAColaExit(uint32_t pcbId);

t_aSerEliminado * procesoASerAbortado(t_PCB* pcb);

void memoryLeaks(t_PCB* pcb);

#endif /* SRC_PLANIFICADOR_H_ */

