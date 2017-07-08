#include "Kernel.h"
#include "configInicialKernel.h"
#include "conexiones.h"
#include "planificador.h"
#include "capaFileSystem.h"
#include "capaMemoria.h"
#include "consolaKernel.h"

#include <Sockets/Serializer.h>
#include <Sockets/StructsUtils.h>

int Tmax;

bool planificacion = true;

char*ruta;

// ********************MAIN***********************
int main(int argc, char** argv)
{

	//int socketMemoria;
	verificarParametros(argc);
	//char*ruta;
	ruta = leerParametros(argv);

	archC = leerArchivoConfiguracion(ruta);

	remove(LOG_PATH);
	logFile = log_create(LOG_PATH, "Proceso Kernel", false, archC->detallelog);

	iniciarInotify();

	//Se establece conexion con memoria

	socketMemoria=AbrirConexion(archC->IP_MEMORIA,archC->PUERTO_MEMORIA);
	if(socketMemoria==-1)
		{
			log_error(logFile,"Error al crear socket");
			//printf("Ha ocurrido un error intentando conectarse a la Memoria\n");
			return EXIT_FAILURE;
		}

	log_info(logFile, "Se envia handshake Kernel");
	enviarHandshake(socketMemoria,HANDSHAKE_KERNEL);

	if(aceptarConexion(socketMemoria,HANDSHAKE_MEMORIA)==0){
		if(recibirMensajeMemoria(socketMemoria))
		{
			log_error(logFile,"No se pudo obtener tamaño de Pagina de Memoria");
			return EXIT_FAILURE;
		}else{
			Tmax=tamanioPagina-2*(sizeof(uint32_t)+sizeof(bool));
		}
		//printf("Conexion con la Memoria establecida, Socket: %d\n", socketMemoria);
		log_info(logFile,"Conexion con la Memoria establecida, Socket: %d.", socketMemoria);
	}else{
		//printf("Protocolo Inesperado en conexion con Memoria");
		log_error(logFile,"Protocolo Inesperado");
		return EXIT_FAILURE;
	}


		//Se establece conexion con FileSystem

	tablaGFT = list_create();
	tablaPFT = list_create();

	auxiliarArchivo = malloc(sizeof(t_datosGlobalesArchivos));

	socketFS = AbrirConexion(archC->IP_FS, archC->PUERTO_FS);
	if (socketFS == -1) {
		log_error(logFile, "Error al crear socket\n");
		//printf("Ha ocurrido un error intentando conectarse al FileSystem\n");
		return EXIT_FAILURE;
	}

	log_info(logFile, "Se envia handshake Kernel\n");
	enviarHandshake(socketFS, HANDSHAKE_KERNEL);
	if (aceptarConexion(socketFS, HANDSHAKE_FILESYSTEM) == 0) {
		//printf("Conexion con Filesytem establecida, Socket: %d\n", socketFS);
		log_info(logFile,
				"Conexion con el FileSystem establecida, Socket: %d.\n",
				socketFS);
	} else {
		//printf("Protocolo Inesperado en conexion con FileSystem\n");
		log_error(logFile, "Protocolo Inesperado\n");
		return EXIT_FAILURE;
	}
	sem_init(&mutex_GFT,0,1);
	sem_init(&mutex_PFT,0,1);
/* pruebas comentar
				t_operacionDescriptorArchivo * operacionDescriptorArchivo=malloc(sizeof(t_operacionDescriptorArchivo));
				operacionDescriptorArchivo->informacion=strdup("/Alumnos/Diaz");
				operacionDescriptorArchivo->pid=3200;
				operacionDescriptorArchivo->flags.creacion=true;
				operacionDescriptorArchivo->flags.lectura=true;
				operacionDescriptorArchivo->flags.escritura=true;
				int fd= abrirArchivo(operacionDescriptorArchivo->pid,operacionDescriptorArchivo->informacion,operacionDescriptorArchivo->flags);


	//prueba de cerrar archivo
	operacionDescriptorArchivo->descriptor = fd;
	auxiliarArchivo->pidActual = operacionDescriptorArchivo->pid;
	auxiliarArchivo->fdActual = operacionDescriptorArchivo->descriptor;
	// cerrarArchivo(operacionDescriptorArchivo->pid,operacionDescriptorArchivo->descriptor);

	//prueba de borrar archivo
	// borrarArchivo(operacionDescriptorArchivo->pid,operacionDescriptorArchivo->descriptor);

	//prueba de escribir datos
	operacionDescriptorArchivo->informacion =
			strdup(
					"prueba de un texto mas largo para ver si abarca otro .bin aumentando el tamaño maximo de cada bloque");
	operacionDescriptorArchivo->tamanio = strlen(
			operacionDescriptorArchivo->informacion);
	escribirArchivo(operacionDescriptorArchivo->descriptor,
			operacionDescriptorArchivo->pid,
			operacionDescriptorArchivo->informacion,
			operacionDescriptorArchivo->tamanio);
	//prueba de mover cursor
	moverCursor(operacionDescriptorArchivo->descriptor,
			operacionDescriptorArchivo->pid,
			operacionDescriptorArchivo->tamanio);
	escribirArchivo(operacionDescriptorArchivo->descriptor,
			operacionDescriptorArchivo->pid,
			operacionDescriptorArchivo->informacion,
			operacionDescriptorArchivo->tamanio);
	//prueba de leer archivo
	operacionDescriptorArchivo->tamanio = 0;
	moverCursor(operacionDescriptorArchivo->descriptor,
			operacionDescriptorArchivo->pid,
			operacionDescriptorArchivo->tamanio);
	operacionDescriptorArchivo->tamanio = 6;
	operacionDescriptorArchivo->informacion = malloc(6);
	leerArchivo(operacionDescriptorArchivo->pid,
			operacionDescriptorArchivo->descriptor,
			operacionDescriptorArchivo->tamanio,
			&operacionDescriptorArchivo->informacion);

*/
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//INICIALIZACION DE ESTRUCTURAS
	listaCPU = list_create();
	listaConsola = list_create();
	listaPCB = list_create();
	listaProgramas = list_create();
	tablaHeap = list_create();
	tablaSyscalls= list_create();

	//un comando es mostrar todos los procesos que esten en algun estado específico(aclara que son colas)
	colaNew = queue_create();
	colaReady=list_create();
	colaExit = queue_create();
	colaExec = list_create();
	colaBlock = queue_create();

	aSerBloqueado = queue_create();
	aSerReplanificado = queue_create();
	aSerEliminado = list_create();

	cant_programas = 0;

	cantProgRunning=0;
	cantProgsEnSistema=0;

	//para poder modificarlo por consola del Kernel
	gradoMultiprog = archC->GRADO_MULTIPROG;

	sem_init(&progReady, 0, 1);
	sem_init(&progRunning, 0, 1);
	//el grado de multiprocesamiento depende de la cantidad de CPU para ejecutar
	sem_init(&mutexCPU, 0, 1);
	sem_init(&mutexConsola, 0, 1);
	sem_init(&mutexLista, 0, 1);
	sem_init(&mutexPid,0,1);
	sem_init(&mutex_lista_pcb,0,1);
	sem_init(&CPULibres, 0, 0);
	sem_init(&mutexNew, 0, 1);
	sem_init(&cantNew, 0, 0);
	pthread_mutex_init(&semPlanificacion, NULL);

	crearHiloPlanificacion(socketMemoria);

	crearHiloConsolaKernel(); //ver ubicacion de esta

	sockets_select(archC, socketMemoria);

	pthread_join(hiloPlanificacion,NULL);

	pthread_join(hiloConsolaKernel, NULL);

	free(archC);

	list_destroy(tablaHeap);
	list_destroy(tablaSyscalls);
	list_destroy(listaCPU);
	list_destroy(listaConsola);
	list_destroy(listaPCB);
	list_destroy(listaProgramas);

	queue_destroy(colaNew);
	list_destroy(colaReady);
	queue_destroy(colaExit);
	list_destroy(colaExec);
	queue_destroy(colaBlock);

	list_destroy(aSerEliminado);
	queue_destroy(aSerBloqueado);
	queue_destroy(aSerReplanificado);

	sem_destroy(&mutexCPU);
	sem_destroy(&mutexConsola);
	sem_destroy(&mutexLista);
	sem_destroy(&mutexPid);
	sem_destroy(&mutex_lista_pcb);
	sem_destroy(&CPULibres);
	sem_destroy(&progReady);
	sem_destroy(&progRunning);

	pthread_mutex_destroy(&semPlanificacion);

	log_destroy(logFile);

	return 0;
}


t_PCB * crearPCB(char * codigo,int socket)
{
	t_PCB* pcb=malloc(sizeof(t_PCB));
	pcb->tablaHeap=malloc(sizeof(t_tabla_heap));
	t_datos_programa* programa = malloc(sizeof(t_datos_programa));;
	int bytesEnviados;
	programa->Script = malloc(strlen(codigo) + 1);

	log_debug(logFile, "Se crea un PCB para el Programa Solicitado\n");

	//creo estructura auxiliar para guardar el código hasta pasar a ready
	programa->Script = codigo;
	programa->PID = actualizarPID();
	//programa->socketConsola = socket;
	bytesEnviados = enviarPid(socket, programa->PID, ENVIO_PID);
//	printf("se enviaron %i bytes\n", bytesEnviados);

	list_add(listaProgramas, programa);

	//inicalizo pcb
	pcb->socket=socket;
	pcb->PID=programa->PID;
	pcb->estado = NEW;
	pcb->abortarEjecucion=false;

	sem_wait(&mutexNew);
	queue_push(colaNew,pcb);
	sem_post(&mutexNew);

	log_info(logFile,"El programa con pid: %i ha ingresado a la cola New,"
			"queda a la espera de la planificación para ingresar al sistema.\n",pcb->PID);

	return pcb;
}

t_list * cargarLista(t_intructions*indiceCodigo, t_size cantInstrucciones)
{
	t_list * lista = list_create();
	int i;
	for(i=0; i<cantInstrucciones; i++)
	{
		t_indice_codigo* linea = malloc(sizeof(t_indice_codigo));
		linea->inicio=indiceCodigo[i].start;
		linea->longitud = indiceCodigo[i].offset;
		list_add(lista,linea);
	}
	return lista;
}


  int inicializarPrograma (int socketMemoria , t_PCB * pcb, int stack_size)
{

	t_metadata_program* datos;
	header_t header;
	header.type=INICIAR_PROGRAMA;

	t_aSerEliminado* ePCB = malloc(sizeof(t_aSerEliminado));
	t_datos_programa* programa;

	bool _tieneElPid(t_datos_programa* programa) {

			return (programa->PID) == pcb->PID;

		}
		programa = list_remove_by_condition(listaProgramas,
				(void*) _tieneElPid);


	t_solicitudPaginas * iniciarPrograma = malloc(sizeof(t_solicitudPaginas));
	iniciarPrograma->pid=pcb->PID;

	//Obtengo Cantidad de paginas
	if (strlen(programa->Script) % tamanioPagina != 0)
		iniciarPrograma->cantidadPaginas = ((strlen(programa->Script)) / tamanioPagina) + 1 + stack_size;
	else
		iniciarPrograma->cantidadPaginas = ((strlen(programa->Script)) / tamanioPagina) + stack_size;

	char *notificacion_solicitudPaginas = solicitudPaginas_serializer(iniciarPrograma,&header.length);

	int nbytes=sockets_send(socketMemoria,&header,notificacion_solicitudPaginas);

	if(nbytes>0)
	{
		recv(socketMemoria, &header, sizeof(header_t), MSG_WAITALL);

		if(header.type==SUCCESS){
			log_debug(logFile,"Reserva de Memoria Correcta: Pid= %d - Cantidad Paginas= %d\n",iniciarPrograma->pid,iniciarPrograma->cantidadPaginas);

			//Aca se envian las paginas de Codigo a Memoria
			int paginasCodigo = iniciarPrograma->cantidadPaginas - stack_size;
			if(enviarCodigoAMemoria(socketMemoria,pcb->PID,paginasCodigo,programa->Script)==0)
			{

				//Obtengo la metadata utilizando el preprocesador del parser
				datos=metadata_desde_literal(programa->Script);

				pcb->quantum=archC->QUANTUM;
				pcb->quantum_sleep=archC->QUANTUM_SLEEP;

				pcb->paginaCodigoActual = 0;
				pcb->paginaStackActual = iniciarPrograma->cantidadPaginas - stack_size;
				pcb->primerPaginaStack = pcb->paginaStackActual;
				pcb->stackPointer = 0;
				pcb->PC=datos->instruccion_inicio;
				pcb->EC=0;
				pcb->rafagas=0;

				t_list * pcbStack = list_create();
				pcb->indiceStack=pcbStack;
				pcb->indiceContextoEjecucionActualStack=0;
				pcb->tamanioEtiquetas=datos->etiquetas_size;

				//Cargo Indice de Codigo
				t_list * listaIndiceCodigo = cargarLista(datos->instrucciones_serializado,datos->instrucciones_size);
				pcb->IC=listaIndiceCodigo;

				if(datos->cantidad_de_etiquetas > 0 || datos->cantidad_de_funciones > 0)
				{
					char* indiceEtiquetas = malloc(datos->etiquetas_size);
					indiceEtiquetas = datos->etiquetas;
					pcb->indice_etiquetas=indiceEtiquetas;
				}else{
					pcb->indice_etiquetas = NULL;
				}

				pcb->tablaHeap=malloc(sizeof(t_tabla_heap));
				pcb->tablaHeap->pagina=list_create();

				t_cantPagsHeap * pagsHeap=malloc(sizeof(t_cantPagsHeap));
				pagsHeap->cantPagsHeap=0;
				pagsHeap->pid=pcb->PID;
				pagsHeap->alocarBytes=0;
				pagsHeap->alocarCant=0;
				pagsHeap->liberarBytes=0;
				pagsHeap->liberarCant=0;

				list_add(tablaHeap, pagsHeap);

				t_cantSyscalls* syscalls= malloc(sizeof(t_cantSyscalls));
				syscalls->cantAbrir=0;
				syscalls->cantCerrar=0;
				syscalls->cantEscribir=0;
				syscalls->cantLeer=0;
				syscalls->pid=pcb->PID;

				list_add(tablaSyscalls, syscalls);
			}else{

				ePCB->exitcode = SIN_DEFINICION;
				ePCB->pcb = pcb;
				ePCB->aviso=GENERICO;

				list_add(aSerEliminado, ePCB);

				log_error(logFile,"Error en envio de Mensaje Escribir_pagina\n");
				free(iniciarPrograma);
				return -3;

			}

		}else{

			ePCB->exitcode = NO_RESERVO_RECURSOS;
			ePCB->pcb = pcb;
			ePCB->aviso=GENERICO;

			list_add(aSerEliminado, ePCB);


			log_error(logFile,
			"no se ha podido ingresar el programa del socket:%i al sistema por falta de espacio"
			" en memoria\n", pcb->socket);
			free(iniciarPrograma);
			return -2;
		}

	}else{

		ePCB->exitcode = SIN_DEFINICION;
		ePCB->pcb = pcb;
		ePCB->aviso=GENERICO;

		list_add(aSerEliminado, ePCB);

		log_error(logFile,"Error en envio de Mensaje Iniciar_Programa\n");
		free(iniciarPrograma);
		return -1;

	}

	free(iniciarPrograma);
	free(datos);
	free(programa->Script);
	return 0;
}



int actualizarPID()
{
	sem_wait(&mutexPid);
	cant_programas++;
	sem_post(&mutexPid);
	return cant_programas;
}


//MANEJO  DE SEMAFOROS
void crearSemaforos(char **semIds, char **semInit)
{
	listaSemaforos = list_create();
	int i;
	for (i = 0; semIds[i] != NULL; i++) {
		t_semaforo *semaforo = malloc(sizeof(t_semaforo));
		semaforo->nombre = semIds[i];
		semaforo->valor = atoi(semInit[i]);
		list_add(listaSemaforos, semaforo);
	}
}

//VARIABLES COMPARTIDAS
void crearVariablesCompartidas(char **sharedVars)
{
	listaVariablesCompartidas = list_create();
	int i;
	for (i = 0; sharedVars[i] != NULL; i++) {
		t_op_varCompartida *variable = malloc(sizeof(t_op_varCompartida));
		variable->nombre = sharedVars[i];
		variable->valor = 0;
		list_add(listaVariablesCompartidas, variable);
	}
}

//OBTENER VARIABLE COMPARTIDA
t_op_varCompartida* obtenerVariable(t_op_varCompartida * varCompartida)
{
	int i;
	for(i = 0;i < list_size(listaVariablesCompartidas);i++){
		t_op_varCompartida * auxVar = list_get(listaVariablesCompartidas,i);
		if(strcmp(auxVar->nombre,varCompartida->nombre)==0)
			return auxVar;
	}
	return NULL;
}

void operacionConArchivos(int socketCPU, int tamanio, int operacion) {
	header_t header;
	char * mensaje = malloc(tamanio);
	t_operacionDescriptorArchivo * operacionDescriptorArchivo;
	uint32_t *valorNumerico;
	t_PCB *pcb;
	t_datos_programa *arch;

	int nbytes = recv(socketCPU, mensaje, tamanio, 0);

	if (nbytes > 0) {
		operacionDescriptorArchivo = operacionDescriptorArchivo_deserializer(
				mensaje);
		free(mensaje);
		uint32_t fd;

		switch(operacion)
		{
			case ABRIR_ARCHIVO:
				auxiliarArchivo->pidActual=operacionDescriptorArchivo->pid;
				log_debug(logFile, "Se recibe solicitud de apertura Pid= %d, Path= %s",operacionDescriptorArchivo->pid,operacionDescriptorArchivo->informacion);
				fd=abrirArchivo(operacionDescriptorArchivo->pid,operacionDescriptorArchivo->informacion,operacionDescriptorArchivo->flags);
				//ACA el Kernel recibira por parte de la CPU lo siguiente
				//operacionDescriptorArchivo->pid - operacionDescriptorArchivo->tamanio=0 (No importa) - operacionDescriptorArchivo->informacion=direccion (Aca viene el path)
				//operacionDescriptorArchivo->flags.creacion - operacionDescriptorArchivo->flags.escritura - operacionDescriptorArchivo->flags.lectura
				//realizar lo que correponda por el kernel y devolver un uint32_t con el fileDescriptor asignado a CPU
				if(fd>2)//se empieza desde 3 la asignacion de fd
				{
					//Para que el CPU no rompa me devuelvo un valor numerico HardCode luego sera el FileDescriptor Asignado
					//*valorNumerico=30;
					mensaje=uint32_serializer(&fd,&header.length);
					log_debug(logFile, "Se abre correctamente el archivo solicitado, FileDescriptor Asignado = %d con CPU, socket: %d",*valorNumerico,socketCPU);
					header.type=SUCCESS;
					sockets_send(socketCPU,&header,mensaje);
					free(mensaje);
				}else{
					header.type=ERROR_APERTURA;
					header.length=0;
					sockets_send(socketCPU,&header,"\0");
					log_error(logFile, "error al alocar espacio en Heap con CPU, socket: %d",socketCPU);
				}
				break;
			case CERRAR_ARCHIVO:

				//ACA el Kernel recibira por parte de la CPU lo siguiente (Igual que Borrar Archivos)
				//borrarArchivo->descriptor=descriptor_archivo; borrarArchivo->pid=pcb->PID; El resto de los parametros no importan
				auxiliarArchivo->pidActual=operacionDescriptorArchivo->pid;
				auxiliarArchivo->fdActual=operacionDescriptorArchivo->descriptor;
				if(cerrarArchivo(operacionDescriptorArchivo->pid,operacionDescriptorArchivo->descriptor))
				{
					header.length=0;
					header.type=SUCCESS;
					sockets_send(socketCPU,&header,"\0");
				}else{
					header.length=0;
					header.type=ERROR; //Por ahora usamos el Error generico
					sockets_send(socketCPU,&header,"\0");
				}
				break;

		case BORRAR_ARCHIVO:
			//ACA el Kernel recibira por parte de la CPU lo siguiente
			//	borrarArchivo->descriptor=descriptor_archivo; borrarArchivo->pid=pcb->PID; El resto de los parametros no importan

			if (borrarArchivo(operacionDescriptorArchivo->pid,operacionDescriptorArchivo->descriptor)) {
				header.length = 0;
				header.type = SUCCESS;
				sockets_send(socketCPU, &header, "\0");
				log_info(logFile, "el archivo se borrado exitosamente\n");
			} else {
				header.length = 0;
				header.type = ERROR; //Por ahora usamos el Error generico
				sockets_send(socketCPU, &header, "\0");
				log_error(logFile, "no se pudo borrar el archivo \n");
			}
			break;

		case MOVER_CURSOR:
				//ACA el Kernel recibira por parte de la CPU lo siguiente
				//pcb->PID; descriptor_archivo; tamanio=Posicion > OJO EN EL TAMANIO SE RECIBE LA POSICION, ESTO ES PARA REUTILIZAR LA ESTRUCTURA
				//Me envio el Ok HardCODE
				if(moverCursor(operacionDescriptorArchivo->descriptor,operacionDescriptorArchivo->pid,operacionDescriptorArchivo->tamanio))
				{
					header.length=0;
					header.type=SUCCESS;
					sockets_send(socketCPU,&header,"\0");
				}else{
					header.length=0;
					header.type=ERROR; //Por ahora usamos el Error generico
					sockets_send(socketCPU,&header,"\0");
				}
				break;

			case LEER_ARCHIVO:
				log_debug(logFile,"Se recibio un pedido de Lectura de CPU,Pid= %d ,Descriptor_archivo= %d, Puntero= %s",operacionDescriptorArchivo->pid, operacionDescriptorArchivo->descriptor,operacionDescriptorArchivo->informacion);
				//Desarrollar la tarea correspondiente en base al pedido de Lectura del Kernel
				//Parametros recibidos, Pid, Descriptor, en Informacion vendra el Puntero Heap y tamanio

				//TODO por ahora se guarda la informacion en la info de operacionDescriptorArchivo, cambiar a la posicion del heap
				int puntero=atoi(operacionDescriptorArchivo->informacion);
				if(leerArchivo(operacionDescriptorArchivo->pid,operacionDescriptorArchivo->descriptor,operacionDescriptorArchivo->tamanio,operacionDescriptorArchivo->informacion))
				{
				almacenar(operacionDescriptorArchivo->informacion,puntero,operacionDescriptorArchivo->pid);
				header.length = 0;
				header.type = SUCCESS;
				sockets_send(socketCPU, &header, "\0");
				log_info(logFile, "se ha leído el archivo correctamente\n");
			} else {
				header.length = 0;
				header.type = ERROR_LECTURA;
				sockets_send(socketCPU, &header, "\0");
				log_error(logFile, "no se pudo leer el archivo\n");
			}

			break;

		case ESCRIBIR_ARCHIVO:
			log_debug(logFile,
					"Se recibio un pedido de Escritura de CPU,Pid= %d ,Descriptor_archivo= %d, contenido= %s",
					operacionDescriptorArchivo->pid,
					operacionDescriptorArchivo->descriptor,
					operacionDescriptorArchivo->informacion);

			if (operacionDescriptorArchivo->descriptor == DESCRIPTOR_SALIDA) {
				//enviar a Imprimir por consola, se debe buscar la consola correspondiente al pid y mandar a imprimir
				header.length = 0;
				header.type = IMPRIMIR_ARCHIVO;
				bool _tieneElPid(t_PCB* programa) {

					return (programa->PID) == operacionDescriptorArchivo->pid;

				}
				pcb = list_find(listaPCB, (void*) _tieneElPid);

				arch = malloc (sizeof(t_datos_programa));
				arch->PID = operacionDescriptorArchivo->pid;
				arch->Script = malloc(operacionDescriptorArchivo->tamanio);
				arch->Script = operacionDescriptorArchivo->informacion;

				mensaje = datosPrograma_serializer(arch,
						&header.length);
				int nbytes = sockets_send(pcb->socket, &header, mensaje);
				log_info(logFile, "se envio para imprimir en consola en Socket numero: %d\n",pcb->socket);
				free(mensaje);

				if (nbytes > 0) {
					//Se envia confirmacion a CPU ya que la misma lo espera
					header.length = 0;
					header.type = SUCCESS;
					sockets_send(socketCPU, &header, "\0");
					log_info(logFile, "se envio el SUCCESS CORRECTAMENTE\n");
				} else {
					header.length = 0;
					header.type = ERROR;
					sockets_send(socketCPU, &header, "\0");
					log_info(logFile, "Error en Impresion Por Consola, Posible "
							"perdida de conexion con consola o Fin de ejecucion de programa");
				}

				}else{

					//Se debe enviar a escribir el archivo
					//En caso fallido se debe enviar el error a la CPU para que la misma devuelva el pcb con el proceso
					//Aca hay que realizar la operacion de de Escritura en Archivo, devolver el Ok o Error segun corresponda
					if(escribirArchivo(operacionDescriptorArchivo->descriptor,operacionDescriptorArchivo->pid,operacionDescriptorArchivo->informacion,operacionDescriptorArchivo->tamanio))
					{
						 header.length=0;
						 header.type=SUCCESS;
						 sockets_send(socketCPU,&header,"\0");
						 log_info(logFile, "se ha podido escribir el archivo correctamente\n");
					}else{
						header.length=0;
						header.type=ERROR_ESCRIBIR;
						sockets_send(socketCPU,&header,"\0");
						log_error(logFile,"se produjo un error al escribir el archivo\n");
					}
				}
			break;
		}

	} else {
		log_error(logFile, "error en recibir datos con CPU, socket: %d\n",
				socketCPU);
	}
}
/*
int buscarConsolaEnColaExec(int socketCPU)
{
	//Adapatar esta funcion segun la lista de pid y consolas que se arme
	int i;/*
	for (i = 0; i < list_size(colaExec); i++) {
		t_socket_pid * socketPid = list_get(listaSocketsConsola, i);
		if (pid == socketPid->pid) {
			return socketPid;
		}
	}*/
//	return 0;

//}

void operacionConVariableCompartida(int socketCPU, int tamanio, int operacion)
{

	header_t header;
	int i;
	char * mensaje = malloc(tamanio);
	char * serialized;
	t_op_varCompartida *varCompartida;
	t_op_varCompartida *varCompartidaEnLista;

	int nbytes=recv(socketCPU, mensaje, tamanio, 0);


	if(nbytes>0)
	{
		varCompartida = deserializer_opVarCompartida(mensaje);
		free(mensaje);
		varCompartidaEnLista = obtenerVariable(varCompartida);
		switch(operacion){

			case LEER_VAR_COMPARTIDA:
				if(varCompartidaEnLista != NULL)
				{
					log_debug(logFile, "Se lee variable compartida: %s\n",varCompartidaEnLista->nombre);
					serialized = serializer_opVarCompartida(varCompartidaEnLista,&header.length);
					sockets_send(socketCPU,&header,serialized);
					free(serialized);
				}else{
					log_error(logFile, "Error Variable Compartida inexistentes solicitada por CPU,"
							" socket: %d , Nombre: %s\n",socketCPU,varCompartida->nombre);
					header.type=ERROR;
					header.length=0;
					sockets_send(socketCPU,&header,"\0");
				}
				free(varCompartida);
				break;
			case ASIGNAR_VAR_COMPARTIDA:
				if(varCompartidaEnLista != NULL)
				{
					log_debug(logFile, "Se recibe una solicitud de asignacion VariableCompartida: %s\n",
							varCompartidaEnLista->nombre);
					varCompartidaEnLista->valor=varCompartida->valor;
					header.type=SUCCESS;
					header.length=0;
					sockets_send(socketCPU,&header,"\0");
				}else{
					log_error(logFile, "Error Variable Compartida inexistentes solicitada por CPU,"
							" socket: %d , Nombre: %s\n",socketCPU,varCompartida->nombre);
					header.type=ERROR;
					header.length=0;
					sockets_send(socketCPU,&header,"\0");
				}
				free(varCompartida);
				break;

		}
	}else{
		log_error(logFile, "error en recibir datos de Variable Compartida con CPU, socket: %d\n",socketCPU);
	}
}

void operacionesConSemaforos(int socketCPU, int tamanio, int operacion)
{
	header_t header;
	t_semaforo * semaforo;
	t_PCB * pcb;
	char * mensaje = malloc(tamanio);

	int nbytes=recv(socketCPU, mensaje, tamanio, 0);

	if(nbytes>0)
	{
		_Bool _obtenerSemaforo(t_semaforo * unSemaforo){
			return (strcmp(unSemaforo->nombre, mensaje)==0);
		}

		semaforo = list_find(listaSemaforos,(void*) _obtenerSemaforo);


		switch(operacion)
		{
			case WAIT:
				if(semaforo!=NULL){
					if(semaforo->valor > 0){
						//No se bloquea
						log_debug(logFile, "CPU: %d no se bloquea PCB por semaforo: %s\n",
								socketCPU,semaforo->nombre);
						header.type=NOTHING;
						header.length=0;
						sockets_send(socketCPU,&header,"\0");
					}else{
						//Se bloquea
						log_debug(logFile, "CPU: %d se bloquea PCB por semaforo: %s\n",socketCPU,
								semaforo->nombre);
						header.type=WAIT;
						header.length=0;
						sockets_send(socketCPU,&header,"\0");

						pcb=recibirPCB(socketCPU);

						if (pcb!=NULL)
						{
							log_debug(logFile,"Se recibio PCB con pid: %d para agregar a cola de "
									"bloqueados\n",pcb->PID);

							t_pcbBloqueado * pcbBloqueado=malloc(sizeof(t_pcbBloqueado));
							pcbBloqueado->nombreSemaforo=semaforo->nombre;
							pcbBloqueado->pcb = pcb;
							//pcbBloqueado->bloqueado=true;
							queue_push(aSerBloqueado, pcbBloqueado);
						}
						else{
							log_error(logFile, "Error en recibir PCB de la CPU:%i\n", socketCPU);

							log_error(logFile, "Un programa debería haber salido de la cola de "
									"ejecucion para ingresar a la cola de bloqueados pero ocurrio"
									"un error en la recepcion del PCB por la CPU con socket:%i.\n",socketCPU);

						}
						//Continuar Planificacion
					}
					semaforo->valor--;
				}else{
					log_error(logFile, "Error operacion WAIT con Semaforo inexistentes solicitado"
							" por CPU, socket: %d , Nombre: %s\n",socketCPU,mensaje);
					header.type=ERROR;
					header.length=0;
					sockets_send(socketCPU,&header,"\0");
				}
				free(mensaje);
				break;

			case SIGNAL:
				if(semaforo!=NULL)
				{
					log_debug(logFile, "Se recibe Signal de CPU: %d de semaforo %s\n",
							socketCPU,semaforo->nombre);

					semaforo->valor++;

					//Verificar si podria despertar el proceso
					if(semaforo->valor > 0)
					{
						int i;
						bool encontrado=false;
						t_queue *aux=queue_create();
						sem_wait(&mutexLista);
						for(i=0;i<queue_size(colaBlock);i++)
						{
							t_pcbBloqueado* pcbBloqueado = queue_pop(colaBlock);
							if(strcmp(pcbBloqueado->nombreSemaforo,semaforo->nombre)==0 && encontrado==false)
							{
								//Bandera para entrar una unica vez
								encontrado=true;
								bool _tieneElPid(t_PCB* programa)
								{
									return (programa->PID) == pcbBloqueado->pcb->PID;
								}

								sem_wait(&mutex_lista_pcb);
								t_PCB* lPCB = list_find(listaPCB,(void*) _tieneElPid);
								sem_post(&mutex_lista_pcb);

								pcbBloqueado->pcb->estado = READY;
								pcbBloqueado->pcb->punteroColaPlanif = colaReady;
								if (lPCB->tablaHeap!=NULL)
								pcbBloqueado->pcb->tablaHeap = lPCB->tablaHeap;

								list_add(colaReady, pcbBloqueado->pcb);

								log_info(logFile, "El programa con PCB:%i ha salido de la cola de"
										"bloqueados para ingresar a la cola de listos\n",pcbBloqueado->pcb->PID);

								log_info(logFile, "El programa con PID:%i ha reingresado a la lista"
										"de PCB con sus atributos actualizados\n", pcbBloqueado-> pcb->PID);

							}else
								queue_push(aux,pcbBloqueado);
						}
						colaBlock=aux;
						sem_post(&mutexLista);
						//signalBloqueados++;
						//Buscar pcb en cola de bloqueados
						//Desbloquear Si corresponde
						//Continuar la magia de la Planificacion
					}

					log_debug(logFile, "Se envio SUCCES operacion SIGNAL con Semaforo solicitado por CPU, socket: %d , Nombre: %s\n",socketCPU,mensaje);

					header.type=SUCCESS;
					header.length=0;
					sockets_send(socketCPU,&header,"\0");
				}else{

					log_error(logFile, "Error operacion SIGNAL con Semaforo inexistentes solicitado"
							" por CPU, socket: %d , Nombre: %s\n",socketCPU,mensaje);

					header.type=ERROR;
					header.length=0;
					sockets_send(socketCPU,&header,"\0");
				}

				free(mensaje);

				break;
		}

	}else{
		log_error(logFile, "error en recibir mensajes con Semaforos en CPU, socket: %d\n",socketCPU);
		//Ver que hacer y si vale la pena resolver este erro o no... No creo pase nunca.
	}
}


int iniciarInotify(){

	if (watch_descriptor > 0 && file_descriptor > 0) {
		inotify_rm_watch(file_descriptor, watch_descriptor);
	}

	file_descriptor = inotify_init();

	if (file_descriptor < 0) {
		log_error(logFile,
				"Error al iniciar inotify");
		//perror("inotify_init");
	}

	watch_descriptor=inotify_add_watch(file_descriptor,ruta, IN_MODIFY);

	return file_descriptor>0;

}

void verificarModificacion() {

	struct inotify_event *event = (struct inotify_event *) &buf[offset];

	t_config * config = config_create(ruta);

	//if (event->len) {
		if (event->mask & IN_MODIFY) {
			if (config && config_has_property(config, "QUANTUM_SLEEP")) {
				quantumSleep = config_get_int_value(config, "QUANTUM_SLEEP");
				archC->QUANTUM_SLEEP=quantumSleep;
				log_info(logFile,
						"Se modifico el archivo de Configuracion de Kernel Quantum Sleep: %d \n",
						quantumSleep);


			}
		}
		offset += EVENT_SIZE + event->len;
//	}

	free(config);
}



