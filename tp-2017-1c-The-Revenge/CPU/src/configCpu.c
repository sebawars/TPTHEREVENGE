/*
 * configCpu.c
 *
 *  Created on: 11/4/2017
 *      Author: utnso
 */

#include "configCpu.h"


void verificarParametros(int cantArg)
{
	if(cantArg<2){
		printf("\n\nfaltan Parametros\n");
		exit(1);
	}
	if(cantArg>2){
		printf("\n\ncantidad de parametros incorrecta\n");
		exit(1);
	}
	printf("Cantidad de parámetros correcta\n");
}
char* leerParametros(char** vectorArg)
{
	char* ruta = strdup(vectorArg[1]);
	printf("%s es la ruta \n\n",ruta);
	return ruta;
}

archivoConfigCPU *leerArchivoConfiguracion(char* ruta)
{
	archivoConfigCPU *arch=malloc(sizeof(archivoConfigCPU));
	t_config * config;
	config=config_create(ruta);

	if(config != NULL)
	{
		if (config_has_property(config, "PUERTO"))
			arch->puerto = config_get_int_value(config, "PUERTO");
		else
			log_error(ptrLog, "El archivo de configuracion no contiene la clave PUERTO");

		if (config_has_property(config, "IP_KERNEL"))
			arch->ip_kernel = config_get_string_value(config, "IP_KERNEL");
		else
			log_error(ptrLog, "El archivo de configuracion no contiene la clave IP_KERNEL");

		if (config_has_property(config, "PUERTO_KERNEL"))
			arch->puerto_kernel = config_get_int_value(config, "PUERTO_KERNEL");
		else
			log_error(ptrLog, "El archivo de configuracion no contiene la clave PUERTO_KERNEL");

		if (config_has_property(config, "IP_MEMORIA"))
			arch->ip_memoria = config_get_string_value(config, "IP_MEMORIA");
		else
			log_error(ptrLog, "El archivo de configuracion no contiene la clave IP_MEMORIA");

		if (config_has_property(config, "PUERTO_MEMORIA"))
			arch->puerto_memoria = config_get_int_value(config, "PUERTO_MEMORIA");
		else log_error(ptrLog, "El archivo de configuracion no contiene la clave PUERTO_MEMORIA");

		if (config_has_property(config, "DETALLELOG"))
			arch->detallelog = log_level_from_string(config_get_string_value(config, "DETALLELOG"));
		else
			log_error(ptrLog,"El archivo de configuracion no contiene la clave DETALLE_LOG");

	}else {
		printf("no se puede encontrar el archivo de configuración\n");
		}

	return arch;
}
