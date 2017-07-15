/*
 * capaFileSystem.h
 *
 *  Created on: 23/6/2017
 *      Author: utnso
 */

#ifndef SRC_CAPAFILESYSTEM_H_
#define SRC_CAPAFILESYSTEM_H_

#include "Kernel.h"


///operaciones de archivos

int abrirArchivo(uint32_t pid,char* path, t_flags permisos);

bool cerrarArchivo(uint32_t pid,uint32_t fd);

bool borrarArchivo(uint32_t pid, uint32_t fd);

bool moverCursor(uint32_t fd, uint32_t pid, uint32_t posicion);

bool escribirArchivo(uint32_t fd, uint32_t pid,char* info,uint32_t size);

bool leerArchivo(uint32_t pid, uint32_t fd, uint32_t size,char** buffer);


///operaciones auxiliares de operaciones de archivos

t_entradaPorPFT* crearEntradaEnPFT(uint32_t pid,t_flags flags,uint32_t fd,uint32_t globalfd);

int agregarEntradaGFT(char*path);

int crearEntradaGFT(char* path);

int nuevoGFD();

int nuevoFD(uint32_t pid);

bool tieneElPath(t_entradaPorGFT* entrada);

bool esDelProceso(t_entradaPorPFT* entrada);

bool fdMenorGFT(t_entradaPorGFT *fd, t_entradaPorGFT *mayorfd);

bool fdMenorPFT(t_entradaPorPFT *fd, t_entradaPorPFT *mayorfd);

int validarArchivo(char* path);

int crearArhivoFS(char* path);

int borrarArchivoFS(char* path);

int guardarDatosFS(char* path, uint16_t offset,uint16_t size,char* buffer);

void borrarEntradaPFT(uint32_t pid,uint32_t fd);

void destroyPFT(t_entradaPorPFT* self);

void destroyGFT(t_entradaPorGFT* self);

char* obtenerDatos(char* path, uint16_t offset,uint16_t size);

bool aBorrarPFT(t_entradaPorPFT* entrada);

bool aBorrarGFT(t_entradaPorGFT* entrada);

t_entradaPorPFT* entradaPorPFT();

t_entradaPorGFT* entradaPorGFT();

bool esDelFD(t_entradaPorPFT* entrada);

bool esDelGFD(t_entradaPorGFT* entrada);

#endif /* SRC_CAPAFILESYSTEM_H_ */
