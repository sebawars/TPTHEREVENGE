/*
 * Memoria.h
 *
 *  Created on: 15/4/2017
 *      Author: utnso
 */

#ifndef MEMORIA_H_
#define MEMORIA_H_

#define LOG_PATH "log.txt"

#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <math.h>
#include <Sockets/SocketsCliente.h>

typedef struct
{
	uint32_t puerto,backlog,marcos,marcos_size,entradas_cache,cache_x_proceso,retardo_memoria,detallelog;
	char* reemplazo_cache;
}metadataMemoria;

typedef struct
{
	char* dirMetadata;
}parametrosMemoria;

typedef struct
{
	uint32_t frame,pid,pagina;
}estructuraAdministrativa;

typedef struct
{
	uint32_t pid,nroPag;
	void* contenido;
}entradaCache;

typedef struct
{
	uint32_t cantidadPags,pid;
}paginasXproceso;

typedef struct
{
	bool cacheMiss;
	void* memoria;
	uint32_t tamanio;
}bytesSolicitud;

uint32_t calcularFramesParaAdministracion();

uint32_t contarEntradasPidCache(uint32_t pid); //SIN SINCRO

bool maximoDeEntradasCachePorProcesoAlcanzado(uint32_t pid); //SIN SINCRO

uint32_t obtenerIndiceEntradaLRUPorPidCache(uint32_t pid);  //SIN SINCRO

void cargarEntradasLibresCache(); //SIN SINCRO

void inicializarCache();

void inicializarEspacioMemoria(); //CON SINCRO

uint32_t funcionHash(uint32_t pid, uint32_t nroPag);  //ESTA GENERA COLISIONES, MIRAR FRAMEPIDPAGINA

uint32_t framePidPagina(uint32_t pid, uint32_t nroPagina, bool(*criterio)(uint32_t,uint32_t,uint32_t) ); // SIN SINCRO

void* insertarEntradaAdministrativa(uint32_t frame, uint32_t pid, uint32_t nroPagina); //SIN SINCRO

bool criterioBusqueda(uint32_t pid, uint32_t nroPagina, uint32_t indice); //SIN SINCRO

bool criterioAlmacenamiento(uint32_t pid, uint32_t nroPagina, uint32_t indice) ; //SIN SINCRO

void* solicitarBytesMemoria(uint32_t pid, uint32_t nroPagina, uint32_t offset); //CON SINCRO

uint32_t numeroEntradaCache(uint32_t pid, uint32_t nroPagina); //SIN SINCRO

void* solicitarBytesCache(uint32_t pid, uint32_t nroPag, uint32_t offset); //SIN SINCRO PORQUE DEVUELVO REFERENCIA DIRECTA AL DATO

void cargarEntradaCache(uint32_t pid, uint32_t nroPag, void*buffer); //SIN SINCRO

void* obtenerEntradaPagXProc(uint32_t pid); //SIN SINCRO

bool existeProceso(uint32_t pid); //SIN SINCRO

bool offsetDentroDePagina(uint32_t inicio,uint32_t offset);

void darDeBajaProgramaCache(uint32_t pid); //SIN SINCRO

uint32_t darDeBajaEstructurasAdministrativas(uint32_t pid); //SIN SINCRO

uint32_t darDeBajaEntradaPaginaXProceso(uint32_t pid); //SIN SINCRO

void atenderConexion(uint32_t socket);

uint32_t enviarTamanioPagina(uint32_t socket);

uint32_t enviarError(uint32_t socket);

uint32_t enviarOk(uint32_t socket);

uint32_t enviarBytesLeidos(uint32_t socket,void* datos,uint32_t tamanio);

void atenderKernel(uint32_t socket);

void atenderCpu(uint32_t socket);

uint32_t recibirConexiones(uint32_t socket);

uint32_t escucharConexiones();

////////////////////////INTERFACES////////////////////////
uint32_t inicializarPrograma(uint32_t idPrograma, uint32_t cantidadPaginasRequeridas); //SIN SINCRO

bytesSolicitud* solicitarBytes(uint32_t pid, uint32_t nroPagina, uint32_t offset, uint32_t tamanio); // CON SINCRO

void* almacenarBytes(uint32_t pid, uint32_t nroPagina, uint32_t offset, uint32_t tamanio,void* buffer); //CON SINCRO

uint32_t asignarPaginasProceso(uint32_t pid, uint32_t cantidadPaginas); //CON SINCRO

uint32_t finalizarPrograma(uint32_t pid); //CON SINCRO

//OTRAS OPERACIONES

void ConsolaMemoria();

void modificarRetardo();

void dumpCache();

void dumpAdministracion();

void flushCache(); //CON SINCRO

void sizeMemoria();

void sizePid(uint32_t pid);

int recvPid(int socket, uint32_t length);

#endif /* MEMORIA_H_ */
