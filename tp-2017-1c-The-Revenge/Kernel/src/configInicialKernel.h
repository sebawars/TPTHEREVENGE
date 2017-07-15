/*
 * configInicialKernel.h
 *
 *  Created on: 11/4/2017
 *      Author: utnso
 */

#ifndef SRC_CONFIGINICIALKERNEL_H_
#define SRC_CONFIGINICIALKERNEL_H_


#include "Kernel.h"

void verificarParametros(int cantArg);
char* leerParametros(char** vectorArg);
archivoConfigKernel *leerArchivoConfiguracion(char* ruta);


#endif /* SRC_CONFIGINICIALKERNEL_H_ */
