/*
 * Consola.c
 *
 *  Created on: 30/3/2017
 *      Author: utnso
 */

#include "Consola.h"
#include "configuracionInicial.h"

//int cantProgramas = 0;
//int finalizarProg = 0;

paquete_t *paquete;
//bool envioPid = false;

int socketFd;

tiempot obtenerTiempo(char* tiempo)
{
	char** tiempov;
	tiempot t;

	tiempov = malloc(15);
	tiempov = string_split(tiempo, ":"); //Separa un string dado un separador, devuelve un vector de strings

	t.hora = atoi(tiempov[0]);
	t.minuto = atoi(tiempov[1]);
	t.segundo = atoi(tiempov[2]);

	free(tiempov);
	return t;
}

tiempot get_tiempo_total(tiempot in, tiempot fin)
{
	tiempot aux;
	aux.hora = fin.hora - in.hora;
	aux.minuto = fin.minuto - in.minuto;
	aux.segundo = fin.segundo - in.segundo;
	aux.segundo=fabs(aux.segundo);

	return aux;
}

bool estaPidEnEjecion(int pid)
{
	_Bool valor=false;
	_Bool _tieneElPid(hiloPrograma_t *prog) {
		return (prog->pid == pid);
	}
	sem_wait(&mutexLista);
	valor = list_any_satisfy(lista_programas, (void*) _tieneElPid);
	sem_post(&mutexLista);

	return valor;
}

void liberarHilo(hiloPrograma_t * programa )
{
	free(programa->ruta);
	free(programa->hilo);
	sem_destroy(&programa->finDehilo);
	sem_destroy(&programa->sem_hilo);
	free(programa);
}

void finalizarThread(hiloPrograma_t * programa )
{
	sem_wait(&programa->finDehilo);
	if(pthread_cancel(*programa->hilo) == 0)
	{
		log_debug(ptrLog,"Se finalizo correctamente el hilo programa con PID= %d", programa->pid);
		pthread_join(*programa->hilo, (void**) NULL);
	}else{
		log_error(ptrLog,"no se pudo matar el hilo con PID= %d", programa->pid);
	}
}

void finalizarPrograma(int pid, int operacion)
{
	hiloPrograma_t *programa;
	//int pid = atoi(spid);

	_Bool _tieneElPid(hiloPrograma_t *prog) {
		return (prog->pid == pid);
	}

	log_debug(ptrLog, "Se solicito finalizacion con operacion: %d del programa con pid:%i", operacion, pid);

	sem_wait(&mutexLista);
	programa =list_remove_by_condition(lista_programas, (void*) _tieneElPid);
	sem_post(&mutexLista);

	if(programa != NULL)
	{
		switch(operacion)
		{
			case FINALIZAR_PROGRAMA:
			case EXIT:
				imprimirDatosPrograma(programa);
				break;
			default:
				if(enviarPid(socketFd,pid)<=0)
					log_error(ptrLog, "Error al enviar mensaje finalizacion a Kernel socket= %d, pid:%i, se finalizo el programa incorrectamente", socketFd, pid);
				log_debug(ptrLog, "Se envio correctamente le finalizacion de programa, PID= %d, Socket Kernel= %d", pid, socketFd);
				finalizarThread(programa);
				imprimirDatosPrograma(programa);
				//Si activo esta da Segmentation Fault
				//liberarHilo(programa);
				break;
		}

	}else{
		log_error(ptrLog,"Pid inexistente en lista de programas en ejecucion, PID: %d",pid);

	}
}

void desconectarConsola()
{
	//ver uso de signal
	//ES ABORTIVO ESTO
	printf(">>desconectando consola...\n");
	int cantHilos;


	while(list_size(lista_programas)!=0)
	{
		hiloPrograma_t* prog;

		sem_wait(&mutexLista);
		prog = (hiloPrograma_t*) list_get(lista_programas, 0);
		sem_post(&mutexLista);

		finalizarPrograma(prog->pid,-1);
		//imprimirDatosPrograma(prog);
	}
	free(lista_programas);

	log_info(ptrLog,"se han finalizado todos los programas debido al comando \"desconectarConsola\" \n");
	printf(">>se han finalizado todos los programas debido al comando \"desconectarConsola\", "
			"a continuación se desconectará la consola\n\n");

	/*
	paquete->operacion=DESCONECTAR_CONSOLA;



	header_t header;

	header.type=DESCONECTAR_CONSOLA;
	header.length=0;
	sockets_send(socketFd,&header,'\0');
	*/

	exit(1);
}

void gestionarConsola()
{

	char* mensaje = malloc(1000);
	char* ruta = malloc(1000);
	char* spid = malloc(10);
	int i,cantLeido,pid;

	while (1) {

		printf("MENU:\n 1-iniciarPrograma\n 2-finalizarPrograma \n 3-desconectarConsola\n 4-limpiarPantalla \n ");
		printf(">>INGRESAR NUMERO DE COMANDO\n");

		cantLeido = scanf("%s", mensaje);

		printf(">>este es el comando: %s\n", mensaje);

		if (strcmp(&mensaje[0], "1") == 0) { //iniciar programa
			printf(">>ingrese la ruta del archivo a ejecutar.\n");

			scanf("%s", ruta);
			if (strcmp(&ruta[0], " ") != 0) {

				printf(">> esta es la ruta %s\n\n", ruta);
				crearHiloPrograma(ruta); //creo hilo de programa

			} else {
				printf(">>faltan parametros\n");
			}
		} else if (strcmp(&mensaje[0], "2") == 0) { //finalizar programa
			printf(">>ingrese el pid del programa a eliminar\n");

			scanf("%s", spid);

			if (strcmp(&spid[0], "") != 0) { //copio el pid del programa que quiero terminar
				pid=atoi(spid);
				finalizarPrograma(pid,-1);

			} else {
				printf(">>faltan parametros\n");

			}
		} else if (strcmp(&mensaje[0], "3") == 0) {//desconectar consola
			printf(">>se finalizarán todos los programas.\n");
			desconectarConsola();
		} else if (strcmp(&mensaje[0], "4") == 0) {//limpiar mensajes de la pantalla
			system("clear");
		} else {
			printf(">>no entiendo el comando: %s\n", mensaje);
		}

	}
	free(spid);
}

void crearHiloUsuario()
{
	int valorHilo = -1;
	valorHilo = pthread_create(&hiloU, NULL, (void*) gestionarConsola, NULL);
	if (valorHilo != 0) {
		perror(">>Error al crear hilo de Usuario\n\n");
		log_error(ptrLog, "error al crear el hilo de usuario\n");
		exit(1);
	} else {
		printf(">>se creó el hilo de usuario\n");
		log_info(ptrLog, "el hilo de usuario se creó correctamente\n");
	}

}

int getTamanioArchivo(char * ruta)
{

	FILE *archivoANSISOP = fopen(ruta, "r");
	if (archivoANSISOP == NULL) {
		printf(">>Error de apertura en script ANSISOP:%s\n\n", ruta);
		//fclose(archivoANSISOP);
		return -1;
	}

	int i = 0;
	while (!feof(archivoANSISOP)) {
		fgetc(archivoANSISOP);
		i++;
	}
	fclose(archivoANSISOP);
	return i;
}

char * getContenidoArchivo(char * ruta, int size)
{
	char *contenido=string_new();
	//contenido = malloc(size + 1);
	strcpy(contenido, "");

	FILE * archivoANSISOP = fopen(ruta, "r");
	if (archivoANSISOP == NULL) {
		printf(">>Error de apertura en script ANSISOP:%s\n\n", ruta);
		return "\0";
	} else { /*Recorro el archivo y lo voy anexando linea por linea a la
	 variable programaANSISOP*/
		do {
			char cadena[MAX_LINEA];
			fgets(cadena, MAX_LINEA, archivoANSISOP);
			string_append(&contenido, cadena);
		} while (!feof(archivoANSISOP) && (strlen(contenido) < size));

		fclose(archivoANSISOP);
	}
	return contenido;
}

int recvPid(int socket, uint32_t length)
{

	char* spid;
	spid = malloc(length);

	int bytes2=recv(socket, spid, length, MSG_WAITALL);

	uint32_t *pid=uint32_deserializer(spid);

	return(*pid);

	free(pid);
	free(spid);
}

t_datos_programa *recvPrograma(int socket, uint32_t length) {
	char* sprog;
	sprog = malloc(length);

	int bytes2 = recv(socket, sprog, length, MSG_WAITALL);

	t_datos_programa *prog = datosPrograma_deserializer(sprog);

	return (prog);

	free(prog);
	free(sprog);
}

int enviarPid(int socket, int pid)
{
	header_t header;
	header.type =FINALIZAR_PROGRAMA;
	header.length=sizeof(uint32_t);

	uint32_t *notificacion_pid = malloc(sizeof(uint32_t));

	*notificacion_pid = pid;

	char *serialized = uint32_serializer(notificacion_pid, &header.length);

	free(notificacion_pid);

	return sockets_send(socket, &header, serialized);
}

int recibirMensajeKernel(int socket)
{
	sem_wait(&ProxPaquete);

	header_t header;
	int nuevoPid;
	paquete = malloc(sizeof(paquete_t));
	t_datos_programa* datosImpresion;


	recv(socket, &header, sizeof(header_t), MSG_WAITALL);

	//Por ahora no creo necesario mandar el okey
	//enviarHeader(socket, SUCCESS);

	switch(header.type){
		//Mensajes recibidos por socketKernel

		case ENVIO_PID:
			paquete->operacion=ENVIO_PID;
			paquete->pid=recvPid(socket, header.length);
			log_debug(ptrLog, "El programa recién creado tiene como PID: %d", paquete->pid);
			sem_post(&recibirPid);
			break;

		//Finalizacion existosa de un programa
		case EXIT:
			paquete->operacion=EXIT;
			paquete->pid=recvPid(socket, header.length);
			if (estaPidEnEjecion(paquete->pid))
			{
				log_debug(ptrLog, "El programa con PID: %d finalizo Correctamente, se inicia fin de Hilo", paquete->pid);
				sem_post(&obtenerHiloPrograma(paquete->pid)->sem_hilo);
			}else{
				log_debug(ptrLog, "Se recibio un paquete de datos EXIT con PID: %d, que se ignorara dado que el PID fue finalizado por la consola"
						"", paquete->pid, paquete->buffer);
				sem_post(&ProxPaquete);
			}
			break;

		case FINALIZAR_PROGRAMA:
			paquete->operacion=FINALIZAR_PROGRAMA;
			nuevoPid=recvPid(socket, header.length);
			paquete->pid=nuevoPid;
			if (estaPidEnEjecion(paquete->pid))
			{
				log_debug(ptrLog, "el programa con Pid:%i fue finalizado en Kernel\n",nuevoPid);
				sem_post(&obtenerHiloPrograma(paquete->pid)->sem_hilo);
			}else{
				log_debug(ptrLog, "Se recibio un paquete de datos FINALIZAR_PROGRAMA con PID: %d, que se ignorara dado que el PID fue finalizado por la consola"
						"", paquete->pid, paquete->buffer);
				sem_post(&ProxPaquete);
			}
			break;

		case IMPRIMIR_ARCHIVO:
			paquete->operacion=IMPRIMIR_ARCHIVO;
			datosImpresion=recvPrograma(socket,header.length);
			paquete->buffer=datosImpresion->Script;
			paquete->pid=datosImpresion->PID;
			//Con los datos del PID se busca en la lista de programas y se activa el semaforo correspondiente
			if (estaPidEnEjecion(paquete->pid))
			{
				log_debug(ptrLog, "Se recibio un paquete de datos con PID: %d, Prints: %s", paquete->pid, paquete->buffer);
				sem_post(&obtenerHiloPrograma(paquete->pid)->sem_hilo);
			}else{
				log_debug(ptrLog, "Se recibio un paquete de datos IMPRIMIR_ARCHIVO con PID: %d, que se ignorara dado que el PID fue finalizado por la consola"
						"", paquete->pid, paquete->buffer);
				sem_post(&ProxPaquete);
			}
			break;

		default:
			log_error(ptrLog,"Protocolo Inesperado");
			return 1;
			}
	return 0;
}


void hiloPrograma(hiloPrograma_t * programa)
{

	log_debug(ptrLog,"Comienza a trabajar con el archivo ingresado, path: %s, Numero de hilo de ejecucion: %d",programa->ruta, *programa->hilo);

	char* tauxiliar;

	//Usando al semaforo como parte de la estructura del hilo no veo necesario mantener el idPrograma
	//sem_wait(&mutexCantProgramas);
	//cantProgramas++;
	///sem_post(&mutexCantProgramas);

	//programa->idprograma=cantProgramas;
	sem_init(&programa->sem_hilo,0,0);
	sem_init(&programa->finDehilo,0,0);

	sem_wait(&mutexLista);
	list_add(lista_programas, programa);
	sem_post(&mutexLista);

	programa->cantPrintF = 0;
	tauxiliar = temporal_get_string_time();
	programa->tiempoInicial = obtenerTiempo(tauxiliar);

	char * contenidoArchivo = getContenidoArchivo(programa->ruta, programa->tamanio);

	log_debug(ptrLog,"Contenido del Script a enviar: %s ,Tamanio: %d\n",contenidoArchivo,programa->tamanio);

	pthread_mutex_lock(&mutex);
	//Zona Critica envio y Recepcion de Pid
	int cantEnviado = enviarProgramaAnsisop(socketFd, contenidoArchivo);
	if (cantEnviado == -1) {

		printf(">>error al mandar el programa Ansisop\n");
		log_error(ptrLog, "error al mandar el programa Ansisop de ruta %s al kernel\n", programa->ruta);
		pthread_exit(NULL);
	} else {
		log_info(ptrLog, "se ha mandado correctamente el programa Ansisop de ruta %s al kernel",programa->ruta);

	}

	//Esta zona es critica dado que se recibe secuencialmente el PID solo un hilo debe estar a la espera de su PID
	//El resto de los hilos puede llegar a estar bloqueado dentro del While, esperando un mensaje de impresion

	sem_wait(&recibirPid);
	programa->pid = paquete->pid;
	printf("Proceso con PID Asignado: %d\n", programa->pid);
	pthread_mutex_unlock(&mutex);

	sem_post(&ProxPaquete);// VER ESTE POST SI ESTA BIEN ACA
	sem_post(&programa->finDehilo);

	log_debug(ptrLog, "El programa en procesamiento tiene los siguientes datos pid=%d",programa->pid);

	//Iterar Mientras no sea Fin de Programa
	while(1)
	{
		//Un Semaforo que bloquee mientras no llegue ningun paquete para el hilo
		sem_wait(&programa->sem_hilo);
		sem_wait(&programa->finDehilo);

		switch(paquete->operacion)
		{
			case IMPRIMIR_ARCHIVO:
				programa->cantPrintF++;
				printf("PID: %d -->", programa->pid);
				printf("Impresion por Pantalla: %s\n", paquete->buffer);
				break;

			case EXIT:
				printf("PID: %d -->", programa->pid);
				printf("Finalizo su ejecucion Correctamente\n");
				finalizarPrograma(programa->pid,EXIT);
				sem_post(&ProxPaquete);
				return;

			case FINALIZAR_PROGRAMA:
				printf("Se finalizo programa desde Kernel con PID: %d \n",paquete->pid);
				finalizarPrograma(programa->pid,FINALIZAR_PROGRAMA);
				sem_post(&ProxPaquete);
				return;

		}
		sem_post(&ProxPaquete);
		sem_post(&programa->finDehilo);
	}

}


int enviar(int socketFd, char* datos){
	int cantEnviado;
	header_t header;
	header.type = FINALIZAR_PROGRAMA;
	header.length = strlen(datos) + 1;

	log_debug(ptrLog,"Operacion enviada a Kernel: %d",header.type);

	return cantEnviado;

}

int enviarProgramaAnsisop(int socketFd, char* datos )
{
	//mando mensaje a kernel con el codigo
	int cantEnviado;
	header_t header;
	header.type=INICIAR_PROGRAMA;
	header.length = strlen(datos) + 1;

	//Serializo el Codigo a enviar
	//notificacion_datos_programa_t *notificacion = malloc(sizeof(notificacion_datos_programa_t));
	//notificacion->Script = malloc(strlen(datos) + 1);
	//strcpy(notificacion->Script, datos);

	//char *serialized = notificacionDatosPrograma_serializer(notificacion,&header.length);

	log_debug(ptrLog,"Operacion enviada a Kernel: %d",header.type);

	cantEnviado = sockets_send(socketFd,&header,datos);

	//free(notificacion->Script);
	//free(notificacion);

	return cantEnviado;
}


void imprimirDatosPrograma(hiloPrograma_t *prog)
{
	char* tauxiliar=malloc(1000);
	tauxiliar = temporal_get_string_time();

	prog->tiempoFinal = obtenerTiempo(tauxiliar);

	prog->tiempoTotal = get_tiempo_total(prog->tiempoInicial,
			prog->tiempoFinal);


	printf(">>el tiempo inicial del programa con pid: %i es %i:%i:%i\n",
			prog->pid, prog->tiempoInicial.hora, prog->tiempoInicial.minuto,
			prog->tiempoInicial.segundo);
	log_info(ptrLog,">>el tiempo inicial del programa con pid: %i es %i:%i:%i\n",
			prog->pid, prog->tiempoInicial.hora, prog->tiempoInicial.minuto,
			prog->tiempoInicial.segundo);

	printf(">>el tiempo final del programa con pid:%i es %i:%i:%i\n", prog->pid,
		prog->tiempoFinal.hora, prog->tiempoFinal.minuto,
			prog->tiempoFinal.segundo);
	log_info(ptrLog,">>el tiempo final del programa con pid:%i es %i:%i:%i\n", prog->pid,
			prog->tiempoFinal.hora, prog->tiempoFinal.minuto,
				prog->tiempoFinal.segundo);

	printf(">>el programa con pid:%i hizo %i impresiones por pantalla\n",
			prog->pid, prog->cantPrintF);
	log_info(ptrLog,">>el programa con pid:%i hizo %i impresiones por pantalla\n",
			prog->pid, prog->cantPrintF);

	printf(">>el tiempo total del programa con pid:%i es %i:%i:%i\n\n", prog->pid,
			prog->tiempoTotal.hora, prog->tiempoTotal.minuto,
			prog->tiempoTotal.segundo);
	log_info(ptrLog,">>el tiempo total del programa con pid:%i es %i:%i:%i\n", prog->pid,
			prog->tiempoTotal.hora, prog->tiempoTotal.minuto,
			prog->tiempoTotal.segundo);

	free(tauxiliar);
}

void crearHiloPrograma(char *ruta)
{
	int valorHilo = -1;

	hiloPrograma_t* prog= malloc(sizeof(hiloPrograma_t));
	prog->ruta = malloc(1000);
	prog->ruta = ruta;
	prog->hilo = malloc(sizeof(pthread_t));

	prog->tamanio = getTamanioArchivo(prog->ruta);

	if(prog->tamanio>0){
		printf(">>ruta: %s \n", prog->ruta);

		log_debug(ptrLog,"Comienza la creacion del Hilo programa - funcion crearHiloPrograma");
		valorHilo = pthread_create(prog->hilo, NULL, (void*) hiloPrograma, (void*) prog);


		if (valorHilo != 0) {
			perror(">>Error al crear hilo de Programa\n\n");
			log_error(ptrLog, "no se pudo crear el hilo de Programa para la ruta:%s",prog->ruta);
			exit(1);

		} else {
			printf(">>se creó un hilo Programa\n");
			log_info(ptrLog, "se creó el hilo programa para la ruta:%s", prog->ruta);

		}
	}else{
		printf("la ruta especificada no existe\n");
	}

}

hiloPrograma_t * obtenerHiloPrograma(uint32_t pid)
{
	hiloPrograma_t* programa;

	bool _tieneElPid(hiloPrograma_t* prog) {
		return (prog->pid) == pid;
	}

	sem_wait(&mutexLista);
	programa = list_find(lista_programas, (void*) _tieneElPid);
	sem_post(&mutexLista);


	return programa;
}

int main(int cantArg, char** vectorArg)
{
	verificarParametros(cantArg);
	char*ruta;
	header_t header;
	ruta= leerParametros(vectorArg);
	int i;


	archivoConfig archC;
	archC = leerArchivoConfiguracion(ruta);

	lista_programas=list_create();

	remove(LOG_PATH);
	ptrLog = log_create(LOG_PATH, "Proceso Consola", false, archC.detallelog);

	//Se inicializan Semaforos
	//sem_init(&mutexCantProgramas, 0, 1);
	sem_init(&mutexLista,0,1);
	sem_init(&recibirPid,0,0);
	sem_init(&ProxPaquete,0,1);

	//for(i=0;i<MAX_HILOS;i++)
		//sem_init(&HiloPrograma[i],0,0);

	pthread_mutex_init(&mutex, NULL);

	socketFd = AbrirConexion(archC.ip_kernel, archC.puerto_kernel);

	enviarHandshake(socketFd, HANDSHAKE_CONSOLA);

	recv(socketFd, &header, sizeof(header_t), MSG_WAITALL);

	if (header.type == HANDSHAKE_KERNEL) {

		crearHiloUsuario();

	} else {
		log_warning(ptrLog, "Protocolo inesperado");
		exit(EXIT_FAILURE);
	}

	while (1) {
		recibirMensajeKernel(socketFd);
		}


	pthread_join(hiloU, NULL);

	//Se eliminan Estructuras
	//sem_destroy(&mutexCantProgramas);
	sem_destroy(&mutexLista);

	sem_destroy(&ProxPaquete);

	//for(i=0;i<MAX_HILOS;i++)
		//sem_destroy(&HiloPrograma[i]);

	pthread_mutex_destroy(&mutex);

	list_destroy(lista_programas);

	log_destroy(ptrLog);

	return 0;
}

