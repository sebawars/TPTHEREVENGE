/*
 * configCpu.h
 *
 *  Created on: 11/4/2017
 *      Author: utnso
 */

#ifndef SRC_CONFIGCPU_H_
#define SRC_CONFIGCPU_H_

#include "CPU.h"

void verificarParametros(int cantArg);
char* leerParametros(char** vectorArg);
archivoConfigCPU *leerArchivoConfiguracion(char* ruta);

#endif /* SRC_CONFIGCPU_H_ */
