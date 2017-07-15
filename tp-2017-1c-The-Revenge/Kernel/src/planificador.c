/*
 * planificador.c
 *
 *  Created on: 23/6/2017
 *      Author: utnso
 */

#include "planificador.h"



int crearHiloPlanificacion(int socketMemoria)
{
	int valorHilo = -1;
	valorHilo = pthread_create(&hiloPlanificacion, NULL, (void*) planificar,
	socketMemoria);
	if (valorHilo != 0) {
		perror(">>Error al crear hilo de Planificacion\n\n");
		log_error(logFile, "error al crear el hilo de Planificacion\n");
		exit(1);
	} else {
		log_info(logFile, "el hilo de Planificacion se creó correctamente\n");
	}
	return valorHilo;

}

void algoritmo()
{
	int algoritmo = strcmp(archC->ALGORITMO, "FIFO");
	t_PCB *proc;
	t_PCB *lPCB;
	t_cpu *cpu;
	int i,cantReady;

	bool _desocupada(t_cpu* cpu)
	{
		return (cpu->ocupado) == false;
	}

	bool _tieneElPid(t_PCB* programa)
	{
		return (programa->PID) == proc->PID;
	}

	if (!list_is_empty(colaReady) && list_any_satisfy(listaCPU,(void*)_desocupada))
	{
		sem_wait(&mutexLista);
		sem_wait(&mutex_lista_pcb);
		//cantReady=list_size(colaReady);
		for (i = 0 ; i < list_size(colaReady); i++)
		{
			//sem_wait(&progRunning); los semaforos no cambian de valor dinamicamente
			//recordar que no se usa progRunning, ahora es cantProgRunning

			proc = list_get(colaReady, i);

			log_debug(logFile, "size-listaCPU= %i ---- size-colaExec=%i  ----- size-aSerAbortados=%i ",list_size(listaCPU),list_size(colaExec),list_size(aSerEliminado));
			if(list_size(listaCPU)>list_size(colaExec))
			{

				if(algoritmo==0)
				{
					proc->quantum=0;

					log_debug(logFile, "El algoritmo de planificacion es FIFO");

				}else{
					pthread_mutex_lock(&semPlanificacion);
					proc->quantum=archC->QUANTUM;
					proc->quantum_sleep=archC->QUANTUM_SLEEP;
					pthread_mutex_unlock(&semPlanificacion);

					log_info(logFile,"El algoritmo de planificacion es RR"
							"con quantum sleep:%d", proc->quantum_sleep);
					log_debug(logFile, "El algoritmo de planificacion es RR"
							"con quantum:%d", proc->quantum);
				}

				//sem_wait(&mutexLista);
				//proc = list_get(colaReady, i);
				//sem_post(&mutexLista);

				//sem_wait(&mutex_lista_pcb);
				lPCB=list_find(listaPCB,(void*)_tieneElPid);
				//sem_post(&mutex_lista_pcb);

				if(!lPCB->abortarEjecucion)
				{

					//sem_wait(&mutexLista);

					proc->punteroColaPlanif=colaExec;
					proc->estado = RUNNING;

					lPCB->punteroColaPlanif=colaExec;
					lPCB->estado = RUNNING;

					proc = list_remove(colaReady, i);
					i=0;
					list_add(colaExec, proc);

					//sem_post(&mutexLista);

					sem_wait(&mutexCPU);
					t_cpu * cpu = list_find(listaCPU,(void*)_desocupada);
					cpu->PidEjecutando=proc->PID;
					cpu->ocupado=true;
					sem_post(&mutexCPU);

					envioPCBACpuDisponible(cpu->socketC, proc);

					log_info(logFile, "Se mando a ejecutar el PCB con PID:%i a la cpu "
							"con socket:%i", proc->PID,cpu->socketC);
				}else{
					if(lPCB->EC == KERNEL_FINALIZAR_PROGRAMA)
					{
						t_aSerEliminado* ePCB = malloc(sizeof(t_aSerEliminado));
						ePCB->exitcode = KERNEL_FINALIZAR_PROGRAMA;
						ePCB->pcb = lPCB;
						ePCB->aviso=FINALIZACION_KERNEL;
						ePCB->pcb->estado=READY;
						list_add(aSerEliminado,ePCB);
						log_debug(logFile, "El PCB con PID: %i pasa de la Cola Ready a la cola aSerEliminados debido a una finalizacion abortiva", ePCB->pcb->PID);
					}else if (lPCB->EC==CONSOLA_FINALIZAR_PROGRAMA)
					{

					t_aSerEliminado * nPCB = procesoASerAbortado(proc);
					nPCB->pcb->estado=READY;
					list_add(aSerEliminado,nPCB);
					log_debug(logFile, "El PCB con PID: %i pasa de la Cola Ready a la cola aSerEliminados debido a una finalizacion abortiva", nPCB->pcb->PID);
					}
				}
			}else{
				break;
			}
		}
		sem_post(&mutex_lista_pcb);
		sem_post(&mutexLista);

	}
}

void verificarAbortados()
{
	t_PCB* lPCB;
	t_pcbBloqueado* pcbBloqueado;
	bool encontrado=false;
	int i;

	/*
	while(!queue_is_empty(aSerBloqueado))
	{

		pcbBloqueado=queue_pop(aSerBloqueado);

		sem_wait(&mutex_lista_pcb);
		lPCB = list_find(listaPCB, (void*) _tieneElPid);
		sem_post(&mutex_lista_pcb);

		//Revisar la asignacion del lPCB de la listaPCB
		pcbBloqueado->pcb->estado=WAITING;
		lPCB->estado=WAITING;
		pcbBloqueado->pcb->punteroColaPlanif=colaBlock;
		lPCB->punteroColaPlanif=colaBlock;
		if (lPCB->tablaHeap!=NULL)
			pcbBloqueado->pcb->tablaHeap=lPCB->tablaHeap;


		sem_wait(&mutexLista);
		//Ver porque hay que hacer free del pcb
		t_PCB *programa =list_remove_by_condition(colaExec, (void*) _tieneElPid);
		//Esto hay que tener cuidado, la cola Exec no maneja estructura t_PCB
		//Esto lo corregimos mal
		queue_push(colaBlock,pcbBloqueado);
		sem_post(&mutexLista);

		log_info(logFile, "El programa con PID:%i ha salido de la cola de ejecucion"
				"para ingresar a la cola de bloqueados\n", pcbBloqueado->pcb->PID);
		log_info(logFile, "El programa con PID:%i ha reingresado a la lista de PCB con "
				"sus atributos actualizados", pcbBloqueado->pcb->PID);

	}*/


	sem_wait(&mutexLista);
	sem_wait(&mutex_lista_pcb);
	int cantBlock=queue_size(colaBlock);
	t_queue *aux=colaBlock;
	for(i=0; i < cantBlock; i++)
	{

		pcbBloqueado = queue_pop(colaBlock);

		bool _tieneElPid(t_PCB* programa)
		{
			return (programa->PID) == pcbBloqueado->pcb->PID;
		}

		//sem_wait(&mutex_lista_pcb);
		lPCB = list_find(listaPCB,(void*) _tieneElPid);
		//sem_post(&mutex_lista_pcb);

		if(lPCB->abortarEjecucion==true && encontrado==false)
		{
			//Bandera para entrar una unica vez
			encontrado=true;

			pcbBloqueado->pcb->estado = READY;
			pcbBloqueado->pcb->punteroColaPlanif = colaReady;

			//sem_wait(&mutex_lista_pcb);
			lPCB->estado = READY;
			lPCB->punteroColaPlanif = colaReady;
			//sem_post(&mutex_lista_pcb);

			//sem_wait(&mutexLista);
			list_add(colaReady, pcbBloqueado->pcb);
			//sem_post(&mutexLista);

			log_info(logFile, "El programa con PCB:%i ha salido de la cola de"
					"bloqueados para ingresar a la cola Ready y ser abortado",pcbBloqueado->pcb->PID);
		}else
			queue_push(aux,pcbBloqueado);
	}
	colaBlock=aux;
	sem_post(&mutex_lista_pcb);
	sem_post(&mutexLista);
}

void planificarNuevos(int socketMemoria)
{
	t_PCB *aux;
	int inicializacion;



	while(!queue_is_empty(colaNew))
	{

		if(gradoMultiprog > cantProgsEnSistema)
		{
			sem_wait(&mutexLista);
			sem_wait(&mutex_lista_pcb);

			sem_wait(&progReady);
			cantProgsEnSistema++;
			sem_post(&progReady);

			aux = queue_pop(colaNew);

			inicializacion = inicializarPrograma(socketMemoria, aux,
				archC->STACK_SIZE);

			log_debug(logFile, "PCB Creado etiquetas_size= %d\n", aux->tamanioEtiquetas);

			if (inicializacion == 0)
			{
				//sem_wait(&mutexLista);
				aux->estado = READY;
				aux->punteroColaPlanif = colaReady;
				list_add(colaReady, aux);
				//sem_post(&mutexLista);

				list_add(listaPCB,aux);

				log_info(logFile, "[Planificacion] El programa con PID:%i ha salido de la cola de New "
						"para ingresar a la cola de listos.", aux->PID);

				log_info(logFile, "El programa con PID:%i ha ingresado al sistema "
						"en la lista de PCBs.\n", aux->PID);
			}//con los otros valores ya lo hacemos en inicializarPrograma (va a colaExit)

			sem_post(&mutex_lista_pcb);
			sem_post(&mutexLista);

		}else
			break;
	}
}

void replanificarQuantum()
{
	t_PCB* nPCB = queue_pop(aSerReplanificado);
	t_PCB* lPCB;


	bool _tieneElPid(t_PCB* programa)
	{
		return (programa->PID) == nPCB->PID;
	}

	sem_wait(&mutexLista);
	sem_wait(&mutex_lista_pcb);
	if(nPCB->estado == RUNNING)
	{
			//sem_wait(&mutexLista);
			t_PCB* programa = list_remove_by_condition(colaExec, (void*) _tieneElPid);
			//sem_post(&mutexLista);

			log_info(logFile, "El programa con pid:%i salio de la cola de Exec por "
					"finalizacion de quantum",programa->PID);

			//sem_wait(&mutex_lista_pcb);
				lPCB = list_find(listaPCB, (void*) _tieneElPid);
			//sem_post(&mutex_lista_pcb);

				lPCB->estado= READY;
				lPCB->punteroColaPlanif= (t_queue*)colaReady;
				if (lPCB->tablaHeap!=NULL)
					nPCB->tablaHeap = lPCB->tablaHeap;
				nPCB->estado= READY;
				nPCB->punteroColaPlanif= (t_queue*)colaReady;
				nPCB->abortarEjecucion=lPCB->abortarEjecucion;

				log_info(logFile, "El programa con pid:%i ingresara a la lista de"
					" Ready",nPCB->PID);

				log_info(logFile, "El programa con pid:%i reingreso a la lista de"
							" PCB con sus atributos actualizados",nPCB->PID);

			//sem_wait(&mutexLista);
				list_add(colaReady,nPCB);

			sem_post(&mutex_lista_pcb);
			sem_post(&mutexLista);

		}else{
			log_error(logFile, "El programa con PID:%i no pudo haber terminado su"
					" quantum porque no estaba ejecutandose",nPCB->PID);
		}

}

void planificarASerEliminados()
{
		t_PCB* lPCB;
		t_aSerEliminado* nPCB;// = malloc(sizeof(t_aSerEliminado));

		bool _tieneElPid(t_PCB* programa)
		{
			return (programa->PID) == nPCB->pcb->PID;
		}

		nPCB= list_get(aSerEliminado,0);

		sem_wait(&mutex_lista_pcb);
			lPCB = list_find(listaPCB, (void*) _tieneElPid);
		sem_post(&mutex_lista_pcb);

		if (lPCB->tablaHeap!=NULL)
			nPCB->pcb->tablaHeap = lPCB->tablaHeap;

		log_info(logFile, "El proceso con pid:%i sera eliminado", nPCB->pcb->PID);

		if(nPCB->pcb->estado == RUNNING && (nPCB->aviso == DEVOLUCION_CPU_KERNEL || nPCB->aviso == DEVOLUCION_CPU_CONSOLA || nPCB->aviso == FINALIZACION_CPU || nPCB->aviso == -1))
		{
			operacionKernel(FINALIZAR_PROCESO,nPCB->pcb->PID,nPCB->aviso, nPCB->exitcode);
		/*	if(enviarFinalizarProgramaAMemoria(socketMemoria,nPCB->pcb->PID)<=0){
						log_error(logFile, "Error al enviar mensaje finalizacion a Memoria socket= %d, pid:%i, se finalizo el programa incorrectamente", socketMemoria, nPCB->pcb->PID);
			}else{
				log_debug(logFile, "Se envio correctamente le finalizacion de programa, PID= %d, Socket Memoria= %d", nPCB->pcb->PID, socketMemoria);
			}*/
			list_remove(aSerEliminado,0);
		}else if(nPCB->pcb->estado!= RUNNING){
			operacionKernel(FINALIZAR_PROCESO,nPCB->pcb->PID,nPCB->aviso, nPCB->exitcode);
		/*	if(enviarFinalizarProgramaAMemoria(socketMemoria,nPCB->pcb->PID)<=0){
							log_error(logFile, "Error al enviar mensaje finalizacion a Memoria socket= %d, pid:%i, se finalizo el programa incorrectamente", socketMemoria, nPCB->pcb->PID);
			}else{
				log_debug(logFile, "Se envio correctamente le finalizacion de programa, PID= %d, Socket Memoria= %d", nPCB->pcb->PID, socketMemoria);
			}*/
			list_remove(aSerEliminado,0);
		}else{
			log_info(logFile, "El proceso con pid:%i no se puede finalizar aun porque se esta ejecutando en alguna CPU\n", nPCB->pcb->PID);
		}

}

int planificar(int socketMemoria)
{

	int cantCPU, i;
	t_cpu *cpuLista;

	while (1)
	{
		while (planificacion == true && !list_is_empty(listaCPU))
			{

					if(planificacion == true && !list_is_empty(aSerEliminado))
					{
						planificarASerEliminados(socketMemoria);
						//la planificacion de la cola exec la hacemos directo cuando termina
						//el quantum(case Exit) en sockets_select() y en terminacionIrregular()
					}
					//planificacion para procesos new
					if (planificacion == true && !queue_is_empty(colaNew))
					{
						planificarNuevos(socketMemoria);
					}

					//planificacion para procesos Ready
					if (planificacion == true && !list_is_empty(colaReady))
					{
						//en esta funcion hacemos la planificacion de los procesos Running
						algoritmo();

					}

					if(planificacion == true &&  !queue_is_empty(colaBlock))
					{
						verificarAbortados();
					//la planificacion de la cola block la hacemos directo en
					//operacionesConSemaforos(), esos semaforos son los unicos motivos
					//por los que se puede bloquear un proceso ESTO SE CAMBIO PARA PODER PAUSAR LA PLANIF

					}

					if(planificacion == true && !queue_is_empty(aSerReplanificado))
					{
						replanificarQuantum();
						//la replanificacion de fin de quantum la hacemos directo en sockets_select()
						// en el case Quantum
					}
			}
		}
	return 0;
}

void recorrerYMandar(t_cpu *cpuLista)
{
	int i;
	t_PCB *pcbAMandar;
	for (i = 0; i < list_size(colaExec); i++)
	{
		//sem_wait(&mutexLista);
		pcbAMandar = list_get(colaExec,i);
		//sem_post(&mutexLista);

		envioPCBACpuDisponible(cpuLista->socketC, pcbAMandar);

		cpuLista->PidEjecutando=pcbAMandar->PID;
		cpuLista->ocupado=true;

		log_info(logFile, "Se mando a ejecutar el PCB con PID:%i a la cpu "
				"con socket:%i\n", pcbAMandar->PID,cpuLista->socketC);
	}
}

void terminacionIrregular(t_PCB* pcb, uint32_t exitcode)
{
		switch (exitcode)
	{
	case FINALIZAR_CORRECTAMENTE:

		pcb->EC = 0;
		log_info(logFile, "El programa con pid:%i fue finalizado correctamente\n",pcb->PID);
		break;

	case NO_RESERVO_RECURSOS:

		pcb->EC = -1;
		log_error(logFile, "El programa con pid:%i fue finalizado porque "
				"no se pudieron reservar los recursos para ejecutarlo\n",pcb->PID);
		break;

	case ARCHIVO_INEXISTENTE:

		pcb->EC = -2;
		log_error(logFile,
				"El programa con pid:%i fue finalizado porque intento acceder a un archivo que "
				"no existe\n",pcb->PID);
		break;

	case LEER_SINPERMISOS:

		pcb->EC = -3;
		log_error(logFile, "El programa con pid:%i fue finalizado porqur intento leer un archivo"
				" sin permisos\n",pcb->PID);
		break;

	case ESCRIBIR_SINPERMISOS:

		pcb->EC = -4;
		log_error(logFile,
				"El programa con pid:%i fue finlizado por intentar escribir un archivo"
				" sin permisos\n",pcb->PID);
		break;

	case EXCEPCION_MEMORIA:

		pcb->EC = -5;
		log_error(logFile, "El programa con pid:%i fue finalizado por excepcion"
				"de memoria\n",pcb->PID);
		break;

	case DESCONEXION_CONSOLA:

		pcb->EC = -6;
		log_info(logFile, "El programa con pid:%i fue finalizado a traves de desconexion de "
				"consola\n",pcb->PID);
		break;

	case CONSOLA_FINALIZAR_PROGRAMA:

		pcb->EC = -7;
		log_info(logFile,
				"El programa con pid:%i fue finalizado a traves del comando Finalizar Programa de"
				" la consola\n",pcb->PID);
		break;

	case MAYOR_MEMORIA:
		pcb->EC = -8;
		log_error(logFile,"El programa con pid:%i fue finalizado porque"
				"intento reservar mas memoria que el tamanio de una pagina\n ",pcb->PID);
		break;

	case NO_ASIGNAR_PAGINAS:
		pcb->EC = -9;
		log_error(logFile, "El programa con pid:%i fue finalizado porque no se le pueden asignar "
				"mas paginas\n",pcb->PID);
		break;

	case STACKOVERFLOW:
		pcb->EC = -10;
		log_error(logFile,"El programa con pid:%i fue finalizado por un problema de stackoverflow"
				" en CPU\n",pcb->PID);
		break;

	case KERNEL_FINALIZAR_PROGRAMA:
		pcb->EC = -11;
		log_error(logFile, "El programa con pid:%i fue finalizado a traves del comando finalizar"
				" programa de Kernel\n",pcb->PID);
		break;

	case ERROR_HEAP:
		pcb->EC = -12;
		log_error(logFile,
				"El programa con pid:%i fue finalizado por un error en la memoria dinamica\n",
				pcb->PID);
		break;

	case CPU_SIGUSR1:
		pcb->EC = -13;
		log_error(logFile,"El programa con pid:%i fue finalizado por CPU\n",
				pcb->PID);
		break;

	default:
		pcb->EC = -20;
		log_error(logFile, "El programa con pid:%i fue finalizado por un error sin definicion\n",
				pcb->PID);
		break;

	}
	pcbAColaExit(pcb->PID);
}

void envioPCBACpuDisponible(int socketCPU, t_PCB * unPCB)
{
	header_t header;
	header.type=EXECUTE_PCB;

	char * serialized = serializer_pcb(unPCB,&header.length);

	//Pincha esto
	log_debug(logFile, "Tamanio Header del PCB: %d\n",header.length);

	sockets_send(socketCPU,&header,serialized);

}

void pcbAColaExit(uint32_t pcbId)
{
	int i, indice = -1;
	t_PCB *pcb;

	_Bool _tieneElPid(t_PCB *prog) {
		return (prog->PID == pcb->PID);
	}

	for (i = 0; i < list_size(listaPCB); i++)
	{
		sem_wait(&mutex_lista_pcb);
		pcb = list_get(listaPCB, i);
		sem_post(&mutex_lista_pcb);

		if (pcb->PID == pcbId)
		{
			indice = i;
			break;
		}
	}
	if (indice > -1) {
		t_PCB *pcbAEliminar;

		if (pcb->estado == READY)
		{
			sem_wait(&mutexLista);
			pcbAEliminar =list_remove_by_condition(colaReady, (void*) _tieneElPid);
			queue_push(colaExit, pcbAEliminar);
			sem_post(&mutexLista);

			pcb->estado=TERMINATED;
			pcb->punteroColaPlanif=colaExit;

			if (enviarFinalizarProgramaAMemoria(socketMemoria, pcb->PID) <= 0) {
				log_error(logFile,
						"Error al enviar mensaje finalizacion a Memoria socket= %d, pid:%i, se finalizo el programa incorrectamente",
						socketMemoria, pcb->PID);
			} else {
				log_debug(logFile,
						"Se envio correctamente le finalizacion de programa, PID= %d, Socket Memoria= %d",
						pcb->PID, socketMemoria);
			}

			sem_wait(&progReady);
			cantProgsEnSistema --;
			sem_post(&progReady);

			log_info(logFile, "El PCB con PID:%d pasa de cola Ready a cola Exit.\n",
					pcbId);

			memoryLeaks(pcb);

		} else if (pcb->estado == RUNNING )
		{
			sem_wait(&mutexLista);
			pcbAEliminar =list_remove_by_condition(colaExec, (void*) _tieneElPid);
			queue_push(colaExit, pcbAEliminar);
			sem_post(&mutexLista);

			pcb->estado=TERMINATED;
			pcb->punteroColaPlanif=colaExit;

			if (enviarFinalizarProgramaAMemoria(socketMemoria, pcb->PID) <= 0) {
				log_error(logFile,
						"Error al enviar mensaje finalizacion a Memoria socket= %d, pid:%i, se finalizo el programa incorrectamente",
						socketMemoria, pcb->PID);
			} else {
				log_debug(logFile,
						"Se envio correctamente le finalizacion de programa, PID= %d, Socket Memoria= %d",
						pcb->PID, socketMemoria);
			}

			sem_wait(&progReady);
			cantProgsEnSistema --;
			sem_post(&progReady);
			log_info(logFile, "el PCB %d pasa de cola Execute a cola Exit.\n",
					pcbId);

			memoryLeaks(pcb);

		} else {
			log_error(logFile,
					"El PCB: %d que se quiere eliminar no se encuentra en la cola EXECUTE"
					" ni en READY.\n",
					pcbId);
		}
	} else {
		log_error(logFile, "El PCB: %d que se quiere eliminar no puede ser eliminado"
				"desde su posicion actual.\n", pcbId);
	}

}

t_aSerEliminado * procesoASerAbortado(t_PCB* pcb)
{
	t_aSerEliminado* ePCB = malloc(sizeof(t_aSerEliminado));
	ePCB->exitcode = CONSOLA_FINALIZAR_PROGRAMA;
	ePCB->pcb = pcb;
	ePCB->aviso=FINALIZACION_CONSOLA;

	return ePCB;
}

void memoryLeaks(t_PCB* pcb)
{
	if(list_is_empty(pcb->tablaHeap->pagina))
	{
		log_info(logFile, "El programa con pid:%i no tiene Memory Leaks.\n",pcb->PID);
	}else
	{
		int pags=list_size(pcb->tablaHeap->pagina);
		int bytes=0;
		int bytesData=0;
		int i=0;

		for(i=0;i<pags;i++)
		{
			t_pagina_heap* aux=list_get(pcb->tablaHeap->pagina,i);
			bytesData= bytesData + tamanioPagina - aux->tam_disponible - (list_size(aux->bloques)*(sizeof(int)+sizeof(bool)));
			bytes= bytes + tamanioPagina - aux->tam_disponible;
		}
		log_info(logFile, "El programa con pid:%i tiene Memory Leaks. Cantidad de paginas sin liberar:%i. Cantidad de bytes sin liberar:%i. Cantidad de bytes de datos sin liberar:%i", pcb->PID, pags, bytes, bytesData);
	}
}
