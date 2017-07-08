/*
 * capaMemoria.c
 *
 *  Created on: 23/6/2017
 *      Author: utnso
 */

#include "capaMemoria.h"

int Tmax;
int tamMetadata=sizeof(uint32_t)+sizeof(bool);


int superaMaximoDisponible(int valorNumerico, int socketCPU)
{
	if (valorNumerico > Tmax)
	{
		header_t header;
		header.type = ALLOC_MAYOR_TAM_PAGINA;
		header.length = 0;

		sockets_send(socketCPU, &header, "\0");

		log_error(logFile,
				"No se puede alocar el pedido de: %i del socket:%i porque"
						" supera el maximo disponible de:%i\n", valorNumerico,
				socketCPU, Tmax);

		return true;

	}else

		return false;
}

int superaMaximoAsignacion(int pid, int socketMemoria, int socketCPU)
{
	header_t header;
	int resultado=pedirPaginaAMemoria(pid,1,socketMemoria);

	if(resultado==-1)
	{
	header.type = NO_PUDO_ASIGNAR_PAGINAS;
	header.length = 0;

	sockets_send(socketCPU, &header, "\0");
	}

	return resultado;
}

uint32_t crearPaginaHeap(int valorNumerico,int socketMemoria, int socketCPU, t_PCB* pcb)
{
	int cantPaginas=list_size(pcb->tablaHeap->pagina);
	int supera=	superaMaximoAsignacion( pcb->PID, socketMemoria, socketCPU);

	if(supera == -1)
	{
		log_error(logFile,
					"No se puede alocar el pedido de: %i del socket:%i porque"
					"no se pueden asignar mas paginas al proceso de pid:%i\n",
					valorNumerico, socketCPU, pcb->PID);
		return -1;

	}else{

		t_heapMetadata* heapOcupado = malloc(sizeof(t_heapMetadata));
		heapOcupado->isFree = false;
		heapOcupado->size = valorNumerico;

		t_heapMetadata* heapLibre = malloc(sizeof(t_heapMetadata));
		heapLibre->isFree = true;
		heapLibre->size = tamanioPagina - (2 * tamMetadata)- valorNumerico;

		t_bloque* bloqueUsado=malloc(sizeof(t_bloque));
		bloqueUsado->heapMetadata=heapOcupado;
		bloqueUsado->numeroBloque=1;
		bloqueUsado->offset = cantPaginas*tamanioPagina + tamMetadata;
		bloqueUsado->data=malloc(bloqueUsado->heapMetadata->size);

		t_bloque* bloqueLibre = malloc(sizeof(t_bloque));
		bloqueLibre->heapMetadata = heapLibre;
		bloqueLibre->numeroBloque = 0;
		bloqueLibre->offset = (cantPaginas + 1 )*tamanioPagina - bloqueLibre->heapMetadata->size;

		t_pagina_heap* paginaAux= malloc(sizeof(t_pagina_heap));
		paginaAux->numero_pagina= cantPaginas;
		paginaAux->tam_disponible= heapLibre->size;
		paginaAux->bloques=list_create();

		list_add(paginaAux->bloques, bloqueUsado);
		list_add(paginaAux->bloques, bloqueLibre);

		list_add(pcb->tablaHeap->pagina,paginaAux);

		bool _tieneElPid(t_PCB* programa)
		{
			return (programa->PID) == pcb->PID;
		}

		t_cantPagsHeap* pagsHeap=list_find(tablaHeap, (void*)_tieneElPid);
		pagsHeap->cantPagsHeap ++;

		log_info(logFile,
				"[HEAP] Se ha creado correctamente la pagina N°:%i correspondiente"
						" al heap para el proceso de pid:%i\n",
				paginaAux->numero_pagina, pcb->PID);

		logEstadoPaginaHeap(paginaAux, pcb->PID);

		log_debug(logFile,
				"[HEAP] Se Aloca correctamente heap en puntero: %d en Heap con CPU,"
						" socket: %d\n para el proceso con pid:%i\n",
				bloqueUsado->offset, socketCPU, pcb->PID);

		return bloqueUsado->offset;

	}

}

bool tienePaginaConTamanioDisponibleSuficiente(t_list* paginas, int valorNumerico)
{

	bool _tieneTamanioSuficiente(t_pagina_heap* pagina)
	{
			return (pagina->tam_disponible >= (valorNumerico+tamMetadata));
	}

	t_pagina_heap* pagaux= list_find(paginas,(void*)_tieneTamanioSuficiente);

	if(pagaux!=NULL){
		return true;
	}else{
		return false;
	}
}

bool tienePaginaALiberar(t_list* paginas, int numPagina)
{

	bool _tieneLaPagina(t_pagina_heap* pagina)
	{
			return (pagina->numero_pagina == numPagina);
	}

	t_pagina_heap* pagaux= list_find(paginas,(void*)_tieneLaPagina);

	if(pagaux!=NULL){
		return true;
	}else{
		return false;
	}

}

bool tieneBloqueFreeConTamanioSuficiente(t_pagina_heap *pagina, int valorNumerico)
{

	bool _tieneTamanioSuficiente(t_bloque* bloque)
	{
		return (bloque->heapMetadata->isFree==true)&&(bloque->heapMetadata->size>=valorNumerico);
	}

	return list_any_satisfy(pagina->bloques,(void*)_tieneTamanioSuficiente);

}

bool tieneBloqueALiberar(t_pagina_heap *pagina, int offset)
{

	bool _tieneElOffset(t_bloque* bloque)
	{
		return (bloque->heapMetadata->isFree==false)&&(bloque->offset==offset);
	}

	return list_any_satisfy(pagina->bloques,(void*)_tieneElOffset);

}

void enviarPunteroACPU(uint32_t offsetADevolver,int socketCPU, int tamanio)
{
	char* mensaje = malloc(tamanio);
	header_t header;

	mensaje = uint32_serializer(&offsetADevolver,&header.length);

	header.type = SUCCESS;
	sockets_send(socketCPU, &header, mensaje);

	free(mensaje);

}

int compactar(t_bloque* bloqIzquierda, t_bloque* bloqDerecha)
{
	if(bloqIzquierda->heapMetadata->isFree && bloqDerecha->heapMetadata->isFree)
	{
		bloqIzquierda->heapMetadata->size=bloqIzquierda->heapMetadata->size + tamMetadata + bloqDerecha->heapMetadata->size;

		log_info(logFile, "[HEAP] Se han compactado dos bloques de una pagina heap generando un unico bloque con "
					"tamaño:%i\n",bloqIzquierda->heapMetadata->size);
		return 1;
	}else{
		return 2;
	}

}
int encontrarBloquesVecinosLibres(t_list* bloques, t_bloque* bloque,t_pagina_heap* pag)
{
	int i;
	int aux=-1;
	int compacto=-1;
	int indice =-1;
	int cantBloques= list_size(bloques);

	for (i = 0; i < cantBloques; i++)
	{
		t_bloque* bloqueaux = list_get(bloques, i);

		if (bloqueaux->offset == bloque->offset)
		{
			indice = i;
			break;
		}
	}

	if(indice!= -1)
	{
		t_bloque* medio= list_get(bloques,i);

		if(indice>0)
		{
			t_bloque* izq = list_get(bloques,i-1);
			if(izq->heapMetadata->isFree){
			compacto = compacto + compactar(izq,medio);
			if(compacto!= -1){
				pag->tam_disponible=pag->tam_disponible+tamMetadata+izq->heapMetadata->size;
				list_remove(bloques,i);
				free(medio);
			}}
		}
		if(indice<cantBloques)
		{
			t_bloque* der= list_get(bloques,i+1);
			if(der->heapMetadata->isFree){
			aux=compactar(medio, der);
			compacto=compacto + aux;
			if(aux!=-1){
				pag->tam_disponible=pag->tam_disponible+tamMetadata+der->heapMetadata->size;
			list_remove(bloques, i+1);
			free(der);
		}}
		}

	}

	return compacto;

}

void liberarPagina(t_tabla_heap* tabla,t_pagina_heap* pagina)
{

	bool _obtenerBloqueUsado(t_bloque * unBloque)
	{
		return (unBloque->heapMetadata->isFree == false);
	}

	t_list * bloquesUsados = list_filter(pagina->bloques,(void*) _obtenerBloqueUsado);

	if(list_is_empty(bloquesUsados))
	{
		bool _esLaPagina(t_pagina_heap* pag)
		{
			return pag->numero_pagina== pagina->numero_pagina;
		}

		if(!list_is_empty(pagina->bloques))
		{
			list_clean(pagina->bloques);
		}

		list_remove_by_condition(tabla->pagina, (void*)_esLaPagina);

		log_info(logFile, "[HEAP] Se ha liberado la pagina:% de heap del programa con "
				"pid:%i\n", pagina->numero_pagina, tabla->pid);
	}

}


void operacionesHeap(int socketCPU, int tamanio, int operacion, int socketMemoria)
{
	header_t header;
	char * mensaje = malloc(tamanio);
	uint32_t *valorNumerico=malloc(sizeof(uint32_t));
	t_solicitudHeap *datosHeap;
	t_PCB* pcb;
	uint32_t offsetADevolver;
	int supera;

	int nbytes=recv(socketCPU, mensaje, tamanio, 0);

	if(nbytes>0)
	{
		//Si la operacion recibida es Alocar valorNumerico sera el espacio pedido por el proceso.
		//Si la operacion recibida es Liberar valorNumerico sera el valor puntero a liberar en Heap
		datosHeap = solicitudHeap_deserializer(mensaje);

		//strcpy(valorNumerico,datosHeap->puntero);
		*valorNumerico=datosHeap->puntero;
		int pid=datosHeap->pid;

		free(mensaje);
		free(datosHeap);


		bool _tieneElPid(t_PCB* programa)
		{
				return (programa->PID) == pid;
		}

		bool _tieneSizeSuficiente(t_bloque* bloque)
		{
			return (bloque->heapMetadata->isFree==true)&&(bloque->heapMetadata->size>=*valorNumerico);
		}

		bool _tieneTamanioDisponibleSuficiente(t_pagina_heap* pagina)
		{
			return (pagina->tam_disponible >= (*valorNumerico+tamMetadata));
		}

		pcb = list_find(listaPCB, (void*) _tieneElPid);

        log_debug(logFile, "Se recibe una operacion codigo: %d sobre Heap con CPU, Valor= %d, "
                                "socket: %d\n",operacion,*valorNumerico,socketCPU);

        t_cantPagsHeap* pagsHeap=list_find(tablaHeap, (void*)_tieneElPid);

		switch(operacion){

			case ALOCAR:
				//Aca hay que desarrollar la tarea interna sobre el Heap ver si es necesario algun dato mas por parte de la CPU
				//Por ahora tenemos socketCPU y tamanio a alocar, el Kernel debe devolver la posicion de puntero referencia al Heap, con SUCCESS o ERROR_HEAP

				supera = superaMaximoDisponible(*valorNumerico, socketCPU);

			if (!supera)
			{
				if (list_is_empty(pcb->tablaHeap->pagina))
				{
					//Todavia no teniamos ninguna pagina de heap

					offsetADevolver = crearPaginaHeap(*valorNumerico,socketMemoria, socketCPU,
							pcb);

					if (offsetADevolver != -1)
					{
						enviarPunteroACPU(offsetADevolver, socketCPU, tamanio);
						pagsHeap->alocarBytes=pagsHeap->alocarBytes + *valorNumerico;
						pagsHeap->alocarCant++;
					} else {

						log_error(logFile,
								"No se pudo crear una pagina de heap para el "
										"proceso con pid:%i\n", pcb->PID);
					}

				} else {
					//tengo paginas de heap
					if (tienePaginaConTamanioDisponibleSuficiente(pcb->tablaHeap->pagina, (int) *valorNumerico))
					{
						int cantPagsPosibles = list_count_satisfying(pcb->tablaHeap->pagina,(void*) _tieneTamanioDisponibleSuficiente);

						//tengo alguna pagina con tamaño disponible suficiente
						bool pudoAlocar = false;

						int cantPagsRevisadas = 0;
						t_list* aux = list_create();

						while (pudoAlocar == false && cantPagsPosibles > cantPagsRevisadas)
						{
							//mientras no haya podido alocar el pedido y no haya revisado todas las paginas posibles
							t_pagina_heap *pagina;
							pagina = list_find((pcb->tablaHeap->pagina),
									(void*) _tieneTamanioDisponibleSuficiente);

							if (tieneBloqueFreeConTamanioSuficiente(pagina,(int) *valorNumerico))
							{
								logEstadoPaginaHeap(pagina, pcb->PID);

								//busco el primero por algoritmo First Fit
								t_bloque *bloque = list_find((pagina->bloques),
										(void*) _tieneSizeSuficiente);

								//actualizo la metadta del bloque
								uint32_t tamanio=bloque->heapMetadata->size;
								bloque->heapMetadata->isFree = false;
								bloque->heapMetadata->size = *valorNumerico;

								//creo un nuevo bloque con lo que me sobra del anterior
								t_heapMetadata* heapLibre = malloc(sizeof(t_heapMetadata));
								heapLibre->isFree = true;
								heapLibre->size = tamanio - *valorNumerico -tamMetadata;

								t_bloque* bloqueNuevo = malloc(	sizeof(t_bloque));
								bloqueNuevo->numeroBloque=bloque->numeroBloque+1;
								bloqueNuevo->heapMetadata= heapLibre;
								bloqueNuevo->offset = bloque->offset + tamMetadata + *valorNumerico;
								bloqueNuevo->data=malloc(bloqueNuevo->heapMetadata->size);

								//todo me importa el orden en el que lo agrego?
								list_add(pagina->bloques, bloqueNuevo);

								//no olvidar actualizar el disponible de la pagina
								pagina->tam_disponible = pagina->tam_disponible - tamMetadata - *valorNumerico;

								//envio a cpu el puntero con lo alocado
								enviarPunteroACPU(bloque->offset, socketCPU, tamanio);
								pagsHeap->alocarBytes=pagsHeap->alocarBytes + *valorNumerico;
								pagsHeap->alocarCant++;

								pudoAlocar = true;

								logEstadoPaginaHeap(pagina, pcb->PID);

							} else {

								//para poder revisar todas las paginas que tengan tamaño suficiente
								cantPagsRevisadas++;
								pagina = list_remove_by_condition(pcb->tablaHeap->pagina,
												(void*) _tieneTamanioDisponibleSuficiente);

								list_add(aux, pagina);
							}
						}
						//para dejar la lista como estaba
						int e;
						for (e = 0; e<cantPagsRevisadas; e++)
						{
							t_pagina_heap * pagina = list_remove(aux, 0);
							list_add(pcb->tablaHeap->pagina, pagina);
						}

						if (cantPagsRevisadas == cantPagsPosibles)
						{
							//significa que el pedido es mas grande que el tamaño de cada bloque libre
							//por fragmentacion externa, pido otra pagina para alocarlo
							offsetADevolver = crearPaginaHeap(*valorNumerico, socketMemoria,
									socketCPU, pcb);

							if (offsetADevolver != -1)
							{
								enviarPunteroACPU(offsetADevolver, socketCPU,
										tamanio);

								pagsHeap->alocarBytes=pagsHeap->alocarBytes + *valorNumerico;
								pagsHeap->alocarCant++;

							} else {
								log_error(logFile,
										"No se pudo crear una pagina de heap para el "
												"proceso con pid:%i\n", pcb->PID);
							}
						}

					} else {
						//no tengo ninguna pagina con tamaño disponible suficiente
						offsetADevolver = crearPaginaHeap(*valorNumerico, socketMemoria,
								socketCPU, pcb);

						if (offsetADevolver != -1)
						{
							enviarPunteroACPU(offsetADevolver, socketCPU,
									tamanio);
							pagsHeap->alocarBytes=pagsHeap->alocarBytes + *valorNumerico;
							pagsHeap->alocarCant++;
						} else {
							log_error(logFile,
									"No se pudo crear una pagina de heap para el "
											"proceso con pid:%i\n", pcb->PID);
						}
					}

				}
			}


				break;

			case LIBERAR:
					//Aca hay que desarrollar la tarea interna sobre el Heap ver si es necesario algun dato mas por parte de la CPU
					//Por ahora tenemos socketCPU, el pid y el puntero a liberar, el Kernel debe devolver con SUCCESS o ERROR_HEAP
				if(1)
				{
				int pagina = *valorNumerico / tamanioPagina;
				int offset = *valorNumerico % tamanioPagina;

				log_debug(logFile, "[HEAP]La posicion de memoria que se quiere liberar se encuentra en la "
								"pagina:%i, offset:%i del programa con pid:%i\n", pagina, offset, pid);


					if(tienePaginaALiberar(pcb->tablaHeap->pagina,pagina))
					{
						bool _tieneLaPagina(t_pagina_heap * pag)
						{
							return(pag->numero_pagina==pagina);
						}

					t_pagina_heap * pag=list_find(pcb->tablaHeap->pagina,(void*)_tieneLaPagina);

						if(tieneBloqueALiberar(pag,offset))
						{
							logEstadoPaginaHeap(pag, pcb->PID);

							bool _tieneElOffset(t_bloque* bloq)
							{
								return bloq->offset==offset;
							}

							t_bloque* bloque=list_find(pag->bloques, (void*) _tieneElOffset);

							bloque->heapMetadata->isFree=true;
//todo ver por que tira basura ese tam disponible
							pag->tam_disponible=pag->tam_disponible+tamMetadata+bloque->heapMetadata->size;
							log_info(logFile, "Se ha liberado correctamente la direccion de memoria "
									"solicitada por la CPU:%d\n", socketCPU);

							log_info(logFile, "[HEAP]Se liberaron %i bytes de heap del proceso con pid:%i\n",
									bloque->heapMetadata->size,pid);

							header.type=SUCCESS;
							header.length=0;

							sockets_send(socketCPU,&header,"\0");
							pagsHeap->liberarBytes=pagsHeap->liberarBytes + *valorNumerico;
							pagsHeap->liberarCant++;

							int compacto= encontrarBloquesVecinosLibres(pag->bloques, bloque,pag);
							if(compacto!=-1)
								logEstadoPaginaHeap(pag, pcb->PID);
							liberarPagina(pcb->tablaHeap,pag);

						}else{

							log_error(logFile, "Error al liberar Heap con CPU, socket: %d\n",socketCPU);

							log_error(logFile, "[HEAP]No se  puede liberar la memoria porque la direccion "
								"solicitada no apunta a un bloque de datos\n");

							header.type=ERROR_HEAP;
							header.length=0;

							sockets_send(socketCPU,&header,"\0");

						}

					}else{
						log_error(logFile, "Error al liberar Heap con CPU, socket: %d\n",socketCPU);

						log_error(logFile, "[HEAP]No se puede liberar la memoria porque no existe "
								"la pagina solicitada \n");

						header.type=ERROR_HEAP;
						header.length=0;

						sockets_send(socketCPU,&header,"\0");

					}
				}

	break;
		}

	}else{
		log_error(logFile, "Error en recibir datos de reserva Heap con CPU, socket: %d\n",socketCPU);

	}
}

void logEstadoPaginaHeap(t_pagina_heap* pagina, int pid)
{
	log_debug(logFile, "[HEAP] El estado actual de la pagina:%i del programa con pid:%i es:"
		"tamaño de pagina disponible:%i, cantidad de bloques:%i. Los bloques en dicha pagina son:",
		pagina->numero_pagina,pid,pagina->tam_disponible,list_size(pagina->bloques));

	int i;
	for(i=0;i<list_size(pagina->bloques);i++)
	{
		t_bloque * aux= list_get(pagina->bloques,i);
		logEstadoBloque(aux);
	}

}

void logEstadoBloque(t_bloque* bloque)
{
	if(bloque->heapMetadata->isFree)
	log_debug(logFile, "[HEAP] Bloque libre con tamaño:%i y offset en:%i.", bloque->heapMetadata->size, bloque->offset);
	else
		log_debug(logFile, "[HEAP] Bloque ocupado con tamaño:%i y offset en:%i", bloque->heapMetadata->size, bloque->offset);
}

void almacenar(char* buffer,uint32_t valorNumerico,uint32_t pid){
	t_solicitudHeap *datosHeap=malloc(sizeof(t_solicitudHeap));
	datosHeap->pid=pid;
	datosHeap->puntero=valorNumerico;

	int pagina = valorNumerico / tamanioPagina;
	int offset = valorNumerico % tamanioPagina;

	_Bool _tieneElPid(t_PCB* programa)
			{
					return (programa->PID) == pid;
			}
	t_PCB* pcb = list_find(listaPCB, (void*) _tieneElPid);

	log_debug(logFile, "La posicion de memoria que se quiere liberar se encuentra en la "
					"pagina:%i, offset:%i del programa con pid:%i\n", pagina, offset, pid);

	bool _tieneLaPagina(t_pagina_heap * pag)
	{
		return(pag->numero_pagina==pagina);
	}
	t_pagina_heap * pag=list_find(pcb->tablaHeap->pagina,(void*)_tieneLaPagina);

	bool _tieneElOffset(t_bloque* bloq)
	{
		return bloq->offset==offset;
	}
	t_bloque* bloque=list_find(pag->bloques, (void*) _tieneElOffset);

	bloque->data=string_from_format(buffer);

}
