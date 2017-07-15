/*
 * serializer.h
 *
 *  Created on: 5/5/2017
 *      Author: utnso
 */

#ifndef SOCKETS_SERIALIZER_H_
#define SOCKETS_SERIALIZER_H_

#include "socket.h"
#include "StructsUtils.h"


t_datos_programa * datosPrograma_deserializer(char *serialized);

char * datosPrograma_serializer(t_datos_programa *datos, uint32_t *length);

char * uint32_serializer(uint32_t * self,uint32_t* length);

uint32_t *uint32_deserializer(char*serialized);

char * solicitudLectura_serializer(t_solicitudLectura *self,uint32_t *length);

t_solicitudLectura * solicitudLectura_deserializer(char *serialized);

char * solicitudEscritura_serializer(t_solicitudEscritura *self,uint32_t *length);

t_solicitudEscritura * solicitudEscritura_deserializer(char *serialized);

char * solicitudHeap_serializer(t_solicitudHeap *instruccion, uint32_t * length);

t_solicitudHeap *solicitudHeap_deserializer(char * serialized);

char * serializer_opVarCompartida (t_op_varCompartida * self , uint32_t * length);

t_op_varCompartida* deserializer_opVarCompartida(char *serialized);

char *serializer_indiceCodigo(t_list* self, uint32_t * length);

t_list *deserializer_indiceCodigo(char * serialized);

char *serializer_indiceStack(t_list* self, uint32_t * length);

t_list *deserializer_indiceStack(char * serialized);

char * serializer_pcb(t_PCB * self, uint32_t *length);

t_PCB * deserializer_pcb(char *serialized);

char *operacionDescriptorArchivo_serializer(t_operacionDescriptorArchivo* self, uint32_t * length);

t_operacionDescriptorArchivo* operacionDescriptorArchivo_deserializer (char* serialized);

char* guardarDatosSerializer(t_guardarDatos* operacion, uint32_t *length);

t_guardarDatos* guardarDatosDesSerializer(char* serializado);

char* obtenerDatosSerializer(t_obtenerDatos* operacion, uint32_t* length);

t_obtenerDatos* obtenerDatosDesSerializer(char* serializado);

char* validarArchivoSerializer(t_validarArchivo* operacion, uint32_t* length);

t_validarArchivo* validarArchivoDesSerializer(char* serializado);

#endif /* SOCKETS_SERIALIZER_H_ */
