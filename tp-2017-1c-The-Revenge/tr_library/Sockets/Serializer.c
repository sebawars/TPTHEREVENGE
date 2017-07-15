/*
 * serializer.c
 *
 *  Created on: 5/5/2017
 *      Author: utnso
 */
#include "Serializer.h"
#include <commons/collections/list.h>



 char * datosPrograma_serializer(t_datos_programa *datos, uint32_t *length) {

	uint32_t tamanio=strlen(datos->Script) + 1;
	//char *serialized = malloc(sizeof(char) + sizeof(uint32_t) + tamanio);
	char *serialized = malloc( sizeof(t_datos_programa) + tamanio);
	int offset = 0, tmp_size = 0;

	memcpy(serialized, &datos->PID, tmp_size = sizeof(uint32_t));
	offset += tmp_size;

	//Se guardan un uint32_t con el tamanio del dato enviado y el caracter Nulo
	memcpy(serialized + offset, &tamanio,tmp_size = sizeof(uint32_t));
	offset += tmp_size;

	memcpy(serialized + offset, datos->Script,tmp_size = tamanio);
	offset += tmp_size;

	*length = offset;

	return serialized;
}


t_datos_programa * datosPrograma_deserializer(char *serialized)
{
	t_datos_programa *datos = malloc(sizeof(t_datos_programa));
	int offset = 0, tmp_size = 0;

	memcpy( &datos->PID, serialized ,tmp_size = sizeof(uint32_t));
	offset += tmp_size;

	uint32_t tamanio;
	memcpy( &tamanio, serialized + offset ,tmp_size = sizeof(uint32_t));
	offset += tmp_size;

	datos->Script = malloc(tamanio);
	memcpy(datos->Script, serialized + offset, tamanio);

	return datos;
}


char * uint32_serializer(uint32_t * self,uint32_t* length){
	char *serialized=malloc(sizeof(uint32_t));
	int offset=0, tmp_size=0;

	memcpy(serialized,self,tmp_size=sizeof(uint32_t));
	offset=tmp_size;

	*length=offset;

	return serialized;
}

uint32_t *uint32_deserializer(char*serialized){
	uint32_t *self = malloc(sizeof(uint32_t));

	memcpy(self,serialized,sizeof(uint32_t));

	return self;
}

//Serializacion de PCB, revisar si hay que agregar o eliminar nuevos campos
char * serializer_pcb(t_PCB * self, uint32_t *length)
{
	uint32_t lengthIndCodigo,lengthIndStack;
	char * indiceCodigo = serializer_indiceCodigo(self->IC,&lengthIndCodigo);
	char * stackSerializado = serializer_indiceStack(self->indiceStack,&lengthIndStack);

	uint32_t tamanioPCB=0;
	tamanioPCB +=lengthIndCodigo;
	tamanioPCB +=lengthIndStack;
	tamanioPCB +=sizeof(uint32_t); //Tamanio Etiquetas

	if(self->indice_etiquetas != NULL && self->tamanioEtiquetas>0)
		tamanioPCB += self->tamanioEtiquetas;

	tamanioPCB += (sizeof(uint32_t) * 14); //Cantidad de uint32_t que tiene el PCB
	tamanioPCB += sizeof(int32_t); //ExitCode requiere valores negativos
	tamanioPCB += sizeof(uint32_t); // Para indidcar tamanio de PCBS

	//Comienza Serializacion
	char *serialized=malloc(tamanioPCB);
	uint32_t offset=0, tmp_size=0;

	//Serializo tamanio de PCB
	memcpy(serialized,&tamanioPCB,tmp_size=sizeof(uint32_t));
	offset=tmp_size;

	//Serializo 13 uint32_t + int32_t, en caso de agregar mas actualizar
	memcpy(serialized + offset,&self->EC,tmp_size=sizeof(int32_t));
	offset+=tmp_size;
	memcpy(serialized + offset,&self->PC,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;
	memcpy(serialized + offset,&self->PID,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;
	memcpy(serialized + offset,&self->estado,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;
	memcpy(serialized + offset,&self->indiceContextoEjecucionActualStack,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;
	memcpy(serialized + offset,&self->paginaCodigoActual,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;
	memcpy(serialized + offset,&self->paginaStackActual,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;
	memcpy(serialized + offset,&self->primerPaginaStack,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;
	memcpy(serialized + offset,&self->quantum,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;
	memcpy(serialized + offset,&self->quantum_sleep,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;
	memcpy(serialized + offset,&self->rafagas,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;
	memcpy(serialized + offset,&self->socket,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;
	memcpy(serialized + offset,&self->stackPointer,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;
	memcpy(serialized + offset,&self->tamanioEtiquetas,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;

	//Serializo Indice de Codigo
	memcpy(serialized + offset, indiceCodigo,lengthIndCodigo);
	offset+=lengthIndCodigo;

	//Serializo indice Stack
	memcpy(serialized + offset, stackSerializado,lengthIndStack);
	offset+=lengthIndStack;

	//Serializo Indice de Etiquetas
	if(self->indice_etiquetas != NULL && self->tamanioEtiquetas > 0){
		memcpy(serialized + offset, &self->tamanioEtiquetas,tmp_size=sizeof(uint32_t));
		offset+=tmp_size;
		memcpy(serialized + offset, self->indice_etiquetas,self->tamanioEtiquetas);
	}else{
		uint32_t longitudIndiceEtiquetas = 0;
		memcpy(serialized + offset, &longitudIndiceEtiquetas, tmp_size=sizeof(uint32_t));
	}

	free(indiceCodigo);
	free(stackSerializado);

	*length=tamanioPCB;

	return serialized;
}

t_PCB * deserializer_pcb(char *serialized)
{
	int offset=0,tmp_size=0;
	uint32_t tamanioPCB;

	//Obtengo el tamanio del PCB
	memcpy(&tamanioPCB,serialized,tmp_size=sizeof(uint32_t));
	offset=tmp_size;

	t_PCB *self = malloc(tamanioPCB);

	//Obtengo los 14 uint32_t + 1 int32_t, ajustar en caso de agregar nuevo dato
	memcpy(&self->EC,serialized + offset,tmp_size=sizeof(int32_t));
	offset+=tmp_size;
	memcpy(&self->PC,serialized + offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;
	memcpy(&self->PID,serialized + offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;
	memcpy(&self->estado,serialized + offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;
	memcpy(&self->indiceContextoEjecucionActualStack,serialized + offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;
	memcpy(&self->paginaCodigoActual,serialized + offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;
	memcpy(&self->paginaStackActual,serialized + offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;
	memcpy(&self->primerPaginaStack,serialized + offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;
	memcpy(&self->quantum,serialized + offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;
	memcpy(&self->quantum_sleep,serialized + offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;
	memcpy(&self->rafagas,serialized + offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;
	memcpy(&self->socket,serialized + offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;
	memcpy(&self->stackPointer,serialized + offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;
	memcpy(&self->tamanioEtiquetas,serialized + offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;

	//Tomo Inidice de Codigo
	uint32_t itemsIndiceDeCodigo;
	memcpy(&itemsIndiceDeCodigo,serialized + offset,tmp_size=sizeof(uint32_t));
	//offset+=tmp_size;

	//Se suman los 4bytes con el tamanio de la lista incluida para poder deserealizar
	uint32_t tamanioIndiceDeCodigo = (sizeof(t_indice_codigo) * itemsIndiceDeCodigo) + sizeof(uint32_t);
	//Incluyo los 4 bytes de tamanio + toda la estructura Serializada
	char * bufferIndiceDeCodigo = malloc(tamanioIndiceDeCodigo);
	memcpy(bufferIndiceDeCodigo,serialized + offset,tamanioIndiceDeCodigo);
	offset+=tamanioIndiceDeCodigo;

 	t_list * indiceDeCodigo = deserializer_indiceCodigo(bufferIndiceDeCodigo);
 	self->IC= indiceDeCodigo;

 	//Tomo Indice de Stack
 	uint32_t tamanioStack;
 	memcpy(&tamanioStack, serialized + offset,tmp_size=sizeof(uint32_t));
 	offset += tmp_size;

 	char * bufferIndiceStack = malloc(tamanioStack);
 	memcpy(bufferIndiceStack, serialized + offset, tamanioStack);
 	offset += tamanioStack;

 	t_list * indiceStack = deserializer_indiceStack(bufferIndiceStack);
 	self->indiceStack = indiceStack;

 	//Tomo Indice de Etiquetas
 	uint32_t longitudIndiceEtiquetas;
 	memcpy(&longitudIndiceEtiquetas, serialized + offset, tmp_size=sizeof(uint32_t));
 	offset+= tmp_size;
 	if(longitudIndiceEtiquetas>0) {
 		char * indiceEtiquetas = malloc(self->tamanioEtiquetas);
 		memcpy(indiceEtiquetas, serialized + offset, longitudIndiceEtiquetas);
 		self->indice_etiquetas = indiceEtiquetas;
 	}else{
 		self->indice_etiquetas = NULL;
 	}

	free(bufferIndiceDeCodigo);
 	free(bufferIndiceStack);

	return self;
}

char * solicitudLectura_serializer(t_solicitudLectura *self,uint32_t *length){

	char *serialized=malloc(sizeof(t_solicitudLectura));
	int offset=0, tmp_size=0;

	memcpy(serialized,&self->inicio,tmp_size=sizeof(uint32_t));
	offset=tmp_size;

	memcpy(serialized + offset, &self->offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;

	memcpy(serialized + offset,&self->pagina,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;

	memcpy(serialized + offset,&self->pid,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;

	*length=offset;

	return serialized;
}

t_solicitudLectura * solicitudLectura_deserializer(char *serialized){

	t_solicitudLectura *self = malloc(sizeof(t_solicitudLectura));
	int offset=0,tmp_size=0;

	memcpy(&self->inicio,serialized,tmp_size=sizeof(uint32_t));
	offset=tmp_size;

	memcpy(&self->offset,serialized + offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;

	memcpy(&self->pagina,serialized + offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;

	memcpy(&self->pid,serialized + offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;

	return self;
}

char * solicitudEscritura_serializer(t_solicitudEscritura *self,uint32_t *length){

	//char *serialized=malloc((sizeof(uint32_t) * 5) + strlen(self->buffer) + 1);
	char *serialized=malloc(sizeof(t_solicitudEscritura) + self->tamanio);
	int offset=0, tmp_size=0;

	memcpy(serialized,&self->pagina,tmp_size=sizeof(uint32_t));
	offset=tmp_size;

	memcpy(serialized + offset, &self->offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;

	memcpy(serialized + offset,&self->tamanio,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;

	memcpy(serialized + offset,&self->pid,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;

	//Envio 4bytes con el largo del buffer
	//uint32_t tamanioBuffer=strlen(self->buffer) + 1;
	//memcpy(serialized + offset,&tamanioBuffer,tmp_size=sizeof(uint32_t));
	//offset+=tmp_size;

	memcpy(serialized + offset,self->buffer,tmp_size=self->tamanio);
	offset+=tmp_size;

	*length=offset;

	return serialized;
}

t_solicitudEscritura * solicitudEscritura_deserializer(char *serialized){

	t_solicitudEscritura *self = malloc(sizeof(t_solicitudEscritura));
	int offset=0,tmp_size=0;

	memcpy(&self->pagina,serialized,tmp_size=sizeof(uint32_t));
	offset=tmp_size;

	memcpy(&self->offset,serialized + offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;

	memcpy(&self->tamanio,serialized + offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;

	memcpy(&self->pid,serialized + offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;

	//uint32_t tamanioBuffer;
	//memcpy(&tamanioBuffer,serialized + offset,tmp_size=sizeof(uint32_t));
	//offset+=tmp_size;

	self->buffer = malloc(self->tamanio);
	memcpy(self->buffer, serialized+offset, tmp_size=self->tamanio);

	return self;
}

char * bytesLeidos_serializer(char* data,uint32_t *length){

	char *serialized=malloc(strlen(data) + 1);
	int offset=0, tmp_size=0;

	memcpy(serialized + offset,data,tmp_size=strlen(data) + 1);
	offset+=tmp_size;

	*length=offset;

	return serialized;
}

char * bytesLeidos_deserializer(char *serialized){

	int offset=0,tmp_size=0;

	for (tmp_size = 1; serialized[tmp_size - 1] != '\0'; tmp_size++)
		;
	char* data = malloc(tmp_size);
	memcpy(data, serialized+offset, tmp_size);

	return data;
}

char * solicitudHeap_serializer(t_solicitudHeap *self, uint32_t * length) {
	int offset = 0, tmp_size = sizeof(uint32_t);
	char * serialized = malloc(sizeof(t_solicitudHeap));

	memcpy(serialized + offset, &self->pid, tmp_size);
	offset += tmp_size;

	memcpy(serialized + offset, &self->puntero, tmp_size);
	offset += tmp_size;

	*length = offset;

	return serialized;
}

t_solicitudHeap *solicitudHeap_deserializer(char * serialized)
{
	int offset = 0, tmp_size = sizeof(uint32_t);
	t_solicitudHeap *self=malloc(sizeof(t_solicitudHeap));

	memcpy(&self->pid, serialized + offset, tmp_size);
	offset += tmp_size;

	memcpy(&self->puntero, serialized + offset, tmp_size);

	return self;

}

char* solicitudPaginas_serializer(t_solicitudPaginas* solicitud,uint32_t * length)
{
	char *serialized=malloc(sizeof(uint32_t) * 2);

	int offset=0, tmp_size=0;

	memcpy(serialized,&solicitud->pid,tmp_size=sizeof(uint32_t));
	offset=tmp_size;

	memcpy(serialized + offset, &solicitud->cantidadPaginas,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;

	*length=offset;

	return serialized;

}

t_solicitudPaginas* solicitudPaginas_deserializer(char *serialized){

	t_solicitudPaginas* self = malloc(sizeof(t_solicitudPaginas));
	int offset=0,tmp_size=0;

	memcpy(&self->pid,serialized,tmp_size=sizeof(uint32_t));
	offset=tmp_size;

	memcpy(&self->cantidadPaginas,serialized + offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;

	return self;
}

char * serializer_opVarCompartida (t_op_varCompartida * self , uint32_t * length){

	char *serialized=malloc((sizeof(uint32_t) * 2) + strlen(self->nombre) +1);

	int offset=0, tmp_size=0;

	uint32_t longNombre=strlen(self->nombre)+1;
	//Se utilizan 4 bytes para guardar la longitud del Nombre
	memcpy(serialized,&longNombre,tmp_size=sizeof(uint32_t));
	offset=tmp_size;

	memcpy(serialized + offset, self->nombre,tmp_size=longNombre);
	offset+=tmp_size;

	memcpy(serialized + offset, &self->valor,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;

	*length=offset;

	return serialized;
}

t_op_varCompartida* deserializer_opVarCompartida(char *serialized){

	t_op_varCompartida* self = malloc(sizeof(t_op_varCompartida));
	int offset=0,tmp_size=0;

	uint32_t longNombre;
	memcpy(&longNombre,serialized,tmp_size=sizeof(uint32_t));
	offset=tmp_size;

	self->nombre=malloc(longNombre);
	memcpy(self->nombre,serialized + offset,tmp_size=longNombre);
	offset+=tmp_size;

	memcpy(&self->valor,serialized + offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;

	return self;
}

char *serializer_indiceCodigo(t_list* self, uint32_t * length){

	char *serialized=malloc((list_size(self) * (2 * sizeof(uint32_t))) + sizeof(uint32_t)) ;
	uint32_t itemsEnLista = list_size(self);
	int offset=0, tmp_size=0,i;

	//Guardo 4bytes con la cantidad de elementos que tiene la lista
	memcpy(serialized,&itemsEnLista,tmp_size=sizeof(uint32_t));
	offset=tmp_size;

	//Guardo cada elemento de la lista
	t_indice_codigo* linea;
	for(i=0;i < itemsEnLista; i++){
		linea=list_get(self, i);
		memcpy(serialized + offset, &linea->inicio,tmp_size=sizeof(uint32_t));
		offset+=tmp_size;
		memcpy(serialized + offset, &linea->longitud,tmp_size=sizeof(uint32_t));
		offset+=tmp_size;
	}

	*length=offset;

	return serialized;
}

t_list *deserializer_indiceCodigo(char * serialized){

	t_list* self = list_create();

	int offset=0,tmp_size=0,i;

	//Obtengo la cantidad de elementos con los 4bytes
	uint32_t cantidadElementos;
	memcpy(&cantidadElementos,serialized,tmp_size=sizeof(uint32_t));
	offset=tmp_size;

	//Recorro el dato serializado, voy obteniendo la linea y agregandola a la lista
	for (i=0;i < cantidadElementos ; i++)
	{
		t_indice_codigo* linea = malloc(sizeof(t_indice_codigo));
		memcpy(&linea->inicio,serialized + offset,tmp_size=sizeof(uint32_t));
		offset+=tmp_size;
		memcpy(&linea->longitud,serialized + offset,tmp_size=sizeof(uint32_t));
		offset+=tmp_size;
		list_add(self,linea);
	}

	return self;
}

char *serializer_indiceStack(t_list* self, uint32_t * length){

	int i;
	uint32_t tamanioTotalBuffer= sizeof(uint32_t);

	t_list * tamanioStackStack = list_create();
	uint32_t tamanioStackParticular;
	for(i=0; i < list_size(self);i++){
		t_stack * linea = list_get(self,i);
		tamanioStackParticular=0;

		int cantidadArgumentos=list_size(linea->argumentos);
		tamanioTotalBuffer += cantidadArgumentos * sizeof(t_argumento); //Tamanio de la lista de argumentos
		tamanioStackParticular += cantidadArgumentos * sizeof(t_argumento);

		int cantidadVariables = list_size(linea->variables);
		tamanioTotalBuffer += cantidadVariables * sizeof(t_variable); //Tamanio de la lista de variables
		tamanioStackParticular += cantidadVariables * sizeof(t_variable);

		tamanioTotalBuffer += sizeof(uint32_t); //Tamanio de variable de direcretorno
		tamanioStackParticular += sizeof(uint32_t);

		tamanioTotalBuffer += sizeof(t_argumento); //Tamanio de retVar
		tamanioStackParticular += sizeof(t_argumento);

		tamanioTotalBuffer += sizeof(uint32_t) * 3; //agrego 3 ints para indicar la cantidad de elemento de las 2 listas y los bytes de t_stack

		t_tamanio_stack_stack * auxiliar = malloc(sizeof(uint32_t) + tamanioStackParticular);
		auxiliar->stack = linea;
		auxiliar->tamanioStack = tamanioStackParticular;
		list_add(tamanioStackStack, auxiliar);
	}

	char * serialized = malloc(tamanioTotalBuffer + sizeof(uint32_t));
	int tmp_size=0,offset=0;

	memcpy(serialized,&tamanioTotalBuffer,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;

	uint32_t cantidadItemEnStack = list_size(self);

	memcpy(serialized + offset,&cantidadItemEnStack,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;

	//Guardo cada elemento/lista del stack

	for(i=0;i < list_size(tamanioStackStack); i++){
		t_tamanio_stack_stack * linea = list_get(tamanioStackStack,i);

		memcpy(serialized + offset, &linea->tamanioStack,tmp_size=sizeof(uint32_t));
		offset+=tmp_size;

		t_stack * stack=linea->stack;

		uint32_t cantidadArgumentos= list_size(stack->argumentos);
		memcpy(serialized + offset, &cantidadArgumentos,tmp_size=sizeof(uint32_t));
		offset+=tmp_size;

		int j;
		for(j = 0; j < cantidadArgumentos; j++){
			t_argumento * argumento = list_get(stack->argumentos,j);
			memcpy(serialized + offset, &argumento->pagina,tmp_size=sizeof(uint32_t));
			offset+=tmp_size;
			memcpy(serialized + offset, &argumento->offset,tmp_size=sizeof(uint32_t));
			offset+=tmp_size;
			memcpy(serialized + offset, &argumento->size,tmp_size=sizeof(uint32_t));
			offset+=tmp_size;
			free(argumento);
		}

		uint32_t cantidadVariables = list_size(stack->variables);
		memcpy(serialized + offset, &cantidadVariables,tmp_size=sizeof(uint32_t));
		offset+=tmp_size;

		for(j = 0; j < cantidadVariables; j++){
			t_variable * variable = list_get(stack->variables,j);
			memcpy(serialized + offset, &variable->idVariable,tmp_size=sizeof(char));
			offset+=tmp_size;
			memcpy(serialized + offset, &variable->pagina,tmp_size=sizeof(uint32_t));
			offset+=tmp_size;
			memcpy(serialized + offset, &variable->offset,tmp_size=sizeof(uint32_t));
			offset+=tmp_size;
			memcpy(serialized + offset, &variable->size,tmp_size=sizeof(uint32_t));
			offset+=tmp_size;
			free(variable);
		}

		memcpy(serialized + offset, &stack->direccion_retorno,tmp_size=sizeof(uint32_t));
		offset+=tmp_size;

		t_argumento * retVarStack = stack->retVar;
		if(retVarStack == NULL){
			retVarStack = malloc(sizeof(t_argumento));
			retVarStack->offset = 0;
			retVarStack->pagina = 0;
			retVarStack->size = 0;
		}
		memcpy(serialized + offset, &retVarStack->pagina,tmp_size=sizeof(uint32_t));
		offset+=tmp_size;
		memcpy(serialized + offset, &retVarStack->offset,tmp_size=sizeof(uint32_t));
		offset+=tmp_size;
		memcpy(serialized + offset, &retVarStack->size,tmp_size=sizeof(uint32_t));
		offset+=tmp_size;
		free(retVarStack);
	}

	for(i=0;i<list_size(tamanioStackStack);i++){
		t_tamanio_stack_stack * tamanioStack= list_get(tamanioStackStack,i);
		t_stack * stack = tamanioStack->stack;
		list_remove(tamanioStackStack,i);
		i--;
		free(stack);
		free(tamanioStack);
	}
	free(tamanioStackStack);

	*length=tamanioTotalBuffer + sizeof(uint32_t);

	return serialized;
}

t_list *deserializer_indiceStack(char * serialized){

	t_list * stack = list_create();
	int offset = 0, tmp_size=0;

	uint32_t cantidadElementosEnStack;
	memcpy(&cantidadElementosEnStack,serialized,tmp_size=sizeof(uint32_t));
	offset=tmp_size;

	int i;
	for(i=0;i<cantidadElementosEnStack;i++){
		uint32_t tamanioItemStack;
		memcpy(&tamanioItemStack,serialized + offset,tmp_size=sizeof(uint32_t));
		offset+=tmp_size;

		t_stack * stack_item = malloc(tamanioItemStack);

		t_list * argumentosStack = list_create();
		uint32_t cantidadArgumentosStack;
		memcpy(&cantidadArgumentosStack,serialized + offset,tmp_size=sizeof(uint32_t));
		offset+=tmp_size;
		int j;
		for(j=0;j<cantidadArgumentosStack;j++){
			t_argumento *argStack = malloc(sizeof(t_argumento));
			memcpy(&argStack->pagina,serialized + offset,tmp_size=sizeof(uint32_t));
			offset+=tmp_size;
			memcpy(&argStack->offset,serialized + offset,tmp_size=sizeof(uint32_t));
			offset+=tmp_size;
			memcpy(&argStack->size,serialized + offset,tmp_size=sizeof(uint32_t));
			offset+=tmp_size;
			list_add(argumentosStack, argStack);
		}
		stack_item->argumentos = argumentosStack;

		t_list * variablesStack = list_create();
		uint32_t cantidadVariablesStack;
		memcpy(&cantidadVariablesStack,serialized + offset,tmp_size=sizeof(uint32_t));
		offset+=tmp_size;
		for(j=0;j<cantidadVariablesStack;j++){
			t_variable *varStack = malloc(sizeof(t_variable));
			memcpy(&varStack->idVariable,serialized + offset,tmp_size=sizeof(char));
			offset+=tmp_size;
			memcpy(&varStack->pagina,serialized + offset,tmp_size=sizeof(uint32_t));
			offset+=tmp_size;
			memcpy(&varStack->offset,serialized + offset,tmp_size=sizeof(uint32_t));
			offset+=tmp_size;
			memcpy(&varStack->size,serialized + offset,tmp_size=sizeof(uint32_t));
			offset+=tmp_size;
			list_add(variablesStack, varStack);
		}
		stack_item->variables = variablesStack;

		uint32_t direcRetorno;
		memcpy(&direcRetorno,serialized + offset,tmp_size=sizeof(uint32_t));
		stack_item->direccion_retorno = direcRetorno;
		offset+=tmp_size;

		t_argumento * retVarStack = malloc(sizeof(t_argumento));
		memcpy(&retVarStack->pagina,serialized + offset,tmp_size=sizeof(uint32_t));
		offset+=tmp_size;
		memcpy(&retVarStack->offset,serialized + offset,tmp_size=sizeof(uint32_t));
		offset+=tmp_size;
		memcpy(&retVarStack->size,serialized + offset,tmp_size=sizeof(uint32_t));
		offset+=tmp_size;
		if(retVarStack->pagina == 0 && retVarStack->offset == 0 && retVarStack->size == 0){
			retVarStack=NULL;
		}
		stack_item->retVar = retVarStack;

		list_add(stack,stack_item);
	}

	return stack;
}

char *operacionDescriptorArchivo_serializer(t_operacionDescriptorArchivo* self, uint32_t * length){

	char *serialized = malloc((sizeof(uint32_t) * 3)  + sizeof(t_flags) + sizeof(char) + self->tamanio);
	int offset = 0, tmp_size = 0;

	memcpy(serialized, &self->pid, tmp_size = sizeof(uint32_t));
	offset += tmp_size;

	memcpy(serialized + offset, &self->descriptor, tmp_size = sizeof(uint32_t));
	offset += tmp_size;

	memcpy(serialized + offset, &self->tamanio, tmp_size = sizeof(uint32_t));
	offset += tmp_size;

	memcpy(serialized + offset, self->informacion,tmp_size = self->tamanio);
	offset += tmp_size;

	memcpy(serialized + offset, &self->flags.creacion,tmp_size = sizeof(bool));
	offset += tmp_size;

	memcpy(serialized + offset, &self->flags.escritura,tmp_size = sizeof(bool));
	offset += tmp_size;

	memcpy(serialized + offset, &self->flags.lectura,tmp_size = sizeof(bool));
	offset += tmp_size;

	*length = offset;

	return serialized;

}

t_operacionDescriptorArchivo* operacionDescriptorArchivo_deserializer (char* serialized){

	t_operacionDescriptorArchivo *self = malloc(sizeof(t_operacionDescriptorArchivo));
	int offset=0,tmp_size=0;

	memcpy(&self->pid,serialized,tmp_size=sizeof(uint32_t));
	offset=tmp_size;

	memcpy(&self->descriptor,serialized + offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;

	memcpy(&self->tamanio,serialized + offset,tmp_size=sizeof(uint32_t));
	offset+=tmp_size;

	self->informacion=malloc(self->tamanio);
	memcpy(self->informacion,serialized + offset,tmp_size=self->tamanio);
	offset+=tmp_size;

	memcpy(&self->flags.creacion,serialized + offset,tmp_size=sizeof(bool));
	offset+=tmp_size;

	memcpy(&self->flags.escritura,serialized + offset,tmp_size=sizeof(bool));
	offset+=tmp_size;

	memcpy(&self->flags.lectura,serialized + offset,tmp_size=sizeof(bool));
	offset+=tmp_size;

	return self;
}

char* guardarDatosSerializer(t_guardarDatos* operacion, uint32_t *length) {
	int tamSerializer = sizeof(int8_t) + 2 * sizeof(int16_t)
			+ 2 * sizeof(int32_t) + strlen(operacion->buffer)
			+ strlen(operacion->path) + 1;
	char* serializado = malloc(tamSerializer);
	int offset = 0, tamAux = 0;
	//offset+=sprintf(serializado,"%d",operacion->operacion);
	//offset+=sprintf(serializado+offset,"%d",operacion->offset);
	//offset+=sprintf(serializado+offset,"%d",operacion->size);
	memcpy(serializado + offset, &operacion->tamPath, tamAux = sizeof(int32_t));
	offset += tamAux;
	memcpy(serializado + offset, operacion->path,
			tamAux = strlen(operacion->path) + 1);
	offset += tamAux;
	memcpy(serializado + offset, &operacion->offset, tamAux = sizeof(int16_t));
	offset += tamAux;
	memcpy(serializado + offset, &operacion->size, tamAux = sizeof(int16_t));
	offset += tamAux;
	memcpy(serializado + offset, &operacion->tamBuffer, tamAux =
			sizeof(int32_t));
	offset += tamAux;
	memcpy(serializado + offset, operacion->buffer,
			tamAux = strlen(operacion->buffer) + 1);
	offset += tamAux;
	*length = offset;
	return serializado;

}

t_guardarDatos* guardarDatosDesSerializer(char* serializado) {
	t_guardarDatos* operacionKernel = malloc(sizeof(t_guardarDatos));
	int offset = 0, tamAux = 0, len;

	//memcpy(&operacionKernel->operacion,serializado+offset,tamAux=sizeof(int8_t));
	//offset+=tamAux;
	memcpy(&operacionKernel->tamPath, serializado + offset, tamAux =
			sizeof(int32_t));
	offset += tamAux;
	//for (len = offset; serializado[len - 1] != '\0'; len++);
	operacionKernel->path = malloc(operacionKernel->tamPath + 1);
	memcpy(operacionKernel->path, serializado + offset,
			tamAux = operacionKernel->tamPath);
	operacionKernel->path[tamAux] = '\0';
	offset += tamAux + 1;
	memcpy(&operacionKernel->offset, serializado + offset, tamAux =
			sizeof(int16_t));
	offset += tamAux;
	memcpy(&operacionKernel->size, serializado + offset, tamAux =
			sizeof(int16_t));
	offset += tamAux;
	memcpy(&operacionKernel->tamBuffer, serializado + offset, tamAux =
			sizeof(int32_t));
	offset += tamAux;
	//for (len = offset; serializado[len - 1] != '\0'; len++);
	operacionKernel->buffer = malloc(operacionKernel->tamBuffer + 1);
	memcpy(operacionKernel->buffer, serializado + offset, tamAux =
			operacionKernel->tamBuffer);
	operacionKernel->buffer[tamAux] = '\0';

	return operacionKernel;

}


char* obtenerDatosSerializer(t_obtenerDatos* operacion, uint32_t* length) {
	int tamSerializer = sizeof(int8_t) + 2 * sizeof(int16_t)
			+ 2 * sizeof(int32_t) + strlen(operacion->path) + 1;
	char* serializado = malloc(tamSerializer);
	int offset = 0, tamAux = 0;
	//offset+=sprintf(serializado,"%d",operacion->operacion);
	//offset+=sprintf(serializado+offset,"%d",operacion->offset);
	//offset+=sprintf(serializado+offset,"%d",operacion->size);
	memcpy(serializado + offset, &operacion->tamPath, tamAux = sizeof(int32_t));
	offset += tamAux;
	memcpy(serializado + offset, operacion->path,
			tamAux = strlen(operacion->path) + 1);
	offset += tamAux;
	memcpy(serializado + offset, &operacion->offset, tamAux = sizeof(int16_t));
	offset += tamAux;
	memcpy(serializado + offset, &operacion->size, tamAux = sizeof(int16_t));
	offset += tamAux;
	*length = offset;
	return serializado;
}


t_obtenerDatos* obtenerDatosDesSerializer(char* serializado) {
	t_obtenerDatos* operacionKernel = malloc(sizeof(t_obtenerDatos));
	int offset = 0, tamAux = 0, len;

	//memcpy(&operacionKernel->operacion,serializado+offset,tamAux=sizeof(int8_t));
	//offset+=tamAux;
	memcpy(&operacionKernel->tamPath, serializado + offset, tamAux =
			sizeof(int32_t));
	offset += tamAux;
	//for (len = offset; serializado[len - 1] != '\0'; len++);
	operacionKernel->path = malloc(operacionKernel->tamPath + 1);
	memcpy(operacionKernel->path, serializado + offset,
			tamAux = operacionKernel->tamPath);
	operacionKernel->path[tamAux] = '\0';
	offset += tamAux + 1;
	memcpy(&operacionKernel->offset, serializado + offset, tamAux =
			sizeof(int16_t));
	offset += tamAux;
	memcpy(&operacionKernel->size, serializado + offset, tamAux =
			sizeof(int16_t));
	offset += tamAux;

	return operacionKernel;
}


char* validarArchivoSerializer(t_validarArchivo* operacion, uint32_t* length) {
	int tamSerializer = sizeof(int8_t) + strlen(operacion->path)+1;
	char* serializado = malloc(tamSerializer);
	int tamAux = 0;
	int offset=0;
	memcpy(serializado+offset,&operacion->resultado,tamAux=sizeof(uint8_t));
	offset+=tamAux;
	memcpy(serializado+offset, operacion->path, tamAux = strlen(operacion->path) + 1);
	offset += tamAux;
	*length=offset;
	return serializado;
}


t_validarArchivo* validarArchivoDesSerializer(char* serializado) {
	t_validarArchivo* operacionKernel = malloc(sizeof(t_guardarDatos));
	int tamAux = 0,offset=0;
	operacionKernel->path = malloc(strlen(serializado) + 1 - sizeof(uint8_t));
	memcpy(&operacionKernel->resultado,serializado,tamAux=sizeof(uint8_t));
	offset=tamAux;
	memcpy(operacionKernel->path, serializado+offset, tamAux = strlen(serializado)-sizeof(uint8_t));
	operacionKernel->path[tamAux] = '\0';


	return operacionKernel;

}
