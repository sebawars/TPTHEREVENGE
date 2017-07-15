/*
 * configMemoria.c
 *
 *  Created on: 14/4/2017
 *      Author: utnso
 */



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <commons/config.h>
#include "configMemoria.h"

/****************************************************
			FUNCIONES DE LECTURA DE ARCHIVOS
****************************************************/

int sizeofString(char* cadena)
{
	int size = 0;
	size = sizeof(char)*strlen(cadena);
	return size;
}

char* getRutaMemoria(parametrosMemoria parametros)//para qué sirve?
{
	char *ruta;

	int tamPathKernel = sizeofString(parametros.dirMetadata);

	int tamCadena = tamPathKernel + 16 + 1;

	ruta = malloc(tamCadena);

	if(ruta==NULL)
	{
		printf("Error - Funcion: %s - Linea: %d - malloc == NULL",__func__,__LINE__);
		exit(1);
	}

	sprintf(ruta,"%s/Memoria/metadata",parametros.dirMetadata);

	return ruta;
}

metadataMemoria leerMetadataMemoria(parametrosMemoria parametros)
{
	metadataMemoria mdata;
	t_config* config; //Estructura

	char* ruta;
	//ruta = getRutaMemoria(parametros);  esto rompe
	ruta= parametros.dirMetadata;    //esto funciona

	config = config_create(ruta);

	if(config==0)
	{
		printf("Error: Funcion: %s - Linea: %d - metadata no encontrado \n",__func__,__LINE__);
		printf("%s\n",ruta);
		exit(20);
	}

	if(config_has_property(config,"PUERTO"))
		mdata.puerto = config_get_int_value(config,"PUERTO");
	if(config_has_property(config,"BACKLOG"))
		mdata.backlog = config_get_int_value(config,"BACKLOG");
	if(config_has_property(config,"MARCOS"))
		mdata.marcos = config_get_int_value(config,"MARCOS");
	if(config_has_property(config,"MARCO_SIZE"))
		mdata.marcos_size =  config_get_int_value(config,"MARCO_SIZE");
	if(config_has_property(config,"ENTRADAS_CACHE"))
		mdata.entradas_cache = config_get_int_value(config,"ENTRADAS_CACHE");
	if(config_has_property(config,"CACHE_X_PROC"))
		mdata.cache_x_proceso = config_get_int_value(config,"CACHE_X_PROC");
	if(config_has_property(config,"REEMPLAZO_CACHE"))
	{
		mdata.reemplazo_cache = malloc(5*sizeof(char));
		strcpy(mdata.reemplazo_cache,config_get_string_value(config,"REEMPLAZO_CACHE"));
		mdata.reemplazo_cache[5] = '\0';
	}
	if(config_has_property(config,"RETARDO_MEMORIA"))
		mdata.retardo_memoria = config_get_int_value(config,"RETARDO_MEMORIA");
	if (config_has_property(config, "DETALLELOG"))
		mdata.detallelog = log_level_from_string(config_get_string_value(config, "DETALLELOG"));

	config_destroy(config);

	free(ruta);
	return mdata;
}
//****************************************************************************************************************


parametrosMemoria leerParametrosMemoria(char** argv)
{
	parametrosMemoria parametros;
	parametros.dirMetadata = strdup(argv[1]);

	return parametros;
}
//****************************************************************************************************************

void verificarParametros(int argc)
{
	if(argc<2){
			printf("\n\nfaltan Parametros\n");
			exit(1);
		}
		if(argc>2){
			printf("\n\ncantidad de parametros incorrecta\n");
			exit(1);
		}
		printf("vamos bien\n");//solo para probar
}

