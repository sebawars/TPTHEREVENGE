/*
 * consolaKernel.c
 *
 *  Created on: 23/6/2017
 *      Author: utnso
 */

#include "consolaKernel.h"

void obtenerListado(int pid)
{
	int i;
	int cant;

	if (pid == 1)
	{
		//todos
		sem_wait(&mutex_lista_pcb);
		cant=list_size(listaPCB);
		int canti=queue_size(colaNew);
		if(cant!=0 || canti!=0)
		{
			printf("\n >>>Los procesos del sistema son:\n >>>");

			for (i = 0; i <cant ; i++)
			{
				t_PCB* aux = list_get(listaPCB, i);
				printf("-PID:%i ", aux->PID);
			}

				t_queue*colaAux=colaNew;
				for (i = 0; i<canti ; i++)
				{
					t_PCB * aux = queue_pop(colaAux);
					printf("-PID:%i ", aux->PID);
					queue_push(colaNew, aux);
				}

			printf("\n");
		}else{
			printf("\n >>>No hay procesos en el sistema.\n\n");
		}
		sem_post(&mutex_lista_pcb);

	}else if (pid ==2)
	{
		//New
		sem_wait(&mutexLista);
		cant=queue_size(colaNew);

		if(cant!=0)
		{
			printf("\n >>>Los procesos del sistema en estado New son:\n\n");
			t_queue*colaAux=colaNew;

			for (i = 0; i <cant ; i++)
			{
				t_PCB * aux = queue_pop(colaAux);
				printf("-PID:%i ", aux->PID);
				queue_push(colaNew,aux);
			}
			printf("\n");
		}else{
			printf("\n >>>No hay procesos en estado New.\n >>>");
			}
		sem_post(&mutexLista);
	}else if (pid ==3)
	{
		//Ready
		sem_wait(&mutexLista);
		cant=list_size(colaReady);

		if(cant!=0)
		{
			printf("\n >>>Los procesos del sistema en estado Ready son:\n >>>");

			for (i = 0; i <cant ; i++)
			{
				t_PCB * aux = list_get(colaReady, i);
				printf("-PID:%i ", aux->PID);
			}
			printf("\n");

		}else{
			printf("\n >>>No hay procesos en estado Ready.\n\n");
			}
		sem_post(&mutexLista);
	}else if (pid ==4)
	{
		//Exec
		sem_wait(&mutexLista);
		cant=list_size(colaExec);

		if(cant!=0)
		{
			printf("\n >>>Los procesos del sistema en estado Exec son:\n >>>");

			for (i = 0; i <cant ; i++)
			{
				t_PCB * aux = list_get(colaExec, i);
				printf("-PID:%i ", aux->PID);
			}
			printf("\n");

		}else{
			printf("\n >>>No hay procesos en estado Exec.\n\n");
			}
		sem_post(&mutexLista);
	}else if (pid ==5)
	{
		//Block
		sem_wait(&mutexLista);
		cant=queue_size(colaBlock);

		if(cant!=0)
		{
			printf("\n >>>Los procesos del sistema en estado Block son:\n >>>");

			t_queue*auxcolaBlock=colaBlock;

			for (i = 0; i <cant ; i++)
			{
				t_PCB * aux = queue_pop(auxcolaBlock);
				printf("-PID:%i ", aux->PID);
				queue_push(colaBlock, aux);
			}
			printf("\n");
		}else{
			printf("\n >>> No hay procesos en estado Block.\n >>>");
			}
		sem_post(&mutexLista);
	}else if (pid ==6)
	{
		//Exit
		sem_wait(&mutexLista);
		cant=queue_size(colaExit);

		if(cant!=0)
		{
			printf("\n >>> Los procesos del sistema en estado Exit son:\n >>>");

			t_queue*auxExit=colaExit;

			for (i = 0; i <cant ; i++)
			{
				t_PCB * aux = queue_pop(auxExit);
				printf("-PID:%i", aux->PID);
				queue_push(colaExit, aux);
			}
			printf("\n");
		}else{
			printf("\n >>>No hay procesos en estado Exit.\n");
			}
		sem_post(&mutexLista);
	}else {
		printf("\n >>>No se pueden listar los procesos del sistema porque no existe el "
				"estado especificado\n");
	}

}

void operacionKernel(uint32_t operacion, int pid, int grado, uint32_t exitcode)
{
	t_PCB *pcb = malloc(sizeof(t_PCB));
	header_t header; //NUEVO

	int i;

	_Bool _tieneElPid(t_PCB *prog) {
		return (prog->PID == pid);
	}

	switch (operacion) {

	case OBTENER_LISTADO:

		obtenerListado(pid);

		break;

	case CONSULTAR_ESTADO:

		pcb = list_find(listaPCB, (void*) _tieneElPid);
	if(pcb!= NULL)
	{
		printf("\n >>> El estado del proceso con PID %d es %d\n", pcb->PID,
				pcb->estado);

		printf("\n >>> La cantidad de rafagas del proceso con PID %d es %d\n",
				pcb->PID,pcb->rafagas);

		t_cantSyscalls * syscall=list_find(tablaSyscalls, (void*)_tieneElPid);
		printf("\n >>> El proceso con pid:%i realizó las siguientes operaciones privilegiadas:\n", pcb->PID);
		printf("Abrir Archivos:%i veces.\n", syscall->cantAbrir);
		printf("Cerrar Archivos:%i veces.\n", syscall->cantCerrar);
		printf("Leer Archivos:%i veces.\n", syscall->cantLeer);
		printf("Escribir Archivos:%i veces.\n", syscall->cantEscribir);

		printf("\n >>> La tabla de archivos abiertos es:\n");
		archivosAbiertosPorProceso(pid);

		t_cantPagsHeap* pagsHeap=list_find(tablaHeap, (void*)_tieneElPid);
		printf("\n >>> El proceso con PID: %d  utilizo %d paginas de Heap\n",	pcb->PID, pagsHeap->cantPagsHeap);
		printf("Aloco %i bloques con un total de %i bytes.", pagsHeap->alocarCant, pagsHeap->alocarBytes);
		printf("Libero %i bloques con un total de %i bytes.\n", pagsHeap->liberarCant,pagsHeap->liberarBytes);

	}else{
		printf("\n >>> No se puede obtener los datos del programa indicado porque no existe en el sistema\n\n");
	}
		break;

	case OBTENER_TABLA_GLOBAL:

		sem_wait(&mutex_GFT);
		if(!list_is_empty(tablaGFT))
		{
		printf("La Tabla Global del sistema es:\n");

		for (i = 0; i < list_size(tablaGFT); i++)
		{
			t_entradaPorGFT * aux = list_get(tablaGFT, i);
			printf("GFD:%d, Open:%i, Path:%s \n", aux->globalFD,aux->open, aux->path);
		}
		}else{
			printf("\n >>> La Tabla Global del sistema esta vacia.\n");
		}
		sem_post(&mutex_GFT);

		break;

	case MODIFICAR_GRADO_MULTIPROG:

		printf("\n >>>El grado de multiprogramación se cambió por el comando de "
				"consola de kernel de: %d a :%d \n\n",
				gradoMultiprog, grado);
		gradoMultiprog=grado;

		break;

	//Verificar este Case porque no es lo mismo una Finalizacion de programa Correcta (EXIT)
	//Hay que crear un Case EXIT que sera la finalizacion exitosa de un proceso
	case FINALIZAR_PROCESO:

		pcb = list_find(listaPCB, (void*) _tieneElPid);

		if(pcb!=NULL)
		{
			printf("\n >>>Se finalizará el programa con pid:%i.\n", pid);
			if(pcb->estado == TERMINATED)
			{
				log_error(logFile, "No se puede finalizar el proceso con pid:&i porque ya fue "
							"finalizado previamente\n", pcb->PID);
			}

			if(grado == FINALIZACION_KERNEL || grado == FINALIZACION_CPU || grado == GENERICO)
			{
				int bytes= enviarPid(pcb->socket, pcb->PID,FINALIZAR_PROGRAMA);

				if (bytes!=-1)
					log_info(logFile, "Se ha enviado un mensaje a la consola:%i para que finalice el programa con pid:%i\n",pcb->socket,pcb->PID);
				else
					log_error(logFile, "Ocurrio un error al enviar un mensaje a la consola:%i para que finalice el programa con pid:%i\n",pcb->socket,pcb->PID);

			}

			terminacionIrregular(pcb, exitcode);
		}else{
			printf("\n >>>No se puede finalizar el programa con pid:%i porque no existe en el sistema.\n", pid);
		}
		break;

	case DETENER_PLANIFICACION:

		log_info(logFile, "se detendrá la planificación de los procesos por el comando en la "
				"consola del Kernel\n");
		printf("\n >>>Se detendrá la planificación de los procesos por el comando en la "
				"consola del Kernel\n");
		planificacion = false;

		break;

	case REANUDAR_PLANIFICACION:

		log_info(logFile, "se reanudará la planificación de los procesos por el comando en la "
				"consola del Kernel\n");
		printf("\n >>> Se reanudará la planificación de los procesos por el comando en la "
				"consola del Kernel\n");

		planificacion=true;

		break;
	}
}

void consolaKernel()
{

	int cantLeido,bytes;

	while(1)
	{
		char* mensaje = malloc(50);
		char* spid= malloc(10);

		printf(">>INGRESE EL NUMERO DEL COMANDO DE CONSOLA DE KERNEL\n");
		printf(
				"1-Obtener Listado\n "
				"2-Consultar Estado\n "
				"3-Obtener Tabla Global \n "
				"4-Modificar Grado Multiprogramacion\n "
				"5-Finalizar Proceso\n "
				"6-Detener Planificacion\n "
				"7-Reanudar Planificacion\n");

		cantLeido = scanf("%s", mensaje);

		if (strcmp(&mensaje[0], "1") == 0)
		{

			printf(">>Ingresar el estado que quiere consultar:\n"
					"1-Todos\n "
					"2- New\n "
					"3-Ready\n "
					"4-Exec\n "
					"5-Block\n "
					"6-Exit\n");
					scanf("%s",spid);
					int pid=atoi(spid);

					operacionKernel(OBTENER_LISTADO, pid, 0, 0);

			} else if (strcmp(&mensaje[0], "2") == 0) {
					printf(">>Ingresar el pid del programa que quiere consultar\n");
					scanf("%s",spid);
					int pid=atoi(spid);
					operacionKernel(CONSULTAR_ESTADO, pid, 0, 0);

			} else if (strcmp(&mensaje[0], "3") == 0) {

				operacionKernel(OBTENER_TABLA_GLOBAL, 0, 0, 0);

			} else if (strcmp(&mensaje[0], "4") == 0) {
				printf(">>Ingresar el nuevo grado de multiprogramacion\n");
						scanf("%s",spid);
						int grado= atoi(spid);

				operacionKernel(MODIFICAR_GRADO_MULTIPROG, 0, grado, 0);

			} else if (strcmp(&mensaje[0], "5") == 0) {
				printf(">>Ingresar el pid que se quiere finalizar\n");
				scanf("%s",spid);
				int pid= atoi(spid);

				bool _tieneElPid(t_PCB* programa)
				{
					return (programa->PID) == pid;
				}

				t_PCB* lPCB = list_find(listaPCB, (void*)_tieneElPid);

				if(lPCB != NULL && !lPCB->abortarEjecucion && lPCB->estado!=TERMINATED){
					bytes=enviarPid(lPCB->socket, lPCB->PID,FINALIZAR_PROGRAMA);
					if(bytes!=-1)
						log_info(logFile, "Se ha enviado un mensaje a la consola:%i para que aborte el programa con pid:%i",lPCB->socket,lPCB->PID);
					else
						log_error(logFile, "Ocurrio un error al enviar un mensaje a la consola:%i para que finalice el programa con pid:%i",lPCB->socket,lPCB->PID);

					lPCB->abortarEjecucion=true;
					log_info(logFile, "El programa con pid:%i sera finalizado por comando Kernel ",pid);
				}else{
					printf(">>El proceso no esta en ejecucion o no Existe en el Kernel\n\n");
					log_warning(logFile, "El programa con pid:%i no esta en ejecucion o fue finalizado previamente ",pid);
				}
				//t_aSerEliminado* ePCB = malloc(sizeof(t_aSerEliminado));
				//ePCB->exitcode = KERNEL_FINALIZAR_PROGRAMA;
				//ePCB->pcb->PID=pid;
				//ePCB->aviso=FINALIZACION_KERNEL;

				//list_add(aSerEliminado, ePCB);

			} else if (strcmp(&mensaje[0], "6") == 0) {

				operacionKernel(DETENER_PLANIFICACION, 0, 0, 0);

			} else if (strcmp(&mensaje[0], "7") == 0) {

				operacionKernel(REANUDAR_PLANIFICACION, 0, 0, 0);

			}else{
				printf("Se ingreso un codigo invalido \n");
			}

		free(mensaje);
		free(spid);
	}


}

int crearHiloConsolaKernel()
{
	int valorHilo = -1;
	valorHilo = pthread_create(&hiloConsolaKernel, NULL, (void*) consolaKernel,
	NULL);
	if (valorHilo != 0) {
		perror(">>Error al crear hilo de Consola de Kernel\n\n");
		log_error(logFile, "error al crear el hilo de Consola de Kernel\n");
		exit(1);
	} else {
		log_info(logFile, "el hilo de Kernel se creó correctamente\n");
	}
	return valorHilo;

}

void archivosAbiertosPorProceso(uint32_t pid){
	t_list* listaPorPID;
	int i;
	_Bool _tieneElPid(t_PCB *prog) {
			return (prog->PID == pid);
		}
	sem_wait(&mutex_PFT);
	listaPorPID=list_filter(tablaPFT,(void*)_tieneElPid);

	for (i = 0; i < list_size(tablaPFT); i++)
	{
		t_entradaPorPFT * aux = list_get(listaPorPID, i);
		log_info(logFile, "PID:%d FD:%d, Cursor:%d,Permisos: C=%d R=%d W=%D ,GFD:%d \n", aux->pid,aux->fd,aux->cursor,aux->flags.creacion,aux->flags.lectura,aux->flags.escritura,aux->globalFD);
	}
	sem_post(&mutex_PFT);

}
