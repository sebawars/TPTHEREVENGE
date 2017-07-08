/*
 * configInicialKernel.c
 *
 *  Created on: 11/4/2017
 *      Author: utnso
 */


#include "configInicialKernel.h"


void verificarParametros(int cantArg)// ./Kernel.c(1) /ruta del archivo de config(2)
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
	//printf(" esta es la ruta %s\n\n",ruta);
	return ruta;
}

archivoConfigKernel *leerArchivoConfiguracion(char* ruta)
{
	archivoConfigKernel *arch=malloc(sizeof(archivoConfigKernel));
	t_config * config;
	config=config_create(ruta);
	if(config != NULL)
	{
		if (config_has_property(config, "PUERTO_PROG"))
			arch->PUERTO_PROG = config_get_int_value(config, "PUERTO_PROG");
		else
			log_error(logFile,"El archivo de configuracion no contiene la clave PUERTO_PROG");

		if (config_has_property(config, "PUERTO_CPU"))
			arch->PUERTO_CPU = config_get_int_value(config, "PUERTO_CPU");
		else
			log_error(logFile,"El archivo de configuracion no contiene la clave PUERTO_CPU");

		if (config_has_property(config, "PUERTO_MEMORIA"))
			arch->PUERTO_MEMORIA = config_get_int_value(config, "PUERTO_MEMORIA");
		else
			log_error(logFile,"El archivo de configuracion no contiene la clave PUERTO_MEMORIA");

		if (config_has_property(config, "IP_MEMORIA"))
			arch->IP_MEMORIA = config_get_string_value(config, "IP_MEMORIA");
		else
			log_error(logFile,"El archivo de configuracion no contiene la clave IP_MEMORIA");

		if (config_has_property(config, "IP_FS"))
			arch->IP_FS=config_get_string_value(config, "IP_FS");
		else
			log_error(logFile,"El archivo de configuracion no contiene la clave IP_FS");

		if (config_has_property(config, "PUERTO_FS"))
			arch->PUERTO_FS = config_get_int_value(config, "PUERTO_FS");
		else
			log_error(logFile,"El archivo de configuracion no contiene la clave PUERTO_FS");

		if (config_has_property(config, "QUANTUM"))
			arch->QUANTUM = config_get_int_value(config, "QUANTUM");
		else
			log_error(logFile,"El archivo de configuracion no contiene la clave QUANTUM");

		if (config_has_property(config, "QUANTUM_SLEEP"))
			arch->QUANTUM_SLEEP = config_get_int_value(config, "QUANTUM_SLEEP");
		else
			log_error(logFile,"El archivo de configuracion no contiene la clave QUANTUM_SLEEP");

		if (config_has_property(config, "ALGORITMO"))
			arch->ALGORITMO =config_get_string_value(config, "ALGORITMO");
		else
			log_error(logFile,"El archivo de configuracion no contiene la clave ALGORITMO");

		if (config_has_property(config, "GRADO_MULTIPROG"))
			arch->GRADO_MULTIPROG = config_get_int_value(config, "GRADO_MULTIPROG");
		else
			log_error(logFile,"El archivo de configuracion no contiene la clave GRADO_MULTIPROG");

		if (config_has_property(config, "SEM_IDS"))
			arch->SEM_IDS = config_get_array_value(config, "SEM_IDS");
		else
			log_error(logFile,"El archivo de configuracion no contiene la clave SEM_IDS");

		if (config_has_property(config, "SEM_INIT"))
			arch->SEM_INIT = config_get_array_value(config, "SEM_INIT");
		else
			log_error(logFile,"El archivo de configuracion no contiene la clave SEM_INIT");

		if (config_has_property(config, "STACK_SIZE"))
			arch->STACK_SIZE = config_get_int_value(config, "STACK_SIZE");
		else
			log_error(logFile,"El archivo de configuracion no contiene la clave STACK_SIZE");

		if (config_has_property(config, "IP_MEMORIA")){
			arch->IP_CPU =malloc(strlen(config_get_string_value(config, "IP_CPU"))+1);
			arch->IP_CPU = config_get_string_value(config, "IP_CPU");
		}else{
			log_error(logFile,"El archivo de configuracion no contiene la clave IP_CPU");
		}

		if (config_has_property(config, "SHARED_VARS")){
			arch->SHARED_VARS =config_get_array_value(config, "SHARED_VARS");
			crearVariablesCompartidas(arch->SHARED_VARS);
		}else{
			log_error(logFile,"El archivo de configuracion no contiene la clave SHARED_VARS");
		}

		if (config_has_property(config, "SEM_IDS")){
			arch->SEM_IDS =config_get_array_value(config, "SEM_IDS");
			if (config_has_property(config, "SEM_INIT")){
				arch->SEM_INIT =config_get_array_value(config, "SEM_INIT");
				crearSemaforos(arch->SEM_IDS,arch->SEM_INIT);
			}else{
				log_error(logFile,"El archivo de configuracion no contiene la clave SEM_INIT");
			}
		}else{
			log_error(logFile,"El archivo de configuracion no contiene la clave SEM_IDS");
		}

		if (config_has_property(config, "DETALLELOG"))
			arch->detallelog = log_level_from_string(config_get_string_value(config, "DETALLELOG"));
		else
			log_error(logFile,"El archivo de configuracion no contiene la clave DETALLE_LOG");
	}else{
		log_error(logFile,"no se puede encontrar el archivo de configuración");
	}

	return arch;
}
