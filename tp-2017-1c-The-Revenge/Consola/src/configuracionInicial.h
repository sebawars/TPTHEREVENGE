/*
 * configuracionInicial.h
 *
 *  Created on: 7/4/2017
 *      Author: utnso
 */

#ifndef SRC_CONFIGURACIONINICIAL_H_
#define SRC_CONFIGURACIONINICIAL_H_

#include "Consola.h"

/*
 * @NAME: verificarParametros
 * @DESC: controla que la cantidad de parametros pasados por línea de comando sean
 * los correctos
 */
void verificarParametros(int cantArg);

/*
 *@NAME: leerParametros
 *@DESC: transforma los strings ingresados por consola a su correspondiente dentro
 *del proceso, en este caso sólo se pasa como parametro la ruta donde esta el archivo
 *de configuracion
 */
char* leerParametros(char** vectorArg);

/*
 * @NAME: leerArchivoConfiguracion
 * @DESC: lee los parametros de su archivo de configuración y los setea en la
 * estructura correspondiente
 */
archivoConfig leerArchivoConfiguracion(char* ruta);


#endif /* SRC_CONFIGURACIONINICIAL_H_ */
