/*
 * primitivasAnsisop.h
 *
 *  Created on: 5/5/2017
 *      Author: utnso
 */

#ifndef SRC_PRIMITIVASANSISOP_H_
#define SRC_PRIMITIVASANSISOP_H_

#include <parser/parser.h>
#include <parser/sintax.h>
#include <parser/metadata_program.h>
#include <commons/log.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <commons/string.h>
#include "CPU.h"

bool esArgumento(t_nombre_variable identificador_variable);

//Ansisop Funciones
t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable);
void asignar(t_puntero direccion_variable, t_valor_variable valor);
t_valor_variable dereferenciar(t_puntero direccion_variable);
t_puntero definirVariable(t_nombre_variable identificador_variable);
t_valor_variable obtenerValorCompartida(t_nombre_compartida variable);
t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor);
void irAlLabel(t_nombre_etiqueta etiqueta);
void llamarSinRetorno(t_nombre_etiqueta etiqueta);
void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar);
void finalizar(void);
void retornar(t_valor_variable retorno);

//Ansisop Kernel
void ansisop_signal(t_nombre_semaforo identificador_semaforo);
void wait(t_nombre_semaforo identificador_semaforo);
t_puntero reservar(t_valor_variable espacio);
void liberar(t_puntero puntero);
t_descriptor_archivo abrir(t_direccion_archivo direccion, t_banderas flags);
void borrar(t_descriptor_archivo descriptor_archivo);
void cerrar(t_descriptor_archivo descriptor_archivo);
void moverCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion);
void ansisop_leer(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio);
void ansisop_escribir(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio);

char * agregarCaracterAdmiracion(char * variable);

extern uint32_t operacion;

#endif /* SRC_PRIMITIVASANSISOP_H_ */
