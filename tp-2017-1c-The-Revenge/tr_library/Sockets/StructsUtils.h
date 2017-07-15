/*
 * StructsUtils.h
 *
 *  Created on: 5/5/2017
 *      Author: utnso
 */

#ifndef SOCKETS_STRUCTSUTILS_H_
#define SOCKETS_STRUCTSUTILS_H_

#include<stdint.h>
#include<commons/collections/list.h>
#include<commons/collections/queue.h>

typedef struct {
	int8_t type;
	uint32_t length;
}__attribute__((__packed__)) header_t;

typedef struct {
	char *Script;
	uint32_t PID;
} t_datos_programa;

typedef struct {
	uint32_t pid;
	uint32_t cantidadPaginas;
}__attribute__((__packed__)) t_solicitudPaginas;

typedef struct {
	uint32_t pagina;
	uint32_t inicio;
	uint32_t offset;
	uint32_t pid;
}__attribute__((__packed__)) t_solicitudLectura;

typedef struct {
	uint32_t pagina;
	uint32_t offset;
	uint32_t tamanio;
	uint32_t pid;
	char* buffer;
}__attribute__((__packed__)) t_solicitudEscritura;

typedef struct {
	uint32_t pid;
	uint32_t puntero;
}__attribute__((__packed__)) t_solicitudHeap;

//auxiliares
typedef struct {
	uint32_t tamanioStack;
	void * stack;
} t_tamanio_stack_stack;

typedef struct {
	uint32_t inicio;
	uint32_t longitud;
}__attribute__((__packed__)) t_indice_codigo;

typedef struct {
	uint32_t pagina;
	uint32_t offset;
	uint32_t size;
} t_argumento;

typedef struct {
	t_list* argumentos;
	t_list* variables;
	uint32_t direccion_retorno;
	t_argumento * retVar;
} t_stack;

typedef struct {
	uint32_t pagina;
	uint32_t offset;
	uint32_t size;
} t_posicion_stack;

typedef struct {
	uint32_t pagina;
	uint32_t offset;
	uint32_t size;
	char idVariable;
} t_variable;

typedef struct {
	char*nombre;
	uint32_t valor;
}__attribute__((__packed__)) t_op_varCompartida;

typedef struct {
	uint32_t size;
	bool isFree;
} t_heapMetadata;

typedef struct{
	t_heapMetadata* heapMetadata;
	uint32_t numeroBloque;
	uint32_t offset;
	char* data;
}t_bloque;

typedef struct{
	int numero_pagina;
	int tam_disponible;
	t_list* bloques;
}t_pagina_heap;

typedef struct {
	int pid;
	t_list* pagina; //lista de t_pagina_heap
	t_list* paginasLiberadas;
} t_tabla_heap;

typedef struct {
	uint32_t PID; // process id
	uint32_t estado;
	uint32_t PC; //program counter
	t_list *IC; //Indice de Codigo
	t_list* indiceStack;
	char* indice_etiquetas;
	t_tabla_heap* tablaHeap;
	int32_t EC; //exit code
	uint32_t socket;//es necesario saber de qué socket viene para poder finalizarlo
	t_queue* punteroColaPlanif;
	uint32_t quantum; //sera la referencia para que el CPU ejecute
	uint32_t quantum_sleep; //Este parametro puede modificarse en tiempo de ejecucion
	uint32_t paginaCodigoActual;
	uint32_t stackPointer; //stack pointer
	uint32_t paginaStackActual;
	uint32_t tamanioEtiquetas;
	uint32_t indiceContextoEjecucionActualStack;
	uint32_t primerPaginaStack;
	uint32_t rafagas; //lo actualiza cpu
	bool abortarEjecucion;
}__attribute__((__packed__)) t_PCB;

enum t_algoritmo {
	FIFO, RR
};

typedef struct{
		bool lectura;
		bool escritura;
		bool creacion;
} t_flags;

typedef struct {
	uint32_t pid;
	uint32_t descriptor;
	uint32_t tamanio;
	t_flags flags;
	char* informacion;
}__attribute__((__packed__)) t_operacionDescriptorArchivo;

typedef struct {
	int32_t programID;
}__attribute__((__packed__)) t_cambio_proc_activo;

typedef struct {
	char* path;
	uint8_t resultado;
}__attribute__((__packed__)) t_validarArchivo;

typedef t_validarArchivo t_crearArchivo;

typedef t_validarArchivo t_borrarAchivo;

typedef t_validarArchivo t_resultado;

typedef struct {
	char* path;
	int32_t tamPath;
	int16_t offset;
	int16_t size;
	char* buffer;
	int32_t tamBuffer;
}__attribute__((__packed__)) t_guardarDatos;

typedef struct {
	int32_t tamPath;
	char* path;
	int16_t offset;
	int16_t size;
}__attribute__((__packed__)) t_obtenerDatos;

enum
{
	EXITO = 1,
	NO_EXISTE = 2,
	EXISTE = 3,
	NO_HAY_ESPACIO = 4,
	ARGUMENTO_INVALIDO = 5,
	ERROR_CREACION=6,
	ERROR_BORRAR=7,
}t_results;

typedef struct{
	int tamanio;
	char** bloques;
}t_infoArchivo;

#endif /* SOCKETS_STRUCTSUTILS_H_ */


