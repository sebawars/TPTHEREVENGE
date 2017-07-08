/*
 * configFS.c
 *
 *  Created on: 14/4/2017
 *      Author: utnso
 */

#include "FileSystem.h"
#include "configFS.h"

void verificarParametros(int cantArg)
/* por ahora es 2  "./FileSystem.c /ruta del archivo de config", cuando lo sigan
 * haciendo tengan en cuenta que ese número se modifica según lo que ustedes necesiten
 * para que funcione. Lo único que tienen que hcer es cambiar ese 2 por el número que
 * necesiten ustedes. Si agregan más parametros tienen que corregir "leerParametros()"
 */
{
	if (cantArg < 2) {
		printf("\n\nfaltan Parametros\n");
		exit(1);
	}
	if (cantArg > 2) {
		printf("\n\ncantidad de parametros incorrecta\n");
		exit(1);
	}
//	printf("vamos bien\n"); //solo para probar
}

char* leerParametros(char** vectorArg)
/*
 * Si ponen más parametros tienen que actualizar esto, es igual, para cada parametro
 * tienen que hacer lo  de strdup(), los printf están sólo para que vean que funciona
 * bien, hay que borrarlos despues
 */
{
	char* ruta = strdup(vectorArg[1]);
//	printf(" esta es la ruta %s\n\n", ruta); //solo para probar
	return ruta;
}


archivoConfig* leerArchivoConfiguracion(char* ruta) {
	archivoConfig* arch = malloc(sizeof(archivoConfig));
	t_config* config;
	config = config_create(ruta);

	if (config == NULL)
		printf("no se puede encontrar el archivo de configuración");

	if (config_has_property(config, "PUERTO"))
		arch->PUERTO = config_get_int_value(config, "PUERTO");
	else
		log_error(logFile,
				"El archivo de configuracion no contiene la clave PUERTO");
	//printf("el contenido del puerto es %i\n", arch.PUERTO);//solo para probar

	if (config_has_property(config, "PUNTO_MONTAJE")) {
		arch->PUNTO_MONTAJE = malloc(
				strlen(config_get_string_value(config, "PUNTO_MONTAJE")) + 1);
		arch->PUNTO_MONTAJE = config_get_string_value(config, "PUNTO_MONTAJE");
	} else
		log_error(logFile,
				"El archivo de configuracion no contiene la clave PUNTO_MONTAJE");
	//printf("el contenido del puerto es %s\n", arch.PUNTO_MONTAJE);//solo para probar

	return arch;
}
archivoMetadata* leerArchivoMetadata(char* rutaAux) {
	char* ruta=string_duplicate(rutaAux);
	archivoMetadata* arch = malloc(sizeof(archivoMetadata));
	t_config* configMetadata=malloc(sizeof(t_config));
	string_append(&ruta, "Metadata/Metadata.bin");
	configMetadata = config_create(ruta);
	if (configMetadata == NULL)
		printf("no se puede encontrar el archivo de configuración");
//////////////////////////////////
	if (config_has_property(configMetadata, "CANTIDAD_BLOQUES"))
		arch->CANT_BLOQUES= config_get_int_value(configMetadata, "CANTIDAD_BLOQUES");
	else
		log_error(logFile,
				"El archivo de configuracion no contiene la clave CANTIDAD_BLOQUES");
//////////////////////////////////
	if (config_has_property(configMetadata, "TAMANIO_BLOQUES"))
			arch->TAM_BLOQUES= config_get_int_value(configMetadata, "TAMANIO_BLOQUES");
		else
			log_error(logFile,"El archivo de configuracion no contiene la clave CANTIDAD_BLOQUES");
//////////////////////////////////
	if (config_has_property(configMetadata, "MAGIC_NUMBER")) {
		arch->MAGIC_NUMBER = malloc(strlen(config_get_string_value(configMetadata, "MAGIC_NUMBER")) + 1);
		arch->MAGIC_NUMBER = config_get_string_value(configMetadata, "MAGIC_NUMBER");
	} else
		log_error(logFile,"El archivo de configuracion no contiene la clave MAGIC_NUMBER");
	//printf("el contenido del puerto es %s\n", arch.PUNTO_MONTAJE);//solo para probar
	free (ruta);
	return arch;
}
