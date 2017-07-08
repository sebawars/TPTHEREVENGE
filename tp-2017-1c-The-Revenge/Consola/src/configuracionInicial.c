/*
 * configuracionInicial.c
 *
 *  Created on: 7/4/2017
 *      Author: utnso
 */
#include "configuracionInicial.h"

void verificarParametros(int cantArg)// ./Consola.c(1) /ruta del archivo de config(2)
{
	if(cantArg<2){
		printf("\n\nfaltan Parametros\n");
		exit(1);
	}
	if(cantArg>2){
		printf("\n\ncantidad de parametros incorrecta\n");
		exit(1);
	}

}
char* leerParametros(char** vectorArg)
{
	char* ruta = strdup(vectorArg[1]);
	printf(" esta es la ruta %s\n\n",ruta);
	return ruta;
}

archivoConfig leerArchivoConfiguracion(char* ruta)
{
	archivoConfig arch;
	t_config * config;
	char*auxiliar;
	config=config_create(ruta);
	if(config ==NULL) printf("no se puede encontrar el archivo de configuración");

	if(config_has_property(config,"IP_KERNEL"))
	{
		auxiliar = config_get_string_value(config,"IP_KERNEL");
		arch.ip_kernel = malloc(strlen(auxiliar)+1);
		strcpy(arch.ip_kernel, auxiliar);
	}else{
		log_error(ptrLog, "El archivo de configuracion no contiene la clave IP_KERNEL");
	}

	if(config_has_property(config,"PUERTO_KERNEL"))
	{
		arch.puerto_kernel=config_get_int_value(config,"PUERTO_KERNEL");
	}else{
		log_error(ptrLog, "El archivo de configuracion no contiene la clave PUERTO_KERNEL");
	}

	if (config_has_property(config, "DETALLELOG"))
		arch.detallelog = log_level_from_string(config_get_string_value(config, "DETALLELOG"));
	else
		log_error(ptrLog,"El archivo de configuracion no contiene la clave DETALLE_LOG");

	return(arch);
}


