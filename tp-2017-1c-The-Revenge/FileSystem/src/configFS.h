/*
 * configFS.h
 *
 *  Created on: 14/4/2017
 *      Author: utnso
 */

#ifndef CONFIGFS_H_
#define CONFIGFS_H_

#include <commons/config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <commons/log.h>

void verificarParametros(int cantArg);

char* leerParametros(char** vectorArg);

archivoConfig* leerArchivoConfiguracion(char* ruta);
archivoMetadata* leerArchivoMetadata(char* ruta);

#endif /* CONFIGFS_H_ */
