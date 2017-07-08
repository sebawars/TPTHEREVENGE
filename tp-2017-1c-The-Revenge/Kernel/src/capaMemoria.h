/*
 * capaMemoria.h
 *
 *  Created on: 23/6/2017
 *      Author: utnso
 */

#ifndef SRC_CAPAMEMORIA_H_
#define SRC_CAPAMEMORIA_H_

#include <Sockets/SocketsCliente.h>
#include <Sockets/socket.h>
#include <Sockets/StructsUtils.h>

#include "Kernel.h"

typedef struct{
	int pid;
	int cantPagsHeap;
	int alocarCant;
	int alocarBytes;
	int liberarCant;
	int liberarBytes;
}t_cantPagsHeap;

t_list* tablaHeap;

int superaMaximoDisponible(int valorNumerico, int socketCPU);

int superaMaximoAsignacion(int pid, int socketMemoria, int socketCPU);

uint32_t crearPaginaHeap(int valorNumerico,int socketMemoria, int socketCPU, t_PCB* pcb);

bool tienePaginaConTamanioDisponibleSuficiente(t_list* tabla, int valorNumerico);

bool tienePaginaALiberar(t_list* paginas, int numPagina);

bool tieneBloqueFreeConTamanioSuficiente(t_pagina_heap *pagina, int valorNumerico);

bool tieneBloqueALiberar(t_pagina_heap *pagina, int offset);

void enviarPunteroACPU(uint32_t offsetADevolver,int socketCPU, int tamanio);

int encontrarBloquesVecinosLibres(t_list* bloques, t_bloque* bloque,t_pagina_heap* pag);

void liberarPagina(t_tabla_heap* tabla,t_pagina_heap* pagina);

void operacionesHeap(int socketCPU, int tamanio, int operacion, int socketMemoria);

void logEstadoPaginaHeap(t_pagina_heap* pagina, int pid);

void logEstadoBloque(t_bloque* bloque);

void almacenar(char* buffer,uint32_t valorNumerico,uint32_t pid);

#endif /* SRC_CAPAMEMORIA_H_ */


