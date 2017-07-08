/*
 * configMemoria.h
 *
 *  Created on: 6/4/2017
 *      Author: utnso
 */

#ifndef HEADERS_CONFIGMEMORIA_H_
#define HEADERS_CONFIGMEMORIA_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <commons/config.h>
#include "Memoria.h"

/****************************************************
			FUNCIONES DE LECTURA DE ARCHIVOS
****************************************************/

int sizeofString(char* cadena);

char* getRutaMemoria(parametrosMemoria parametros);


metadataMemoria leerMetadataMemoria(parametrosMemoria parametros);

//****************************************************************************************************************


parametrosMemoria leerParametrosMemoria(char** argv);

//****************************************************************************************************************

void verificarParametros(int argc);


#endif /* HEADERS_CONFIGMEMORIA_H_ */
