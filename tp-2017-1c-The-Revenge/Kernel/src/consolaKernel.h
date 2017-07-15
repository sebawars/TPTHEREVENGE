/*
 * consolaKernel.h
 *
 *  Created on: 23/6/2017
 *      Author: utnso
 */

#ifndef SRC_CONSOLAKERNEL_H_
#define SRC_CONSOLAKERNEL_H_

#include <Sockets/SocketsCliente.h>
#include <Sockets/socket.h>
#include <Sockets/StructsUtils.h>

#include "Kernel.h"
#include "capaMemoria.h"

void consolaKernel();

void obtenerListado(int pid);

void operacionKernel(uint32_t operacion, int pid, int gradoMulti, uint32_t exitcode);

int crearHiloConsolaKernel();

void archivosAbiertosPorProceso(uint32_t pid);

#endif /* SRC_CONSOLAKERNEL_H_ */

