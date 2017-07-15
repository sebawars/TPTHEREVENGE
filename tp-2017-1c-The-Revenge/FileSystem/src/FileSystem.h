/*
 * FileSystem.h
 *
 *  Created on: 14/4/2017
 *      Author: utnso
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_
#define LOG_PATH "log.txt"

#include <Sockets/StructsUtils.h>
#include <Sockets/socket.h>
#include <Sockets/SocketsCliente.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>
#include <commons/log.h>
#include <commons/config.h>
#include <semaphore.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/config.h>
#include <commons/bitarray.h>
#include <sys/mman.h>
#include <stdint.h>
//#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <Sockets/socket.h>
#include <Sockets/Serializer.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
t_log *logFile;

#define true 1
#define false 0
typedef int boolean;


t_log *logFile;
int socketKernel;
typedef struct {
	uint16_t PUERTO;
	char* PUNTO_MONTAJE;
	t_log_level DETALLE_LOG;
} archivoConfig;

typedef struct {
	uint16_t TAM_BLOQUES;
	uint16_t CANT_BLOQUES;
	char* MAGIC_NUMBER;
} archivoMetadata;

archivoConfig* archCFS;
archivoMetadata* metadata;
char* map;
t_bitarray* bitArray;

int main(int argc, char** argv);

int AbrirServidor();

int handShake(int server);

void guardarBitArray();

int abrirBitmap();

int validarArchivo(char* path);

void crearArchivo(char* path);

void borrarArchivo(char* path);

void obtenerDatos(char*path, uint16_t offset, uint16_t size);

int guardarDatos(char*path, uint16_t offset, uint16_t size, char* buffer);

int crearServer(int port, int backlog);

t_infoArchivo* leerInfoArchivo(char* path);

void asignarBloque(char* path,uint16_t bloque);

uint8_t tamanioArchivo(char*path);

int bloqueVacio();

void crearBloque(uint16_t bloque);

void liberarBloques(t_infoArchivo* archivo);

char* binAPath(int bloque);

int leerDatosBin(int bin, uint16_t offset,uint16_t* size,char** buffer);

int escribirDatosBin(int bin, uint16_t offset,uint16_t* size,char** buffer);

void _mkdir(char* dir);

void cleanBitArray();

int crearBitmap();

int validarArchivoSinEnvio(char* path);

void aumentarTamanio(char* path);

#endif /* FILESYSTEM_H_ */
