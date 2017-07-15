/*
 * conexiones.c

 *
 *  Created on: 28/5/2017
 *      Author: utnso
 */

#include "conexiones.h"

int recvPid(int socket, uint32_t length)
{
	char* spid;
	spid = malloc(length);

	int pidAEliminar=recv(socket, spid, length, MSG_WAITALL);

	if(pidAEliminar==0){
	//	printf("hubo un error al recibir el pid a eliminar\n");
		log_info(logFile, "hubo un error al recibir el pid a eliminar \n");
	}

	uint32_t *pid=uint32_deserializer(spid);

	return(*pid);

	free(pid);
	free(spid);
}


int demasiadosClientes(int socket)
{
//	printf("Demasidos clientes conectados, se cierra socket de conexion\n");
	log_info(logFile, "Demasidos clientes conectados, se cierra socket de conexion\n");
	//log_info(logFile,"Se cierra socket de conexion: %s, grado de Multiprogramacion excedido",socket);
	close(socket);
	return 0;
}

void agregarCPULibre(sem_t *mutexCPU, t_list *listaCPU, int sockFd, sem_t *CPULibres)
{
	//nueva estructura
	t_cpu *cpuNueva;
	cpuNueva = malloc(sizeof(t_cpu));
	cpuNueva->socketC=sockFd;
	cpuNueva->ocupado=0;
	cpuNueva->PidEjecutando=NULL;

	sem_wait(mutexCPU);
	list_add(listaCPU, cpuNueva); //Agrega el socket del CPU a la lista de CPU disponibles
	sem_post(mutexCPU);
	sem_post(CPULibres); //Aumento la cantidad de CPU disponibles
}


int atiendeCpu(int socket, uint32_t tamStack)
{
	header_t header;
	uint32_t *notificacion_tamanioStack = malloc(sizeof(uint32_t));
	header.type = TAMANIO_STACK;
	int nbytes = 0;

	if (!aceptarConexion(socket, HANDSHAKE_CPU)) {
		log_info(logFile, "Conexion con CPU establecida, Socket: %d.", socket);
		enviarHandshake(socket, HANDSHAKE_KERNEL);

		*notificacion_tamanioStack = tamStack;

		char *serialized = uint32_serializer(notificacion_tamanioStack,
				&header.length);

		nbytes = sockets_send(socket, &header, serialized);

		agregarCPULibre(&mutexCPU, listaCPU, socket, &CPULibres);
		free(notificacion_tamanioStack);
		free(serialized);
	} else {
		log_warning(logFile,
				"Mensaje inesperado de nueva conexion, en Socket CPU");
		close(socket);
		return -1;
	}
	return nbytes;
}

void borrarCPU(int socketCPU)
{
	t_cpu * lcpu;

	bool _obtenerCPU (t_cpu * cpu){
		return (cpu->socketC) == socketCPU;
	}

	sem_wait(&mutexCPU);
	lcpu = list_remove_by_condition(listaCPU,(void*) _obtenerCPU);

	if (lcpu != NULL){
		free(lcpu);
		log_info(logFile, "se elimino la CPU con socket:%i del sistema",socketCPU);
	}else
		log_info(logFile, "La CPU ya fue eliminada de la lista previamente, "
				"seguramente se recibio SIGUSR1",socketCPU);

	sem_post(&mutexCPU);
}

int esFDCPU(int sockFd)
{
	int i;
	for (i = 0; i < (listaCPU->elements_count); i++) {
		if (sockFd ==(int) list_get(listaCPU, i))
			return 1;

	}
	return 0;
}

int esFDConsola(int sockFd)
{
	int i;
	for (i = 0; i < (listaConsola->elements_count); i++) {
		if (sockFd == (int) list_get(listaConsola, i))
			return 1;

	}
	return 0;
}

void borrarConsola(int socketConsola)
{
	uint16_t pos = 0;
	int aux;
	bool encontro = false;

	sem_wait(&mutexConsola);
	while ((pos < listaConsola->elements_count) && (!encontro)) {
		aux = (int) list_get(listaConsola, pos);
		if (aux == socketConsola)
			encontro = true;
		else
			pos++;
	}
	if (encontro)
		list_remove(listaConsola, pos);
	sem_post(&mutexConsola);
	log_info(logFile,"se ha eliminado la consola con socket:%i del sistema\n",socketConsola);
}

void operacionPCB(int i, header_t header, uint32_t exitcode)
{
	char* mensaje;
	t_PCB* nPCB;

	resetarCPU(i);

	bool _tieneElPid(t_PCB* programa)
	{
		return (programa->PID) == nPCB->PID;
	}

	mensaje =malloc(header.length);

	recv(i, mensaje, header.length, 0);

	nPCB= deserializer_pcb(mensaje);

	t_PCB* lPCB = list_find(listaPCB, (void*)_tieneElPid);
	if(lPCB->tablaHeap!=NULL)
		nPCB->tablaHeap=lPCB->tablaHeap;
	nPCB->abortarEjecucion=lPCB->abortarEjecucion;

	if(exitcode == FINALIZAR_CORRECTAMENTE && !nPCB->abortarEjecucion)
	{
		enviarPid(nPCB->socket, nPCB->PID,EXIT);
	}

	//Esto lo comento dado que no le veo mucho sentido
	//lPCB=nPCB;
	if(exitcode != FINALIZAR_CORRECTAMENTE && !nPCB->abortarEjecucion)
	{
		t_aSerEliminado* ePCB=malloc(sizeof(t_aSerEliminado));
		ePCB->exitcode=exitcode;
		ePCB->pcb=nPCB;
		ePCB->aviso=FINALIZACION_CPU;

		log_info(logFile, "El programa con pid:%i sera finalizado por accion de CPU, el pcb no esta en la cola de aSerEliminados,:%i",ePCB->pcb->PID);

		list_add(aSerEliminado, ePCB);

	}else if(!nPCB->abortarEjecucion && exitcode==FINALIZAR_CORRECTAMENTE){

		t_aSerEliminado* ePCB=malloc(sizeof(t_aSerEliminado));
		ePCB->exitcode=exitcode;
		ePCB->pcb=nPCB;
		//Esto se pone asi para luego no mandar mas mensaje a Consola
		ePCB->aviso=-1;

		log_info(logFile, "El programa con pid:%i sera finalizado Correctamente\n",ePCB->pcb->PID);

		list_add(aSerEliminado, ePCB);

	}else{
		//t_aSerEliminado* ePCB =list_find(aSerEliminado,(void*)_yaLoQuerianFinalizar);
		//ePCB->pcb=nPCB;

		if(nPCB->EC==KERNEL_FINALIZAR_PROGRAMA)
		{
			t_aSerEliminado* ePCB = malloc(sizeof(t_aSerEliminado));
			ePCB->exitcode = KERNEL_FINALIZAR_PROGRAMA;
			ePCB->pcb = nPCB;
			ePCB->aviso=DEVOLUCION_CPU_KERNEL;
			log_info(logFile, "El programa con pid:%i sera finalizado, entrara a la cola de aSerEliminados por accion de consola de Kernel\n",ePCB->pcb->PID);

			list_add(aSerEliminado, ePCB);

		}else if(nPCB->EC==CONSOLA_FINALIZAR_PROGRAMA)
		{	t_aSerEliminado* ePCB = malloc(sizeof(t_aSerEliminado));
			ePCB->exitcode = CONSOLA_FINALIZAR_PROGRAMA;
			ePCB->pcb = nPCB;
			ePCB->aviso=DEVOLUCION_CPU_CONSOLA;
			log_info(logFile, "El programa con pid:%i sera finalizado, entrara a la cola de aSerEliminados por accion de Consola",ePCB->pcb->PID);

			list_add(aSerEliminado, ePCB);

		}
	}
}

void resetarCPU(int socketCPU)
{
	t_cpu* cpu;

	bool _tieneLaCpu(t_cpu* cpu)
	{
			return (cpu->socketC) == socketCPU;
	}

	sem_wait(&mutexCPU);
	cpu = list_find(listaCPU,(void*) _tieneLaCpu);
	if(cpu != NULL){
		cpu->ocupado=false;
		cpu->PidEjecutando=-1;
	}
	sem_post(&mutexCPU);
}

int tieneElPidParaColas(t_queue* cola, int pid)
{
	int i;
	int encontro=-1;
	t_queue* aux=cola;
	t_PCB* pcbaux;
	for(i=0;i<queue_size(cola);i++)
	{
		pcbaux= queue_pop(aux);
		if(pcbaux->PID==pid)
		{
			encontro=i;
		}
		queue_push(cola,pcbaux);
	}
	return encontro;
}

t_PCB* queue_get(t_queue* cola, int pid)
{
	int i;
	int encontro=-1;
	t_queue* aux=cola;
	t_PCB* nPCB;
	for(i=0;i<queue_size(cola);i++)
	{
		t_PCB* pcbaux= queue_pop(cola);
		if(pcbaux->PID!=pid)
		{
			queue_push(aux,pcbaux);
		}else{
			nPCB=pcbaux;
		}
	}
	cola=aux;
	return nPCB;
}

void queue_add_index(t_queue* cola, t_PCB* pcb, int indice)
{
	int i;
	int size= queue_size(cola);
	t_queue* aux=cola;

	for(i=0;i<indice;i++)
	{
		t_PCB* pcbaux= queue_pop(cola);
		queue_push(aux,pcbaux);
	}

	queue_push(aux,pcb);

	for(i=0;i<(size-indice);i++)
	{
		t_PCB* pcbaux= queue_pop(cola);
		queue_push(aux,pcbaux);
	}
	cola=aux;
}

int sockets_select(archivoConfigKernel *configuracion, int socketMemoria)
{

	fd_set setAux, read_fds;
	header_t header;
	int j;
	int maxFD, i, socketNuevo, resultado, socketConsolas, socketCpu;
	offset = 0;

	t_PCB* proceso;
	t_PCB* nPCB;
	t_cantSyscalls* syscalls;
	uint32_t tamStack;
	char * codigo;

	char* mensaje = NULL;

	tamStack = configuracion->STACK_SIZE;

	FD_ZERO(&read_fds); 	// borra los conjuntos maestro y temporal
	FD_ZERO(&setAux);

	socketConsolas = sockets_getSocket();

	if (sockets_bind(socketConsolas, configuracion->PUERTO_PROG) != 0) {
		log_error(logFile, "Fallo el Bind con Consolas\n");
		return 1;
	}

	if (listen(socketConsolas, configuracion->backlog) == -1) {
		log_error(logFile,
				"No se puede escuchar en el puerto especificado para Consola\n");
		return 2;
	}

	socketCpu = sockets_getSocket();

	if (sockets_bind(socketCpu, configuracion->PUERTO_CPU) != 0) {
		log_error(logFile, "Fallo el Bind con CPU\n");
		return 1;
	}

	if (listen(socketCpu, configuracion->backlog) == -1) {
		log_error(logFile,
				"No se puede escuchar en el puerto especificado para CPU\n");
		return 2;
	}

	maxFD = socketCpu; //Llevo control del FD maximo de los sockets
	FD_SET(socketConsolas, &read_fds); //agrego el FD del socketConsolas al setMaestro
	FD_SET(socketCpu, &read_fds); //agrego el FD del socketCpu al setMaestro
	FD_SET(file_descriptor,&read_fds); // FD de inotify

	while (1) {
		setAux = read_fds;
		log_info(logFile,
				"Servidor en escucha,Puerto Consolas: %d, Puerto CPU: %d\n",
				configuracion->PUERTO_PROG, configuracion->PUERTO_CPU);

		if (select((maxFD + 1), &setAux, NULL, NULL, NULL) == -1) {

			log_error(logFile, "Error en la escucha de Consolas\n");
			return EXIT_FAILURE;
		}
		//Recorro las conexiones en busca de interacciones nuevas
		for (i = 0; i <= maxFD; i++) {
			if (FD_ISSET(i, &setAux)) {

				if (i == file_descriptor) {

					length_inotify = read(file_descriptor, buf, BUF_LEN);
					if (length_inotify < 0) {
						log_error(logFile,
								"Error al leer el archivo de Configuracion");
					}
					while (length_inotify > offset) {

						verificarModificacion();

					}

					inotify_rm_watch(file_descriptor, watch_descriptor);

					FD_CLR(file_descriptor, &read_fds);

					iniciarInotify();

				}

				// Me fijo en el set de descriptores a ver cual respondió
				else if (i == socketConsolas) { //Tengo una nueva Consola queriendose conectar
					socketNuevo = sockets_accept(socketConsolas);
					if (socketNuevo == -1) {
						return EXIT_FAILURE;
					}
					FD_SET(socketNuevo, &read_fds); //agrego el nuevo socket al setMaestro

					if (socketNuevo > maxFD)
						maxFD = socketNuevo; //mantengo actualizado el maxFD
					log_info(logFile, "Nueva Consola conectada, socket nº %d\n",
							socketNuevo);
					aceptarConexionConsola(socketNuevo);
					list_add(listaConsola, socketNuevo);
					log_info(logFile, "Actualmente hay :%i Consolas en el sistema.\n", list_size(listaConsola));
				} else if (i == socketCpu) { //Tengo un nuevo CPU queriendose conectar
					socketNuevo = sockets_accept(socketCpu);
					if (socketNuevo == -1) {
						return EXIT_FAILURE;
					}
					FD_SET(socketNuevo, &read_fds); //agrego el nuevo socket al setMaestro

					if (socketNuevo > maxFD)
						maxFD = socketNuevo; //mantengo actualizado el maxFD

					log_info(logFile, "Nuevo CPU conectado, socket nº %d\n",
							socketNuevo);
					atiendeCpu(socketNuevo, tamStack);
					log_info(logFile, "Actualmente hay :%i CPUs en el sistema.\n", list_size(listaCPU));

				} else { // Hay actividad nueva en alguna Conexión
					resultado = recv(i, &header, sizeof(header_t), 0);

					if (resultado == 0) { // si resultado==0 es porque se cerro una conexión
										  //programaDesconectado(i);
						if (esFDCPU(i)) {
							log_info(logFile,
									"Se desconectó un CPU que tenía el socket %i \n",
									i);
							log_info(logFile, "Actualmente hay :%i CPUs en el sistema.\n", list_size(listaCPU));
							borrarCPU(i);
						}

						if (esFDConsola(i)) {
							log_info(logFile,
									"Se desconectó la Consola que tenía el socket %i\n",
									i);
							log_info(logFile, "Actualmente hay :%i CPUs en el sistema.\n", list_size(listaCPU));
							borrarConsola(i);
						}

						log_info(logFile,
								"se cerró una conexión en el socket: i= %d", i);
						FD_CLR(i, &read_fds); // borra el file descriptor del set
						close(i); // cierra el file descriptor

					} else {
						if (resultado < 0) {
							return EXIT_FAILURE; //error al recibir
						}

						switch (header.type) {

						int pid;

					case INICIAR_PROGRAMA:

						//Se elimino del tipo de dato t_pcb la variable contenido
						codigo = recibirProgramaAnsisop(i, header.length);
					//	printf("Script: %s", codigo);

						log_info(logFile, "Script: %s", codigo);

						proceso = crearPCB(codigo, i);

						//planificar(socketMemoria, proceso,tamStack);
						//Esto lo comente porque se dispara planificar como un hilo en la funcion crearHiloPlanificacion en kernel.c
						//planificar(socketMemoria);
						break;

					case FINALIZAR_PROGRAMA:
						//finalizar programa viene con el pid del programa a eliminar

						pid = recvPid(i, header.length);

						bool _tieneElPid(t_PCB* prog) {

							return (prog->PID) == pid;

						}

						sem_wait(&mutexLista);
						sem_wait(&mutex_lista_pcb);
						if (list_any_satisfy(listaPCB,(void*)_tieneElPid)){

							nPCB=list_find(listaPCB,(void*)_tieneElPid);
							nPCB->abortarEjecucion=true;
							nPCB->EC=CONSOLA_FINALIZAR_PROGRAMA;
							log_info(logFile, "Se recibio un mensaje de finalizacion abortiva, el proceso esta"
									"en el flujo de planificacion al llegar a la cola Ready sera finalizado el proceso con PID: %i", nPCB->PID);
						}else if(tieneElPidParaColas(colaNew, pid) != -1)
						{
							int indice=tieneElPidParaColas(colaNew, pid);
							nPCB=queue_get(colaNew,pid);
							nPCB->abortarEjecucion=true;
							nPCB->EC=CONSOLA_FINALIZAR_PROGRAMA;
							queue_add_index(colaNew,nPCB,indice);

							log_info(logFile, "Se recibio una finalizacion abortiva de Consola, el proceso con pid:%i esta"
									"en la colaNew se forzara el envio a la cola ready para su finalizacion", nPCB->PID);
						}else
							log_warning(logFile, "el proceso con pid:%i no pordra se finalizado abortivamente"
									"porque ya fue finalizado", pid);

						sem_post(&mutex_lista_pcb);
						sem_post(&mutexLista);
						/*else if (list_any_satisfy(aSerEliminado,(void*)_tieneElPid))
						{
							nPCB=list_find(aSerEliminado,(void*)_tieneElPid);
							log_info(logFile, "Se recibio una finalizacion abortiva, el proceso esta"
									"en la colaExec %i", nPCB->PID);
							finalizarProcesoDesdeConsola(nPCB, i);

						}else if(tieneElPidParaColas(colaNew, pid) != -1)
						{
							nPCB=queue_get(colaNew,pid);
							log_info(logFile, "Se recibio una finalizacion abortiva, el proceso esta"
									"en la colaNew %i", nPCB->PID);
							finalizarProcesoDesdeConsola(nPCB, i);

							sem_wait(&mutexNew);
							nPCB=queue_get(colaNew,pid);
							sem_post(&mutexNew);

							sem_wait(&mutexNew);
							list_remove_by_condition(listaProgramas,(void*)_elProgramaTieneElPid);
							sem_post(&mutexNew);

							nPCB->estado=TERMINATED;
							nPCB->EC = CONSOLA_FINALIZAR_PROGRAMA;
							nPCB->punteroColaPlanif= colaExit;

							sem_wait(&mutexLista);
							queue_push(colaExit,nPCB);
							sem_post(&mutexLista);

							log_info(logFile, "se finalizó el programa con pid%i\n",
									proceso->PID);

							//La consola no espera un Okey
							//header.type=SUCCESS;
							//header.length=0;
							//sockets_send(i,&header,'\0');

						}else if(tieneElPidParaColas(colaBlock, pid) != -1)
						{
							nPCB=queue_get(colaBlock,pid);
							log_info(logFile, "Se recibio una finalizacion abortiva, el proceso esta"
									"en la colaBlock %i", nPCB->PID);
							finalizarProcesoDesdeConsola(nPCB, i);

						}else
							log_error(logFile, "el proceso con pid:%i no puede ser finalizado por comando de consola de Consola porque ya fue finalizado anteriormente\n.", pid);

						else if(tieneElPidParaColas(aSerReplanificado, pid) != -1)
						{
							nPCB=queue_get(aSerReplanificado,pid);
							log_info(logFile, "Se recibio una finalizacion abortiva, el proceso esta"
									"en estado de replanificacion %i", nPCB->PID);
							finalizarProcesoDesdeConsola(nPCB, i);

						}else if(tieneElPidParaColas(aSerBloqueado, pid) != -1)
						{
							nPCB=queue_get(aSerBloqueado,pid);
							log_info(logFile, "Se recibio una finalizacion abortiva, el proceso esta"
									"en estado de ser bloqueado %i", nPCB->PID);
							finalizarProcesoDesdeConsola(nPCB, i);
						}*/

						break;

					case QUANTUM_TERMINADO:
						//Quantum terminado replanificar pcb

						//Se libera CPU para reutilizacion
						resetarCPU(i);

						mensaje = malloc(header.length);

						recv(i, mensaje, header.length, MSG_WAITALL);

						nPCB = deserializer_pcb(mensaje);

						queue_push(aSerReplanificado, nPCB);

						log_debug(logFile,
								"Se finalizara el Quantum del proceso con "
										"pid:%i, por la CPU: %i", nPCB->PID, i);

						break;

					case EXIT:
						//Programa finalizado correctamente desde CPU
						log_debug(logFile,
								"Se finalizo correctamente la ejecucion"
										"de un programa desde la CPU: %i", i);

						operacionPCB(i, header, FINALIZAR_CORRECTAMENTE);

						break;

						//solicitud lectura variable compartida
					case LEER_VAR_COMPARTIDA:
						//Siempre vamos a tener que recibir el header.length y luego ver que mas se necesita
						log_debug(logFile,
								"Se recibio una solicitud de Lectura de una Variable Compartida por parte del CPU: %i",
								i);
						operacionConVariableCompartida(i, header.length,
								LEER_VAR_COMPARTIDA);
						break;

						//solicitud asignacion variable compartida
					case ASIGNAR_VAR_COMPARTIDA:
						//grabarValorVariableCompartida(i,varCompartidas,valorCompartidas,mutexCompartidas);
						log_debug(logFile,
								"Se recibio una solicitud de asignacion de una Variable Compartida por parte del CPU: %i",
								i);
						operacionConVariableCompartida(i, header.length,
								ASIGNAR_VAR_COMPARTIDA);
						break;

					case WAIT:
						//Opereacion con Semaforos
						log_debug(logFile, "Solicitud de Wait, CPU: %i", i);
						operacionesConSemaforos(i, header.length, WAIT);
						break;

					case SIGNAL:
						//Operacion con semaforos
						log_debug(logFile, "Solicitud de Signal, CPU: %i", i);
						operacionesConSemaforos(i, header.length, SIGNAL);
						break;

					case ALOCAR:
						//Operacines HEAP
						log_debug(logFile, "Solicitud Alocar, CPU: %i", i);
						operacionesHeap(i, header.length, ALOCAR,
								socketMemoria);
						break;

					case LIBERAR:
						//Operacines HEAP
						log_debug(logFile, "Solicitud Liberar, CPU: %i", i);
						operacionesHeap(i, header.length, LIBERAR,
								socketMemoria);
						break;

					case ABRIR_ARCHIVO:
						log_debug(logFile,
								"Se recibio una solicitud Abrir Archivo por parte del CPU: %i",
								i);
						operacionConArchivos(i, header.length, ABRIR_ARCHIVO);
						break;

					case CERRAR_ARCHIVO:
						log_debug(logFile,
								"Se recibio una solicitud Abrir Archivo por parte del CPU: %i",
								i);
						operacionConArchivos(i, header.length, CERRAR_ARCHIVO);
						break;

					case BORRAR_ARCHIVO:
						log_debug(logFile,
								"Se recibio una solicitud de Borrar Archivo por parte del CPU: %i",
								i);
						operacionConArchivos(i, header.length, BORRAR_ARCHIVO);
						break;

					case MOVER_CURSOR:
						log_debug(logFile,
								"Se recibio una solicitud de Mover Cursor Archivo por parte del CPU: %i",
								i);
						operacionConArchivos(i, header.length, MOVER_CURSOR);
						break;

					case LEER_ARCHIVO:
						log_debug(logFile,
								"Se recibio una solicitud de Borrar Archivo por parte del CPU: %i",
								i);
						operacionConArchivos(i, header.length, LEER_ARCHIVO);
						break;

					case ESCRIBIR_ARCHIVO:
						log_debug(logFile,
								"Se recibio una solicitud de print por parte del CPU: %i",
								i);
						operacionConArchivos(i, header.length,
								ESCRIBIR_ARCHIVO);
						break;

					case ERROR_HEAP:
						//Finalizacion por error en Operacion Alocar/Liberar, el pcb ya venbra con el EC=-9 desde CPU, ver que funcion generica por finalizacion con error usar
						operacionPCB(i, header, ERROR_HEAP);

//						log_error(logFile,"Finalizacion de ejecucion del proceso con PID:%i por"
//								" error Heap CPU: %i",programa->PID,i);

						break;

					case ERROR_APERTURA:
						//Finalizacion por error en Apertura Archivo, el pcb ya venbra con el desde CPU, ver que funcion generica por finalizacion con error usar

						operacionPCB(i, header, ARCHIVO_INEXISTENTE);

//						log_error(logFile,"Finalizacion de ejecucion del proceso  por"
//								" error de Apertura de archivo CPU: %i",i);

						break;

					case ERROR_ESCRIBIR:
						//Finalizacion Escritura Archivo, el pcb ya venbra con el EC=-4 desde CPU, ver que funcion generica por finalizacion con error usar
						operacionPCB(i, header, ESCRIBIR_SINPERMISOS);

//						log_error(logFile,"Finalizacion de ejecucion del proceso  por "
//								"error de Escritura de archivo CPU: %i",i);

						break;

					case ERROR_MEMORIA:
						//Finalizacion por error en Memoria, el pcb ya venbra con el EC=-5 desde CPU, ver que funcion generica por finalizacion con error usar
						operacionPCB(i, header, EXCEPCION_MEMORIA);

						break;

					case ERROR_STACKOVERFLOW:
						//Finalizacion por error StackOverFlow, el pcb ya venbra con el EC=-5 desde CPU, ver que funcion generica por finalizacion con error usar
						operacionPCB(i, header, STACKOVERFLOW);

//						log_error(logFile,"Finalizacion de ejecucion de proceso con PID:%i"
//								"por error Stackoverflow, CPU: %i",programa->PID,i);

						break;

					case FINALIZAR_SIGUSR1:
						//Se recibio una señal de la CPU
						log_debug(logFile,"Se recibe un aviso de CPU luego de concluir la ejecucion actual"
								"la CPU: %i se desconectara por señal SIGUSR1",i);

						//Se eliminara la CPU de la lista, dado que no se debe replanificar
						borrarCPU(i);
						break;

					case ALLOC_MAYOR_TAM_PAGINA:

						operacionPCB(i, header, MAYOR_MEMORIA);

						break;

					case NO_PUDO_ASIGNAR_PAGINAS:

						operacionPCB(i, header, NO_ASIGNAR_PAGINAS);

						break;

					case ERROR:
						//Finalizacion por error sin definicion, el pcb ya venbra con el EC=-20 desde CPU, ver que funcion generica por finalizacion con error usar
						operacionPCB(i, header, SIN_DEFINICION);

//						log_error(logFile,"Finalizacion de ejecucion del proceso "
//								"con PID:%i por error generico,"
//								" CPU: %i",programa->PID,i);

						break;

						}
					}
				}
			}
		}

	}

	close(socketConsolas);
	close(socketCpu);
}

int pedirPaginaAMemoria(int pid, int cant, int socketMemoria)
{
	int nbytes=0;
	header_t header;
	header.type=ASIGNAR_PAGINAS;

	t_solicitudPaginas* solicitud =malloc(sizeof(t_solicitudPaginas));
	solicitud->cantidadPaginas=cant;
	solicitud->pid=pid;

	char*solicitudPagina= solicitudPaginas_serializer(solicitud,&header.length);

	nbytes=sockets_send(socketMemoria,&header,solicitudPagina);

	if(nbytes>0)
	{
		recv(socketMemoria, &header, sizeof(header_t), MSG_WAITALL);

		if(header.type==SUCCESS){
			log_debug(logFile,"Asignacion de Pagina de Heap Correcta para el proceso con pid: %d, Paginas Cantidad: %d", pid , cant);

		}else{
			log_error(logFile,"Error en respuesta de pedido de pagina por "
					"parte de la memoria para el proceso con pid:%d",pid);
			return -1;
		}
	}else{
		log_error(logFile,"Error en envio de pedido de pagina a memoria del proceso "
				"de pid:%i",pid);
		return -2;
	}

	free(solicitudPagina);
return 0;
}

int enviarCodigoAMemoria(int socketMemoria , int pid , int paginasCodigo, char * codigo)
{
	int nbytes,offset=0;
	header_t header;
	uint32_t pagina,tamanio;
	t_solicitudEscritura * solicitudEscritura = malloc(sizeof(t_solicitudEscritura));
	//int tamCodigo = strlen(codigo) + 1;
	int tamCodigo = strlen(codigo);

	//if (paginasCodigo == 1)
	solicitudEscritura->pid = pid;
	solicitudEscritura->offset=0;
	for(pagina=0;pagina<paginasCodigo;pagina++)
	{
		header.type=ESCRIBIR_PAGINA;
		if(pagina==paginasCodigo-1){
			tamanio=tamCodigo-(tamanioPagina*(paginasCodigo-1));
			solicitudEscritura->buffer = malloc(tamanio);
			//strcpy(solicitudEscritura->buffer,codigo);
			memcpy(solicitudEscritura->buffer,codigo+offset,tamanio);
			offset+=tamanio;
			solicitudEscritura->pagina=pagina;
			solicitudEscritura->tamanio=tamanio;
		}else{
			solicitudEscritura->buffer = malloc(tamanioPagina);
			//strcpy(solicitudEscritura->buffer,codigo);
			memcpy(solicitudEscritura->buffer,codigo+offset,tamanioPagina);
			offset+=tamanioPagina;
			solicitudEscritura->pagina=pagina;
			solicitudEscritura->tamanio=tamanioPagina;
		}

		char * notificacion_solicitudEscritura= solicitudEscritura_serializer(solicitudEscritura,&header.length);
		free(solicitudEscritura->buffer);
		nbytes=sockets_send(socketMemoria,&header,notificacion_solicitudEscritura);
		if(nbytes>0)
		{
			recv(socketMemoria, &header, sizeof(header_t), MSG_WAITALL);

			if(header.type==SUCCESS){
				log_debug(logFile,"Reserva de Memoria para codigo Fuente Correcta, Pagina Nro: %d",pagina);
			}else{
				log_error(logFile,"Error en respuesta de escritura por parte de la memoria, fallo en la Pagina Nro: %d",pagina);
				return -1;
			}
		}else{
			log_error(logFile,"Error en envio de escritura a memoria, fallo en la Pagina Nro: %d",pagina-1);
			return -2;
		}
	}
	free(solicitudEscritura);
	return 0;
}

void recibirTamanio(int socket, uint32_t tamanio, uint32_t * elemento)
{
	char *respuesta = malloc(tamanio);

	recv(socket, respuesta, tamanio, MSG_WAITALL);

	uint32_t *notificacion_tamanio = uint32_deserializer(respuesta);

	*elemento = *notificacion_tamanio;

	free(notificacion_tamanio);
	free(respuesta);
}

int recibirMensajeMemoria(int socket)
{
	header_t header;

	recv(socket, &header, sizeof(header_t), MSG_WAITALL);

	switch (header.type) {
	//Mensajes recibidos por socketKernel
	case TAMANIO_PAGINA:
		recibirTamanio(socket, header.length, &tamanioPagina);
		log_info(logFile, "Inicilizando CPU, tamPagina:%d", tamanioPagina);
		//printf("tamPagina: %d\n", tamanioPagina);
		break;
	default:
		log_error(logFile, "Protocolo Inesperado");
		return 1;
	}
	return 0;
}


char *recibirProgramaAnsisop(int socketConsola, uint32_t length)
{
	header_t header;

	char *datos;
	//datos_programa_t *notificacion;

	datos = malloc(length);
	recv(socketConsola, datos, length, MSG_WAITALL);

	//notificacion = notificacionDatosPrograma_deserializer(datos);
	//free(datos);

	log_debug(logFile, "Script recibido y des-seriealizado: %s.",
			datos);

	return datos;
}


int enviarPid(int socket, int pid, int tipo)
{
	header_t header;
	header.type = tipo;
	//TODO Averiguar si es necesario hacer un malloc de puntero uint32_t, yo por las dudas lo hago.
	uint32_t *notificacion_pid = malloc(sizeof(uint32_t));

	*notificacion_pid = pid;

	char *serialized = uint32_serializer(notificacion_pid, &header.length);

	free(notificacion_pid);
	//sprintf(str, "%d", pid);
	return sockets_send(socket, &header, serialized);
}



int aceptarConexionConsola(int sockfdProgramas)
{
	int nbytes;

	//printf("Socket a escuchar: type=%d \n.", sockfdProgramas);

	log_info(logFile,"Socket a escuchar: type=%d \n.", sockfdProgramas);

	if (!aceptarConexion(sockfdProgramas, HANDSHAKE_CONSOLA)) {
		log_info(logFile, "Conexion con Consola establecida, Socket: %d.\n",
				sockfdProgramas);
		nbytes = enviarHandshake(sockfdProgramas, HANDSHAKE_KERNEL);
	} else {
		log_warning(logFile,
				"Mensaje inesperado de nueva conexion, en Socket Consola\n");
		close(sockfdProgramas);
		return -1;
	}
	return nbytes;
}

t_PCB * recibirPCB(int socketCPU)
{
	header_t header;

	t_PCB * pcb;

	char * mensaje;

	int nbytes = recv(socketCPU, &header, sizeof(header_t), MSG_WAITALL);

	if(nbytes > 0 )
	{
		//Se libera CPU para reutilizacion
		resetarCPU(socketCPU);
		log_debug(logFile,
				"Se resetea el estado de la CPU, para reutilizacion\n");

		mensaje = malloc(header.length);

		recv(socketCPU, mensaje, header.length, MSG_WAITALL);

		pcb = deserializer_pcb(mensaje);
		free(mensaje);

		log_debug(logFile, "Se recibio correctamente PCB con pid: %d.", pcb->PID);

		return pcb;
	}else{

		log_error(logFile, "Error al recibir PCB de CPU");
		return NULL;
	}
}

int enviarFinalizarProgramaAMemoria(int socket, uint32_t pid)
{
	header_t header;
	header.type =FINALIZAR_PROGRAMA;

	uint32_t *notificacion_pid = malloc(sizeof(uint32_t));

	*notificacion_pid = pid;

	char *serialized = uint32_serializer(notificacion_pid, &header.length);

	uint32_t nbytes = sockets_send(socket, &header, serialized);

	free(notificacion_pid);
	free(serialized);

	return nbytes;
}

