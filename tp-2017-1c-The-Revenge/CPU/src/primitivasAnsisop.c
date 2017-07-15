/*
 * primitivasAnsisop.c
 *
 *  Created on: 5/5/2017
 *      Author: utnso
 */

#include "primitivasAnsisop.h"

bool esArgumento(t_nombre_variable identificador_variable){
	if (isdigit(identificador_variable))
		return true;
	else
		return false;

}


t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable)
{
	if(!esArgumento(identificador_variable)){
		t_variable* variable=malloc(sizeof(t_variable));
		int i=0;
		log_debug(ptrLog,"Obtener posicion variable '%c'", identificador_variable);
		//Genero la linea de stack del contexto actual
		t_stack*lineaActualStack = list_get(pcb->indiceStack,pcb->indiceContextoEjecucionActualStack);
		//Me posiciono al inicio de esta linea de satck
		//me fijo cual variable de la lista coincide con el nombre que me estan pidiendo
		if(list_size((t_list*)lineaActualStack->variables) > 0){
			for(i=0;i<list_size(lineaActualStack->variables);i++){

				variable=list_get(lineaActualStack->variables, i);
				if (variable->idVariable == identificador_variable){
					t_puntero posicionRet=(variable->pagina*tamanioPagina) + variable->offset;
					return posicionRet;
				}
			}
		}
		//devuelvo -1 - si no esta en la linea de stack actual
		t_puntero posicionRet=-1;
		free(variable);
		return posicionRet;
	}else{
		t_argumento* argumento = malloc(sizeof(t_argumento));
		int i=0;
		log_debug(ptrLog,"Obtener posicion argumento '%c'", identificador_variable);
		//Genero la linea de stack del contexto actual
		t_stack*lineaActualStack = list_get(pcb->indiceStack,pcb->indiceContextoEjecucionActualStack);
		//Me posiciono al inicio de esta linea de satck
		//me fijo cual variable de la lista coincide con el nombre que me estan pidiendo
		if(list_size((t_list*)lineaActualStack->argumentos) > 0){
			int lineaArg = identificador_variable - '0';
			argumento=list_get(lineaActualStack->argumentos, i);
			t_puntero posicionRet=(argumento->pagina*tamanioPagina) + argumento->offset;
			return posicionRet;
		}
		//devuelvo -1 - si no esta en la linea de stack actual
		t_puntero posicionRet=-1;
		free(argumento);
		return posicionRet;
	}

}

void asignar(t_puntero direccion_variable, t_valor_variable valor)
{
	header_t header;

	log_debug(ptrLog, "Asignar. Posicion %d - Valor %d", direccion_variable, valor);
	//Calculo de la posicion de la variable en el stack mediante el desplazamiento

	t_solicitudEscritura* mensaje= malloc(sizeof(t_solicitudEscritura));
	mensaje->pagina= (direccion_variable / tamanioPagina);
	mensaje->offset= direccion_variable % tamanioPagina;
	mensaje->tamanio=TAMANIO_VARIABLE;
	mensaje->pid=pcb->PID;
	mensaje->buffer=malloc(sizeof(t_valor_variable));
	//sprintf(mensaje->buffer, "%d", valor);
	memcpy(mensaje->buffer,&valor,sizeof(t_valor_variable));

	header.type=ESCRIBIR_PAGINA;
	char * serializer = solicitudEscritura_serializer(mensaje,&header.length);

	int nbytes=sockets_send(socketMemoria,&header,serializer);

	if(nbytes<=0)
	{
		operacion=ERROR;
	}else{

		recv(socketMemoria, &header, sizeof(header_t), MSG_WAITALL);

		if(header.type==SUCCESS)
		{
			log_info(ptrLog,"Variable asignada correctamente");
		}else{
			log_error(ptrLog,"La variable no pudo asignarse. Se finaliza el proceso");
			operacion=ERROR_MEMORIA;
		}

	}
	free(mensaje->buffer);
	free(mensaje);
	free(serializer);
}

t_valor_variable dereferenciar(t_puntero direccion_variable)
{
	t_posicion_stack posicionRet;
	posicionRet.pagina= (direccion_variable/tamanioPagina);
	posicionRet.offset=	(direccion_variable%tamanioPagina);
	posicionRet.size=TAMANIO_VARIABLE;
	t_valor_variable valor=0;
	log_debug(ptrLog,"Dereferenciar %d",direccion_variable);
	t_solicitudLectura*solicitar=malloc(sizeof(t_solicitudLectura));
	solicitar->pid=pcb->PID;
	solicitar->pagina=posicionRet.pagina;
	solicitar->offset=TAMANIO_VARIABLE;
	solicitar->inicio=posicionRet.offset;

	header_t header;
	header.type=LEER;
	char *serializer=solicitudLectura_serializer(solicitar,&header.length);

	int nbytes=sockets_send(socketMemoria,&header,serializer);

	free(serializer);

	if(nbytes>0)
	{
		recv(socketMemoria, &header, sizeof(header_t), MSG_WAITALL);

		if(header.type==SUCCESS)
		{

			char * mensaje=malloc(header.length + 1);

			recv(socketMemoria, mensaje, header.length, MSG_WAITALL);
			mensaje[header.length]='\0';
			log_debug(ptrLog,"Se recibe valor desreferenciado de memoria %s",mensaje);
			//t_instruccion*instruccion=instruccion_deserializer(mensaje);

			//int valueAsInt=atoi(instruccion->instruccion);
			//int valueAsInt=atoi(mensaje);
			memcpy(&valor, mensaje,sizeof(t_valor_variable));
			log_debug(ptrLog,"Variable dereferenciada. Valor: %d",valor);
			free(mensaje);

		}else{

			log_error(ptrLog,"La variable no se pudo dereferenciar. Se finaliza el Proceso");
			operacion=ERROR_MEMORIA;
			free(solicitar);
			return 0;
		}


	}

	free(solicitar);
	return valor;
}

t_puntero definirVariable(t_nombre_variable identificador_variable)
{
	if(!esArgumento(identificador_variable)){//si entra a este if es porque es una variable, si no entra es porque es un argumento, hay que revisar si es del 0 a 9,

		log_debug(ptrLog,"Definir variable %c",identificador_variable);
		t_variable* nuevaVariable=malloc(sizeof(t_variable));
		t_stack* lineaStack= list_get(pcb->indiceStack,pcb->indiceContextoEjecucionActualStack);
		if(pcb->stackPointer + TAMANIO_VARIABLE > tamanioPagina && pcb->paginaStackActual - pcb->primerPaginaStack == iTamStack - 1){
			if(!huboStackOver){
				log_error(ptrLog, "Stackoverflow. Se finalizara el proceso");
				huboStackOver = true;
			}
			return -1;
		}else{
			if(lineaStack == NULL){
				//el tamaño de la linea del stack seria de los 4 ints mas
				uint32_t tamLineaStack = 7* sizeof(uint32_t)+1;
				lineaStack=malloc(tamLineaStack);
				lineaStack->retVar=NULL;
				lineaStack->direccion_retorno=0;
				lineaStack->argumentos=list_create();
				lineaStack->variables = list_create();
				list_add(pcb->indiceStack,lineaStack);
			}
			//me fijo si el offset de la ultima + el tamaño o superan o son iguales al tamaño de la pagina, si esto pasa tengo que pasar a una pagina nueva
			if(pcb->stackPointer + TAMANIO_VARIABLE > tamanioPagina){
				pcb->paginaStackActual++;
				nuevaVariable->offset=0;
				pcb->stackPointer=TAMANIO_VARIABLE;
			}else{
				nuevaVariable->offset=pcb->stackPointer;
				pcb->stackPointer+=TAMANIO_VARIABLE;
			}

			nuevaVariable->idVariable=identificador_variable;
			nuevaVariable->pagina=pcb->paginaStackActual;
			nuevaVariable->size=TAMANIO_VARIABLE;
			list_add(lineaStack->variables,nuevaVariable);

			//Calculo el desplazamiento desde la primer pagina del stack hasta donde arranca mi nueva variable
			uint32_t posicionRetorno=(nuevaVariable->pagina * tamanioPagina) + nuevaVariable->offset;
			log_debug(ptrLog,"nuevaVariable->idVariable= %c, nuevaVariable->pagina= %i, nuevaVariable->offset= %i, nuevaVariable->size= %i",nuevaVariable->idVariable, nuevaVariable->pagina, nuevaVariable->offset, nuevaVariable->size);
			return posicionRetorno;
		}

	}else{
		//en este caso es un argumento, realizar toda la logica aca y tambien obtener posicion de variable, asignar imprimir y retornar
		log_debug(ptrLog,"Definir Variable - argumento %c", identificador_variable);
		t_argumento* nuevoArgumento=malloc(sizeof(t_argumento));
		t_stack* lineaStack= list_get(pcb->indiceStack, pcb->indiceContextoEjecucionActualStack);
		if(pcb->stackPointer + TAMANIO_VARIABLE > tamanioPagina && pcb->paginaStackActual - pcb->primerPaginaStack == iTamStack - 1 ){
			if(!huboStackOver){
				log_error(ptrLog, "Stackoverflow. Se finalizara el proceso");
				huboStackOver = true;
			}
			return -1;
		}else{
			//me fijo si el offset de la ultima + el tamaño superan o son iguales al tamaño de la pagina si esto sucede, tengo que pasar a una pagina nueva
			if(pcb->stackPointer + TAMANIO_VARIABLE > tamanioPagina){
				pcb->paginaStackActual++;
				nuevoArgumento->offset = 0;
				pcb->stackPointer = TAMANIO_VARIABLE;
			}else{
				nuevoArgumento->offset = pcb->stackPointer;
				pcb->stackPointer += TAMANIO_VARIABLE;
			}
			nuevoArgumento->pagina = pcb->paginaStackActual;
			nuevoArgumento->size = TAMANIO_VARIABLE;
			list_add(lineaStack->argumentos,nuevoArgumento);

			//Calculo el desplazamiento desde la primer pagina del stack hasta donde arranca mi nueva variable
			uint32_t posicionRetorno=(nuevoArgumento->pagina * tamanioPagina) + nuevoArgumento->offset;
			log_debug(ptrLog,"identificador_variable= %c, nuevoArgumento->pagina= %i, nuevoArgumento->offset= %i, nuevoArgumento->size= %i",identificador_variable ,nuevoArgumento->pagina, nuevoArgumento->offset, nuevoArgumento->size);
			return posicionRetorno;
		}
	}

}

t_valor_variable obtenerValorCompartida(t_nombre_compartida variable){

	header_t header;
	char *mensaje;
	variable = agregarCaracterAdmiracion(variable);

	t_op_varCompartida *varCompartida = malloc(sizeof(t_op_varCompartida));

	varCompartida->nombre=variable;
	varCompartida->valor=0;

	header.type=LEER_VAR_COMPARTIDA;
	char *serialized =serializer_opVarCompartida(varCompartida,&header.length);
	free(varCompartida);

	log_debug(ptrLog, "Obtener valor compartida '%s'", variable);

	int nbytes=sockets_send(socketKernel,&header,serialized);
	free(serialized);

	if(nbytes>0)
	{
		recv(socketKernel, &header, sizeof(header_t), MSG_WAITALL);

		if(header.type==ERROR)
		{
			log_error(ptrLog,"No se pudo obtener Valor de variable compartida");
			operacion=ERROR;
			return -1;
		}else{

			mensaje=malloc(header.length);
			recv(socketKernel, mensaje, header.length, MSG_WAITALL);
			t_op_varCompartida *varCompartida = deserializer_opVarCompartida(mensaje);
			log_debug(ptrLog, "Variable Compartida: '%s' Valor Obtenido: %d", variable, varCompartida->valor);
			free(mensaje);
			t_valor_variable valor = varCompartida->valor;
			free(varCompartida);
			return valor;
		}
	}
}

t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){

	header_t header;
	header.type=ASIGNAR_VAR_COMPARTIDA;

	t_op_varCompartida* varCompartida = malloc(sizeof(t_op_varCompartida));
	variable = agregarCaracterAdmiracion(variable);
	log_debug(ptrLog, "Asignar valor compartida. Variable compartida: '%s' - Valor: %d", variable, valor);
	varCompartida->nombre=malloc(strlen(variable)+1);
	strcpy(varCompartida->nombre,variable);
	varCompartida->valor = valor;

	char * serializer = serializer_opVarCompartida(varCompartida,&header.length);

	int nbytes=sockets_send(socketKernel,&header,serializer);

	if(nbytes>0)
	{
		recv(socketKernel, &header, sizeof(header_t), MSG_WAITALL);

		if(header.type==ERROR)
		{
			log_error(ptrLog,"No se pudo asignar Valor de variable compartida");
			operacion=ERROR;
			return -1;
		}else{

			log_debug(ptrLog,"Se asigno correctamente el valor de la variable compartida: %s", varCompartida->nombre);
			free(serializer);
			free(varCompartida->nombre);
			free(varCompartida);
			return valor;
		}
	}


}

void irAlLabel(t_nombre_etiqueta etiqueta){

	//devuelvo la primer instruccion ejecutable de etiqueta o -1 en caso de error
	//en vez de devolverla se agrega al program counter
	log_debug(ptrLog, "Ir al Label '%s'", etiqueta);
	t_puntero_instruccion numeroInstruccion = metadata_buscar_etiqueta(etiqueta, pcb->indice_etiquetas, pcb->tamanioEtiquetas);
	pcb->PC=numeroInstruccion - 1;

}

void llamarSinRetorno(t_nombre_etiqueta etiqueta){

	log_debug(ptrLog, "llamar sin Retorno. reservando espacio y cambiando al nuevo contexto de ejecucion");
	uint32_t tamlineaStack = 4*sizeof(uint32_t) + 2*sizeof(t_list);
	t_stack* nuevaLineaStackEjecucionActual;
	t_argumento* varRetorno = NULL;

	if(nuevaLineaStackEjecucionActual==NULL){
		//el tamanio de la linea del stack seria de los 4 ints mas
		nuevaLineaStackEjecucionActual = malloc(tamlineaStack);
		nuevaLineaStackEjecucionActual->argumentos = list_create();
		nuevaLineaStackEjecucionActual->variables = list_create();
		nuevaLineaStackEjecucionActual->retVar = varRetorno;
		nuevaLineaStackEjecucionActual->direccion_retorno = pcb->PC;
		list_add(pcb->indiceStack , nuevaLineaStackEjecucionActual);

		pcb->indiceContextoEjecucionActualStack++;
	}

	irAlLabel(etiqueta);

}

void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){

	log_debug(ptrLog, "llamar con Retorno. reservando espacio y cambiando al nuevo contexto de ejecucion");
	uint32_t tamlineaStack = 4*sizeof(uint32_t) + 2*sizeof(t_list);
	t_stack* nuevaLineaStackEjecucionActual=NULL;
	t_argumento* varRetorno = malloc(sizeof(t_argumento));
	varRetorno->pagina = (donde_retornar/tamanioPagina);
	varRetorno->offset = donde_retornar%tamanioPagina;
	varRetorno->size = TAMANIO_VARIABLE;

	if(nuevaLineaStackEjecucionActual==NULL){
		//el tamanio de la linea del stack seria de los 4 ints mas
		nuevaLineaStackEjecucionActual = malloc(tamlineaStack);
		nuevaLineaStackEjecucionActual->argumentos = list_create();
		nuevaLineaStackEjecucionActual->variables = list_create();
		nuevaLineaStackEjecucionActual->retVar = varRetorno;
		nuevaLineaStackEjecucionActual->direccion_retorno = pcb->PC;
		list_add(pcb->indiceStack , nuevaLineaStackEjecucionActual);

		pcb->indiceContextoEjecucionActualStack++;
	}

	irAlLabel(etiqueta);

}
// Esta instruccion la puedo resolver directamente
void finalizar(void){

	//Si el indice del contexto de ejecucion actual es 0, significa que estoy en la funcion principal
	if(pcb->indiceContextoEjecucionActualStack == 0){
		operacion=EXIT;
		//Revisar aca bien como finalizo por Exit el programa, en base al bucle de la CPU.
	}else{

		//agarro el contexto Actual y anterior
		int numeroContextoActual= pcb->indiceContextoEjecucionActualStack;
		t_stack* contextoEjecucionActual = list_get(pcb->indiceStack,numeroContextoActual);

		//Limpio el contexto actual
		int i;
		for( i=0 ; list_size(contextoEjecucionActual->argumentos) ; i++){
			t_argumento * arg = list_get(contextoEjecucionActual->argumentos, i);
			pcb->stackPointer = pcb->stackPointer -TAMANIO_VARIABLE;
			free(arg);
		}

		for( i=0 ; list_size(contextoEjecucionActual->variables) ; i++){
			t_variable * var = list_get(contextoEjecucionActual->variables, i);
			pcb->stackPointer = pcb->stackPointer -TAMANIO_VARIABLE;
			free(var);
		}
		free(contextoEjecucionActual);

		list_remove(pcb->indiceStack,numeroContextoActual);
		pcb->indiceContextoEjecucionActualStack--;

		t_stack *NuevoContextoEjecucion=list_get(pcb->indiceStack,pcb->indiceContextoEjecucionActualStack);

		pcb->PC=NuevoContextoEjecucion->direccion_retorno;
	}
}

void retornar(t_valor_variable retorno){

	//agarro el contexto Actual y anterior
	int numeroContextoActual= pcb->indiceContextoEjecucionActualStack;
	t_stack* contextoEjecucionActual = list_get(pcb->indiceStack,numeroContextoActual);

	//Limpio el contexto actual
	int i;
	for( i=0 ; i < list_size(contextoEjecucionActual->argumentos) ; i++){
		t_argumento * arg = list_get(contextoEjecucionActual->argumentos, i);
		pcb->stackPointer = pcb->stackPointer -TAMANIO_VARIABLE;
		free(arg);
	}

	for( i=0 ; i < list_size(contextoEjecucionActual->variables) ; i++){
		t_variable * var = list_get(contextoEjecucionActual->variables, i);
		pcb->stackPointer = pcb->stackPointer -TAMANIO_VARIABLE;
		free(var);
	}
	t_argumento* retVar = contextoEjecucionActual->retVar;
	t_puntero direcVariable = (retVar->pagina * tamanioPagina) + retVar->offset;
	//Calculo la direccion a la que tengo que retornar mediante la direccion de pagina start y offset que esta en el campo retvar
	asignar(direcVariable,retorno);
	//elimino el contexto actual del indice del stack
	//seteo el contexto de ejecucion actual en el anterior
	pcb->PC = contextoEjecucionActual->direccion_retorno;
	free(contextoEjecucionActual);
	list_remove(pcb->indiceStack,pcb->indiceContextoEjecucionActualStack);
	pcb->indiceContextoEjecucionActualStack=pcb->indiceContextoEjecucionActualStack-1;
	t_stack *NuevoContextoEjecucion=list_get(pcb->indiceStack,pcb->indiceContextoEjecucionActualStack);
	log_debug(ptrLog, "Llamada a retornar");

}

//Funciones Provilegiadas

void ansisop_escribir(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio)
{
	header_t header;
	header.type=ESCRIBIR_ARCHIVO;
	t_operacionDescriptorArchivo * escribirDescriptorArchivo=malloc(sizeof(t_operacionDescriptorArchivo));

	log_debug(ptrLog,"Print.. DESCRIPTOR_SALIDA: %d", descriptor_archivo);

	escribirDescriptorArchivo->descriptor=descriptor_archivo;
	escribirDescriptorArchivo->pid=pcb->PID;
	escribirDescriptorArchivo->tamanio=tamanio;
	escribirDescriptorArchivo->informacion= malloc(tamanio);
	memcpy(escribirDescriptorArchivo->informacion,informacion,tamanio);

	char *serialized=operacionDescriptorArchivo_serializer(escribirDescriptorArchivo,&header.length);

	free(escribirDescriptorArchivo->informacion);
	free(escribirDescriptorArchivo);

	int nbytes=sockets_send(socketKernel,&header,serialized);
	free(serialized);

	if(nbytes<=0)
	{
		operacion=ERROR;
	}else{

		recv(socketKernel, &header, sizeof(header_t), MSG_WAITALL);

		switch(header.type)
		{
			case SUCCESS:
				log_info(ptrLog,"Operacion resuelta correctamente");
				break;
			case ERROR_ESCRIBIR:
				log_error(ptrLog,"Error de escritura, se finalizara el proceso por ERROR_ESCRIBIR ");
				operacion=ERROR_ESCRIBIR;
				break;
			case ERROR:
				log_error(ptrLog,"Error de escritura, se finalizara el proceso por ERROR Generico");
				operacion=ERROR;
				break;
		}

	}
	return;
}


void ansisop_leer(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio)
{
	header_t header;
	header.type=LEER_ARCHIVO;
	t_operacionDescriptorArchivo * leerDescriptorArchivo=malloc(sizeof(t_operacionDescriptorArchivo));

	log_debug(ptrLog,"Solicitud de Lectura, Descriptor: %d", descriptor_archivo);

	leerDescriptorArchivo->descriptor=descriptor_archivo;
	leerDescriptorArchivo->pid=pcb->PID;
	leerDescriptorArchivo->tamanio=tamanio;
	leerDescriptorArchivo->informacion= malloc(sizeof(t_puntero));
	sprintf(leerDescriptorArchivo->informacion,"%d",informacion);

	char *serialized=operacionDescriptorArchivo_serializer(leerDescriptorArchivo,&header.length);

	free(leerDescriptorArchivo->informacion);
	free(leerDescriptorArchivo);

	int nbytes=sockets_send(socketKernel,&header,serialized);
	free(serialized);

	if(nbytes<=0)
	{
		operacion=ERROR;
	}else{

		recv(socketKernel, &header, sizeof(header_t), MSG_WAITALL);

		if(header.type==SUCCESS)
		{
			log_info(ptrLog,"Operacion de Lectura resuelta correctamente");
		}else{
			log_error(ptrLog,"Error de Lectura de Archivo");
			operacion=ERROR_LECTURA;
		}

	}
	return;
}

void wait(t_nombre_semaforo identificador_semaforo)
{

	header_t header;
	header.type=WAIT;
	header.length = strlen(identificador_semaforo) + 1;

	int nbytes=sockets_send(socketKernel,&header,identificador_semaforo);
	log_debug(ptrLog,"Wait. Semaforo: '%s' ", identificador_semaforo);
	if(nbytes<=0)
	{
		operacion=ERROR;
	}else{

		recv(socketKernel, &header, sizeof(header_t), MSG_WAITALL);

		switch(header.type)
		{
			case NOTHING:
				//operacion = header.type;
				log_debug(ptrLog,"Proceso no queda bloqueado por Semaforo '%s'", identificador_semaforo);
				break;
			case WAIT:
				operacion = header.type;
				log_debug(ptrLog,"Proceso bloqueado por Semaforo '%s'", identificador_semaforo);
				break;
			case ERROR:
				operacion=ERROR;
				log_error(ptrLog,"Operacion WAIT con semaforo Inexistente , se finalizara el proceso");
		}
	}
}

void ansisop_signal(t_nombre_semaforo identificador_semaforo)
{
	header_t header;
	header.type=SIGNAL;
	header.length = strlen(identificador_semaforo) + 1;

	log_debug(ptrLog,"Signal. Semaforo '%s'", identificador_semaforo);
	int nbytes = sockets_send(socketKernel,&header,identificador_semaforo);

	if(nbytes<=0)
	{
		operacion=ERROR;
	}else{

		recv(socketKernel, &header, sizeof(header_t), MSG_WAITALL);

		switch(header.type)
		{
			case SUCCESS:
				log_debug(ptrLog,"Operacion SIGNAL completada correctamente '%s'", identificador_semaforo);
				break;
			case ERROR:
				operacion=ERROR;
				log_error(ptrLog,"Operacion SIGNAL con semaforo Inexistente , se finalizara el proceso");
		}
	}
}

t_puntero reservar(t_valor_variable espacio)
{
	header_t header;
	header.type=ALOCAR;
	uint32_t *puntero;
	t_solicitudHeap*datosHeap=malloc(sizeof(t_solicitudHeap));

	datosHeap->pid=pcb->PID;
	datosHeap->puntero=espacio;

	log_debug(ptrLog,"Alocar. Pid=%d, bytes= %d", datosHeap->pid,espacio);

	char * serializer= solicitudHeap_serializer(datosHeap,&header.length);

	int nbytes = sockets_send(socketKernel,&header,serializer);
	free(serializer);
	free(datosHeap);

	if(nbytes<=0)
	{
		operacion=ERROR;
		return NULL;
	}else{

		recv(socketKernel, &header, sizeof(header_t), MSG_WAITALL);
		serializer=malloc(header.length);
		switch(header.type)
		{
			case SUCCESS:
				recv(socketKernel, serializer, header.length, MSG_WAITALL);
				puntero = uint32_deserializer(serializer);
				log_debug(ptrLog,"Se aloco correctamente, Puntero a heap devuelto por el Kernel es %d", *puntero);
				return (*puntero);
			case ALLOC_MAYOR_TAM_PAGINA:
				operacion=ALLOC_MAYOR_TAM_PAGINA;
				log_error(ptrLog,"no se pudo alocar el espacio solicitado en heap, por Superar el tamanio Maximo disponible");
				return NULL;
			case NO_PUDO_ASIGNAR_PAGINAS:
				operacion=NO_PUDO_ASIGNAR_PAGINAS;
				log_error(ptrLog,"no se pudo alocar el espacio solicitado en heap, no se pueden asignar mas paginas");
				return NULL;
			default:
				operacion=ERROR;
				log_error(ptrLog,"no se pudo alocar el espacio solicitado en heap, ERROR Generico protocolo de Kernel no identificado");
				return NULL;
		}
	}

}

void liberar(t_puntero puntero)
{

	header_t header;
	header.type=LIBERAR;
	t_solicitudHeap*datosHeap=malloc(sizeof(t_solicitudHeap));

	datosHeap->pid=pcb->PID;
	datosHeap->puntero=puntero;

	log_debug(ptrLog,"Liberar. Puntero %d, PID= %d", datosHeap->puntero, datosHeap->pid);

	char * serializer= solicitudHeap_serializer(datosHeap,&header.length);

	int nbytes = sockets_send(socketKernel,&header,serializer);
	free(serializer);
	free(datosHeap);

	if(nbytes<=0)
	{
		operacion=ERROR;
	}else{

		recv(socketKernel, &header, sizeof(header_t), MSG_WAITALL);
		serializer=malloc(header.length);
		switch(header.type)
		{
			case SUCCESS:
				log_debug(ptrLog,"Se libero correctamente, Puntero en heap ");
				return;
			case ERROR_HEAP:
				operacion=ERROR_HEAP;
				log_error(ptrLog,"no se pudo liberar el puntero solicitado en heap, se finalizara el proceso por ERROR_HEAP");
				return;
		}
	}
}

t_descriptor_archivo abrir(t_direccion_archivo direccion, t_banderas flags)
{
	header_t header;
	header.type=ABRIR_ARCHIVO;
	t_operacionDescriptorArchivo * abrirArchivo=malloc(sizeof(t_operacionDescriptorArchivo));
	uint32_t * puntero;
	char* serializer;

	log_debug(ptrLog,"Solicitud de Apertura de Archivo, Direccion: %s, flags.Creacion = %i, flags.escritura= %i, flags.lectura= %i", direccion, flags.creacion,flags.escritura, flags.lectura);

	abrirArchivo->descriptor=0;
	abrirArchivo->pid=pcb->PID;
	abrirArchivo->tamanio=strlen(direccion)+1;
	abrirArchivo->informacion= malloc(strlen(direccion)+1);
	abrirArchivo->informacion=direccion;
	abrirArchivo->flags.creacion=flags.creacion;
	abrirArchivo->flags.escritura=flags.escritura;
	abrirArchivo->flags.lectura=flags.lectura;

	char *serialized=operacionDescriptorArchivo_serializer(abrirArchivo,&header.length);

	free(abrirArchivo->informacion);
	free(abrirArchivo);

	int nbytes=sockets_send(socketKernel,&header,serialized);
	free(serialized);

	if(nbytes<=0)
	{
		operacion=ERROR;
	}else{

		recv(socketKernel, &header, sizeof(header_t), MSG_WAITALL);
		serializer = malloc(header.length);
		if(header.type==SUCCESS)
		{
			recv(socketKernel, serializer, header.length, MSG_WAITALL);
			puntero = uint32_deserializer(serializer);
			log_debug(ptrLog,"Se realizo correctamente la operacion de Apertura por parte del Kernel, Puntero devuelto por el Kernel es %d", *puntero);
			return (*puntero);
		}else{
			log_error(ptrLog,"Error de Apertura de Archivo");
			operacion=ERROR_APERTURA;
		}

	}
}

void borrar(t_descriptor_archivo descriptor_archivo)
{
	header_t header;
	header.type=BORRAR_ARCHIVO;
	t_operacionDescriptorArchivo * borrarArchivo=malloc(sizeof(t_operacionDescriptorArchivo));

	log_debug(ptrLog,"Solicitud de Borrado de Archivo, Descriptor: %d", descriptor_archivo);

	borrarArchivo->descriptor=descriptor_archivo;
	borrarArchivo->pid=pcb->PID;
	borrarArchivo->tamanio=0;

	char *serialized=operacionDescriptorArchivo_serializer(borrarArchivo,&header.length);

	free(borrarArchivo);

	int nbytes=sockets_send(socketKernel,&header,serialized);
	free(serialized);

	if(nbytes<=0)
	{
		operacion=ERROR;
	}else{

		recv(socketKernel, &header, sizeof(header_t), MSG_WAITALL);
		if(header.type==SUCCESS)
		{
			log_debug(ptrLog,"Se Borro correctamente el Archivo");
		}else{
			log_error(ptrLog,"Error en Borrado de Archivo, se finalizar el programa por error desconocido");
			operacion=ERROR;
		}
	}
}

void cerrar(t_descriptor_archivo descriptor_archivo)
{
	header_t header;
	header.type=CERRAR_ARCHIVO;
	t_operacionDescriptorArchivo * cerrarArchivo=malloc(sizeof(t_operacionDescriptorArchivo));

	log_debug(ptrLog,"Solicitud de Cerrado de Archivo, Descriptor: %d", descriptor_archivo);

	cerrarArchivo->descriptor=descriptor_archivo;
	cerrarArchivo->pid=pcb->PID;
	cerrarArchivo->tamanio=0;

	char *serialized=operacionDescriptorArchivo_serializer(cerrarArchivo,&header.length);

	free(cerrarArchivo);

	int nbytes=sockets_send(socketKernel,&header,serialized);
	free(serialized);

	if(nbytes<=0)
	{
		operacion=ERROR;
	}else{

		recv(socketKernel, &header, sizeof(header_t), MSG_WAITALL);
		if(header.type==SUCCESS)
		{
			log_debug(ptrLog,"Se Cerro correctamente el Archivo");
		}else{
			log_error(ptrLog,"Error en Cerrado de Archivo, se finalizar el programa por error desconocido");
			operacion=ERROR;
		}

	}
}

void moverCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion)
{
	header_t header;
	header.type=MOVER_CURSOR;
	t_operacionDescriptorArchivo * moverCursor=malloc(sizeof(t_operacionDescriptorArchivo));
	char* serializer;

	log_debug(ptrLog,"Solicitud de Mover Cursor, Descriptor: %d, posicion = %d", descriptor_archivo, posicion);

	//En tamanio se envia la posicion - Es para reutilizar la estructura
	moverCursor->descriptor=descriptor_archivo;
	moverCursor->pid=pcb->PID;
	moverCursor->tamanio=sizeof(t_valor_variable);
	moverCursor->informacion = malloc(sizeof(t_valor_variable));
	memcpy(moverCursor->informacion,&posicion,moverCursor->tamanio);

	char *serialized=operacionDescriptorArchivo_serializer(moverCursor,&header.length);

	free(moverCursor->informacion);
	free(moverCursor);

	int nbytes=sockets_send(socketKernel,&header,serialized);
	free(serialized);

	if(nbytes<=0)
	{
		operacion=ERROR;
	}else{

		recv(socketKernel, &header, sizeof(header_t), MSG_WAITALL);
		if(header.type==SUCCESS)
		{
			log_debug(ptrLog,"Se Movio Cursor correctamente");
		}else{
			log_error(ptrLog,"Error al Mover Cursor");
			operacion=ERROR;
		}
	}
}

char * agregarCaracterAdmiracion(char * variable)
{
	char *nombreConCaracter=malloc(strlen(variable) + 2);
	strcpy(nombreConCaracter,"!\0");
	strcat(nombreConCaracter, variable);
	return nombreConCaracter;
}
