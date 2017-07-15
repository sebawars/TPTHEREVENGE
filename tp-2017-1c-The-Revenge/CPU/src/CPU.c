/*
 * CPU.c
 *
 *  Created on: 7/4/2017
 *      Author: utnso
 */

#include "CPU.h"
#include "configCpu.h"
#include <Sockets/Serializer.h>
#include <Sockets/StructsUtils.h>
#include "primitivasAnsisop.h"

AnSISOP_funciones functions = { .AnSISOP_obtenerPosicionVariable = obtenerPosicionVariable,
								.AnSISOP_dereferenciar = dereferenciar,
								.AnSISOP_asignar = asignar,
								.AnSISOP_definirVariable = definirVariable,
								.AnSISOP_obtenerValorCompartida = obtenerValorCompartida,
								.AnSISOP_asignarValorCompartida = asignarValorCompartida,
								.AnSISOP_irAlLabel = irAlLabel,
								.AnSISOP_llamarSinRetorno = llamarSinRetorno,
								.AnSISOP_llamarConRetorno = llamarConRetorno,
								.AnSISOP_finalizar = finalizar,
								.AnSISOP_retornar = retornar
};

AnSISOP_kernel kernel_functions = { .AnSISOP_signal = ansisop_signal,
									.AnSISOP_wait = wait,
									.AnSISOP_reservar = reservar,
									.AnSISOP_liberar = liberar,
									.AnSISOP_abrir = abrir,
									.AnSISOP_borrar = borrar,
									.AnSISOP_cerrar = cerrar,
									.AnSISOP_moverCursor = moverCursor,
									.AnSISOP_leer = ansisop_leer,
									.AnSISOP_escribir = ansisop_escribir
};
bool cerrarCPU = false;
bool huboStackOver = false;
uint32_t operacion=NOTHING;

int main (int cantArg, char** vectorArg){

	char* ruta;
	verificarParametros(cantArg);


	ruta = leerParametros(vectorArg);
	archivoConfigCPU* configFile = leerArchivoConfiguracion(ruta);

	//Esto elimina logs y en caso de varios procesos solo queda logueando uno de ellos
	//remove(LOG_PATH);
	ptrLog = log_create(LOG_PATH, "Proceso CPU", false,configFile->detallelog);

	//Establezco conexion con Memoria
	socketMemoria=AbrirConexion(configFile->ip_memoria,configFile->puerto_memoria);
	if(socketMemoria==-1)
	{
		log_error(ptrLog,"Error al crear socket");
		printf("Ha ocurrido un error intentando conectarse a la Memoria\n");
		return EXIT_FAILURE;
	}
	log_info(ptrLog, "Se envia handshake CPU");
	enviarHandshake(socketMemoria,HANDSHAKE_CPU);

	if(aceptarConexion(socketMemoria,HANDSHAKE_MEMORIA)==0){
		printf("Conexion con la Memoria establecida, Socket: %d\n", socketMemoria);
		log_info(ptrLog,"Conexion con la Memoria establecida, Socket: %d.", socketMemoria);
	}else{
		printf("Protocolo Inesperado en conexion con Memoria");
		log_info(ptrLog,"Protocolo Inesperado");
		return EXIT_FAILURE;
	}

	//Establezco conexion con Kernel
	socketKernel=AbrirConexion(configFile->ip_kernel,configFile->puerto_kernel);
	if(socketKernel==-1)
	{
		log_error(ptrLog,"Error al crear socket");
		printf("Ha ocurrido un error intentando conectarse al Kernel\n");
		return EXIT_FAILURE;
	}
	log_info(ptrLog, "Se envia handshake CPU");
	enviarHandshake(socketKernel,HANDSHAKE_CPU);

	if(aceptarConexion(socketKernel,HANDSHAKE_KERNEL)==0){
		printf("Conexion con el Kernel establecida, Socket: %d\n", socketKernel);
		log_info(ptrLog,"Conexion con la Memoria establecida, Socket: %d.", socketKernel);
	}else{
		printf("Protocolo Inesperado en conexion con Kernel");
		return EXIT_FAILURE;
	}

	/*
	 * Manejo de la interrupcion SIGUSR1
	 */
	signal(SIGUSR1 , revisarSigusR1);
	if(!recibirMensajeInicialDeMemoria(socketMemoria)){

		return controlarConexiones();
	}else{
		printf("Error en recepcion tamanio Pagina, CPU no puede iniciar");
		free(configFile);
		return EXIT_FAILURE;
	}

	free(configFile);
	return 0;
}

int controlarConexiones(){
	while(1){
		log_info(ptrLog,"Esperando mesajes de Kernel");
		if(!recibirMensaje(socketKernel)){

		}else{
			log_error(ptrLog,"Se perdio la conexion con Kernel, se desconectara el proceso CPU");
			return 0;
		}
	}
	return 0;
}

void revisarSigusR1(int signo){

	if(signo == SIGUSR1){

		log_debug(ptrLog, "Se recibe SIGUSR1");

		header_t header;
		header.type = FINALIZAR_SIGUSR1;
		header.length=0;
		char buffer='\0';

		sockets_send(socketKernel,&header,&buffer);

		if(operacion == NOTHING){
			log_debug(ptrLog, "Se cierra socketKernel, el CPU ya esta liberado");
			close(socketKernel);
			return;
		}

		cerrarCPU = true;
		log_debug(ptrLog, "Termina la rafaga actual y luego se cierra la CPU");
	}

}

int recibirMensaje(socket)
{
	header_t header;
	header.type=-1;
	header.length=0;

	recv(socket, &header, sizeof(header_t), MSG_WAITALL);

	switch(header.type){
		//Mensajes recibidos por socketKernel
		case TAMANIO_STACK:
			recibirTamanio(socket,header.length,&iTamStack);
			log_info(ptrLog, "Inicilizando CPU, tamStack:%d",iTamStack);
			printf("tamStack: %d\n",iTamStack);
			break;
		case EXECUTE_PCB:
			operacion=CPU_RUNNING;
			recibirPCB(socket,header.length);
			break;
		default:
			log_error(ptrLog,"Protocolo Inesperado - Se procedera a la desconexion de la CPU");
			return 1;
		}
	return 0;
}

int recibirMensajeInicialDeMemoria(socket)
{
	header_t header;

	recv(socket, &header, sizeof(header_t), MSG_WAITALL);

	switch(header.type){
		//Mensajes recibidos por socketKernel
		case TAMANIO_PAGINA:
			recibirTamanio(socket,header.length,&tamanioPagina);
			log_info(ptrLog, "Inicilizando CPU, tamPagina:%d",tamanioPagina);
			printf("tamPagina: %d\n",tamanioPagina);
			break;
		default:
			log_error(ptrLog,"Protocolo Inesperado");
			return 1;
		}
	return 0;
}

char * recibirInstruccion(socket)
{
	header_t header;
	char *respuesta;

	recv(socket, &header, sizeof(header_t), MSG_WAITALL);

	if(header.type==SUCCESS)
	{
		//respuesta = malloc(header.length + 1);
		respuesta = malloc(header.length);

		recv(socket, respuesta, header.length, MSG_WAITALL);

		//Hay que tener Cuidado con esto porque me genera Segmentation Fault
		//Agrego caracter Nulo para loguear
		//char * aux=malloc(header.length+1);
		//aux=respuesta;
		//aux[header.length] = '\0';
		//log_debug(ptrLog,"Cadena recibida de Memoria: %s",respuesta);
		//Para loguear

		return respuesta;
	}else{
		log_error(ptrLog,"Error al recibir instruccion de Memoria");
		operacion=ERROR_MEMORIA;
		return NULL;
	}

}

//Manejo de mensajes recibidos
void recibirTamanio(int socket, uint32_t tamanio, int32_t * elemento)
{
	char *respuesta = malloc(tamanio);

	recv(socket, respuesta, tamanio, MSG_WAITALL);

	uint32_t *notificacion_tamanio = uint32_deserializer(respuesta);

	*elemento=*notificacion_tamanio;

	free(notificacion_tamanio);
	free(respuesta);
}

void recibirPCB(int socket,uint32_t tamanio) {

	char *mensaje = malloc(tamanio);

	recv(socket, mensaje, tamanio, MSG_WAITALL);

	pcb = deserializer_pcb(mensaje);

	free(mensaje);
	setPCB(pcb);
	comenzarEjecucionDePrograma();
}

void setPCB(t_PCB * pcbDeCPU) {
	pcb = pcbDeCPU;
}

void comenzarEjecucionDePrograma() {
	log_info(ptrLog, "Recibo PCB id: %i", pcb->PID);
	int contador=1;

	//Si es RR ejecutara hasta la primer condicion || Si es FIFO ejecutara hasta que termine
	while(contador<=pcb->quantum || pcb->quantum==0)
	{
		char* proximaInstruccion = solicitarProximaInstruccionAMemoria();

		limpiarInstruccion(proximaInstruccion);

		log_debug(ptrLog,"Instruccion recibida y depurada de: %s",proximaInstruccion);

		if (proximaInstruccion != NULL) {

			//Ver como y porque se puede llegar con este mensaje desde la memoria
			if (strcmp(proximaInstruccion, "FINALIZAR") == 0) {
				log_error(ptrLog, "Instruccion no pudo leerse. Hay que finalizar el Proceso.");
				return;
			} else {

				analizadorLinea(proximaInstruccion, &functions,&kernel_functions);
				free(proximaInstruccion);

				if (huboStackOver) {
					printf(">>>Hubo StackOverflow... Se finalizara el proceso.\n");
					finalizarEjecucion(ERROR_STACKOVERFLOW);
					revisarfinalizarCPU();
					log_error(ptrLog, "Proceso finalizado por StackOver");
					return;
				}

				//revisar despues bien el orden si incrementar o no despues de un error o no, lo mismo con el end.
				contador++;
				pcb->rafagas=+contador;
				pcb->PC++;


				switch (operacion) {
					case WAIT:
						log_debug(ptrLog, "Finalizo ejecucion por un Wait.");
						finalizarEjecucion(WAIT);
						revisarfinalizarCPU();
						return;
					case ERROR_APERTURA:
						log_debug(ptrLog, "Finalizo ejecucion por Error de Apertura de Archivo.");
						printf(">>>Error de Apertura de Archivo... Se finalizara el proceso.\n");
						finalizarEjecucion(ERROR_APERTURA);
						revisarfinalizarCPU();
						return;
					case ERROR_ESCRIBIR:
						log_debug(ptrLog, "Finalizo ejecucion por Error de Escritura de Archivo.");
						printf(">>>Error de Escritura de Archivo... Se finalizara el proceso.\n");
						finalizarEjecucion(ERROR_ESCRIBIR);
						revisarfinalizarCPU();
						return;
					case ERROR_MEMORIA:
						log_debug(ptrLog, "Finalizo ejecucion por Error de Memoria.");
						printf(">>>Error de Memoria... Se finalizara el proceso.\n");
						finalizarEjecucion(ERROR_MEMORIA);
						return;
					case ERROR_HEAP:
						log_debug(ptrLog, "Finalizo ejecucion por Error en Liberar, Error Heap.");
						printf(">>>Error de Heap... Se finalizara el proceso.\n");
						finalizarEjecucion(ERROR_HEAP);
						return;
					case ALLOC_MAYOR_TAM_PAGINA:
						log_debug(ptrLog, "Finalizo ejecucion por Error en Alocar - Se supero el Tamanio de Pagina Maximo, Error Heap.");
						printf(">>>Error Alloc tamaño pedido supera tamaño de pagina... Se finalizara el proceso.\n");
						finalizarEjecucion(ALLOC_MAYOR_TAM_PAGINA);
						return;
					case NO_PUDO_ASIGNAR_PAGINAS:
						log_debug(ptrLog, "Finalizo ejecucion por Error en Alocar - No se pudo asignar mas paginas al programa, Error Heap.");
						printf(">>>Error no se pudo asignar Paginas... Se finalizara el proceso.\n");
						finalizarEjecucion(NO_PUDO_ASIGNAR_PAGINAS);
						return;
					case ERROR:
						log_debug(ptrLog, "Finalizo ejecucion por Error sin definicion.");
						printf(">>>Error Generico sin definicion... Se finalizara el proceso.\n");
						finalizarEjecucion(ERROR);
						revisarfinalizarCPU();
						return;
					case EXIT:
						finalizarEjecucion(EXIT);
						revisarfinalizarCPU();
						log_debug(ptrLog, "Finalizo la ejecucion del Proceso correctamente.");
						return;
					}

					//Esto solo aplica si se trabaja bajo el algoritmo RR
					if(pcb->quantum!=0){
						usleep(pcb->quantum_sleep * 1000);
						log_debug(ptrLog, "Quantum Sleep utilizado es: %d",pcb->quantum_sleep);
					}
			}

		}else{
			log_info(ptrLog, "No se pudo recibir la instruccion de UMC. Cierro la conexion");
			return;

		}
	}

	//Por lo que veo la unica forma de salir por aca es por fin de Quantum
	log_debug(ptrLog, "Finalizo ejecucion por fin de Quantum");
	finalizarEjecucion(QUANTUM_TERMINADO);
	revisarfinalizarCPU();

	return;
}

char * solicitarProximaInstruccionAMemoria() {

	header_t header;
	header.type=LEER;
	uint32_t longitudAPedir,inicioAPedir,longitudPedida=0,offset=0,tmpSize=0;
	t_indice_codigo *indice = list_get(pcb->IC, pcb->PC);
	uint32_t inicio = indice->inicio;
	uint32_t longitudInstruccion = indice->longitud;
	char * instruccionRecibida;
	char * instruccion = malloc(indice->longitud + 1);
	char * serialized;

	log_debug(ptrLog, "Inicio de Instruccion: %d, Longitud de Instruccion: %d",inicio,longitudInstruccion);

	t_solicitudLectura * solicitarBytes = malloc(sizeof(t_solicitudLectura));

	longitudAPedir=longitudInstruccion;

	uint32_t contador = 0;
	while (inicio >= (tamanioPagina + (tamanioPagina * contador))) {
		contador++;
	}

	uint32_t paginaAPedir = contador;
	inicioAPedir=inicio - (tamanioPagina * paginaAPedir);

	solicitarBytes->pid = pcb->PID;

	while(longitudPedida<longitudInstruccion){

		pcb->paginaCodigoActual = paginaAPedir;

		solicitarBytes->pagina = paginaAPedir;
		solicitarBytes->inicio = inicioAPedir;

		if(inicioAPedir + longitudAPedir > tamanioPagina)
		{
			longitudAPedir = tamanioPagina-inicioAPedir;
			solicitarBytes->offset = longitudAPedir;
			paginaAPedir++;
		}else{
			solicitarBytes->offset = longitudAPedir;
		}

		serialized = solicitudLectura_serializer(solicitarBytes,&header.length);

		log_debug(ptrLog, "Solicito a Memoria-> Pagina: %d - Inicio: %d - Offset: %d - Header.length: %d",solicitarBytes->pagina,solicitarBytes->inicio ,solicitarBytes->offset, header.length);

		int nbytes=sockets_send(socketMemoria,&header,serialized);

		free(serialized);

		if (nbytes > 0) {

			instruccionRecibida = recibirInstruccion(socketMemoria);

			if(instruccionRecibida!=NULL)
			{
				memcpy(instruccion+offset,instruccionRecibida,tmpSize=longitudAPedir);
				offset+=tmpSize;
				free(instruccionRecibida);
				inicioAPedir=0;
				longitudPedida += longitudAPedir;
				longitudAPedir = longitudInstruccion - longitudPedida;
			}else{
				return NULL;
			}

		}else{

			log_error(ptrLog, "Ocurrio un error al enviar una solicitud de lectura de instruccion a la Memoria");
			return NULL;
		}

	}
	free(solicitarBytes);
	instruccion[longitudInstruccion]='\0';
	return instruccion;
}


void limpiarInstruccion(char * instruccion) {
	char *p2 = instruccion;
	int a = 0;
	while (*instruccion != '\0') {
		if (*instruccion != '\t' && *instruccion != '\n' && !iscntrl(*instruccion)) {
			if (a == 0 && isdigit((int )*instruccion)) {
				++instruccion;
			} else {
				*p2++ = *instruccion++;
				a++;
			}
		} else {
			++instruccion;
		}
	}
	*p2 = '\0';
}

void finalizarEjecucion(int8_t type)
{

	header_t header;

	header.type=type;

	operacion=NOTHING;

	if(huboStackOver==true)
		huboStackOver=false;

	char * serialized = serializer_pcb(pcb,&header.length);
	free(pcb);
	//freePCB();

	log_debug(ptrLog, "PCB serializado para enviar a Kernel. Finalizo por: %i, header.length= %d",type,header.length);

	int bytesEnviados= sockets_send(socketKernel,&header,serialized);

	if (bytesEnviados <= 0) {
		log_error(ptrLog, "Error al devolver el PCB por Finalizacion a Kernel");
	} else {
		log_info(ptrLog, "Pcb enviado a Kernel correctamente");
	}
	free(serialized);
	return;
}

void revisarfinalizarCPU()
{
	if(cerrarCPU)
	{
		log_debug(ptrLog, "Cerrando CPU por SIGUSR1");
		if(operacion==NOTHING){
			close(socketKernel);
			close(socketMemoria);
		}
		log_debug(ptrLog,"CPU Cerrada");
		free(config);
		exit(0);
	}
}

//Creo Funcion pero Pincha Por ahora
void freePCB()
{

	if(pcb->indice_etiquetas){
		free(pcb->indice_etiquetas);
	}

	while(!list_is_empty(pcb->IC))
	{
		t_indice_codigo * indice = list_remove(pcb->IC,0);
		free(indice);
	}
	//list_destroy(pcb->IC);

	while(!list_is_empty(pcb->indiceStack))
	{
		t_stack * linea = list_remove(pcb->indiceStack,0);
		if(linea->argumentos != NULL)
		{
			while(!list_is_empty(linea->argumentos))
			{
				if (linea->argumentos != NULL){
					t_argumento * argumento = list_remove(linea->argumentos,0);
					free(argumento);
				}
			}
			list_destroy(linea->argumentos);
		}

		if(linea->variables != NULL)
		{
			while(!list_is_empty(linea->variables))
			{
				t_variable * variable=list_remove(linea->variables ,0);
				free(variable);
			}
			list_destroy(linea->variables);
		}
	}
	//list_destroy(pcb->indiceStack);
	free(pcb);
}
