/*
 * capaFileSystem.c
 *
 *  Created on: 23/6/2017
 *      Author: utnso
 */


#include "capaFileSystem.h"


///operaciones de archivos
int abrirArchivo(uint32_t pid,char* path, t_flags permisos){
	_Bool _tieneElPid(t_PCB *prog) {
		return (prog->PID == pid);
	}

	log_debug(logFile,"abriendo archivo %s desde el proceso con pid %d",path,pid);
	uint32_t globalFD;
	int fd;
	if(validarArchivo(path))
	{
		sem_wait(&mutex_GFT);
		globalFD=agregarEntradaGFT(path);
		sem_post(&mutex_GFT);
		sem_wait(&mutex_PFT);
		fd=nuevoFD(pid);
		crearEntradaEnPFT(pid,permisos,fd,globalFD);
		sem_post(&mutex_PFT);
		t_cantSyscalls* aux = list_find(tablaSyscalls, (void*)_tieneElPid);
		aux->cantAbrir++;
		return fd;
	}else{

		if(permisos.creacion){
			if(crearArhivoFS(path)<0)
				return -1;
			sem_wait(&mutex_GFT);
			globalFD=agregarEntradaGFT(path);
			sem_post(&mutex_GFT);
			sem_wait(&mutex_PFT);
			fd=nuevoFD(pid);
			crearEntradaEnPFT(pid,permisos,fd,globalFD);
			sem_post(&mutex_PFT);
			log_info(logFile,"se abrio correctamente el archivo %s desde el proceso con pid %d",path,pid);
			t_cantSyscalls* aux = list_find(tablaSyscalls, (void*)_tieneElPid);
			aux->cantAbrir++;
			return fd;
		}else{
			log_error(logFile,"error en apertura de archivo en proceso: %d, con direccion: %s\n",
					pid,path);
			return 0;
		}
	}

}

bool cerrarArchivo(uint32_t pid,uint32_t fd){
	log_debug(logFile,"cerrando archivo con FD %d del proceso con PID %d",fd,pid);
	t_entradaPorGFT* entradaGFT=malloc(sizeof(t_entradaPorGFT));
	t_entradaPorPFT* entradaPFT=malloc(sizeof(t_entradaPorPFT));
	sem_wait(&mutex_PFT);
	entradaPFT=entradaPorPFT();
	sem_post(&mutex_PFT);
	auxiliarArchivo->globalFDActual=entradaPFT->globalFD;
	auxiliarArchivo->fdActual=fd;
	auxiliarArchivo->pidActual=pid;
	sem_wait(&mutex_GFT);
	entradaGFT=entradaPorGFT();
	sem_post(&mutex_GFT);
	if(entradaGFT==NULL){
			return false;
			log_error(logFile,"no se pudo cerrar el archivo con FD %d del proceso con PID %d",fd,pid);

		}else{
			entradaGFT->open--;
			if(entradaGFT->open==0){
				sem_wait(&mutex_PFT);
				borrarEntradaPFT(pid,fd);
				sem_post(&mutex_PFT);
				sem_wait(&mutex_GFT);
				list_remove_and_destroy_by_condition(tablaGFT,(void*)aBorrarGFT,(void*)destroyGFT);
				sem_post(&mutex_GFT);
			}
			_Bool _tieneElPid(t_PCB *prog) {
				return (prog->PID == pid);
			}

			t_cantSyscalls* aux = list_find(tablaSyscalls, (void*)_tieneElPid);
			aux->cantCerrar++;

			log_info(logFile,"se cerro correctamente el archivo con FD %d desdel el proceso con PID",fd,pid);
			return true;
		}
}

bool borrarArchivo(uint32_t pid, uint32_t fd){
	log_debug(logFile,"borrando archivo con FD %d del proceso con PID %d",fd,pid);
	auxiliarArchivo->fdActual=fd;
	auxiliarArchivo->pidActual=pid;
	sem_wait(&mutex_PFT);
	t_entradaPorPFT* entradaPFT=entradaPorPFT();
	sem_post(&mutex_PFT);
	auxiliarArchivo->globalFDActual=entradaPFT->globalFD;
	sem_wait(&mutex_GFT);
	t_entradaPorGFT* entrada= entradaPorGFT();
	sem_post(&mutex_GFT);
	if((entrada->open==1)&&(entradaPFT->globalFD==entrada->globalFD)){
		if(borrarArchivoFS(entrada->path)>0){
	/*		sem_wait(&mutex_PFT);
			entrada->open--;
			borrarEntradaPFT(pid,fd);
			sem_post(&mutex_PFT);
			sem_wait(&mutex_GFT);
			list_remove_and_destroy_by_condition(tablaGFT,(void*)aBorrarGFT,(void*)destroyGFT);
			sem_post(&mutex_GFT);*/
			log_info(logFile,"se borro correctamente el archivo con FD %d del proceso con PID %d",fd,pid);
			_Bool _tieneElPid(t_PCB *prog)
			{
				return (prog->PID == pid);
			}
			t_cantSyscalls* aux = list_find(tablaSyscalls, (void*)_tieneElPid);
			aux->cantBorrar++;
			return true;
		}
		log_error(logFile,"no se pudo borrar el archivo con FD %d del proceso con PID %d",fd,pid);
		return false;
	}else{
		log_error(logFile,"no se pudo borrar el archivo con FD %d del proceso con PID %d",fd,pid);
		return false;
	}
}

bool moverCursor(uint32_t fd, uint32_t pid, uint32_t posicion){
	log_debug(logFile,"se mueve el cursor a la posicion %d del archivo con FD %d del proceso con PID %d",posicion,fd,pid);
	auxiliarArchivo->fdActual=fd;
	auxiliarArchivo->pidActual=pid;
	sem_wait(&mutex_PFT);
	t_entradaPorPFT* entrada=entradaPorPFT();
	entrada->cursor=posicion;
	sem_post(&mutex_PFT);
	_Bool _tieneElPid(t_PCB *prog)
	{
		return (prog->PID == pid);
	}
	t_cantSyscalls* aux = list_find(tablaSyscalls, (void*)_tieneElPid);
	aux->cantBorrar++;
	return true;
}

bool escribirArchivo(uint32_t fd, uint32_t pid,char* info,uint32_t size){
	log_debug(logFile,"escribiendo >%s< con tamanio %d en el archivo con FD %d del proceso con PID %d",info,size,fd,pid);
	auxiliarArchivo->fdActual=fd;
	auxiliarArchivo->pidActual=pid;
	sem_wait(&mutex_PFT);
	t_entradaPorPFT* entradaPFT=entradaPorPFT();
	auxiliarArchivo->globalFDActual=entradaPFT->globalFD;
	sem_post(&mutex_PFT);
	sem_wait(&mutex_GFT);
	t_entradaPorGFT* entrada= entradaPorGFT();
	sem_post(&mutex_GFT);
	if(entradaPFT->flags.escritura){
		if(guardarDatosFS(entrada->path,entradaPFT->cursor,size,info))
		{	_Bool _tieneElPid(t_PCB *prog) {
			return (prog->PID == pid);
		}

			t_cantSyscalls* aux = list_find(tablaSyscalls, (void*)_tieneElPid);
			aux->cantEscribir++;
			return true;
		}
		else{
			log_error(logFile,"se obtiene error de FS al escribir el archivo con FD %d del proceso con PID %d por no tener los permisos necesarios",fd,pid);
			return false;
		}
	}
	else{
		log_error(logFile,"no se pudo escribir el archivo con FD %d del proceso con PID %d por no tener los permisos necesarios",fd,pid);
		return false;
	}
}

bool leerArchivo(uint32_t pid, uint32_t fd, uint32_t size,char** buffer){
	log_debug(logFile,"leyendo datos con tamanio %d del archivo con FD %d del proceso con PID %d",size,fd,pid);
	auxiliarArchivo->fdActual=fd;
	auxiliarArchivo->pidActual=pid;
	sem_wait(&mutex_PFT);
	t_entradaPorPFT* entradaPFT=entradaPorPFT();
	sem_post(&mutex_PFT);
	auxiliarArchivo->globalFDActual=entradaPFT->globalFD;
	sem_wait(&mutex_GFT);
	t_entradaPorGFT* entrada= entradaPorGFT();
	sem_post(&mutex_GFT);
	if(entradaPFT->flags.lectura){
		*buffer=strdup(obtenerDatos(entrada->path,entradaPFT->cursor,size));
		if(*buffer!=NULL)
		{	_Bool _tieneElPid(t_PCB *prog) {
			return (prog->PID == pid);
		}
			t_cantSyscalls* aux = list_find(tablaSyscalls, (void*)_tieneElPid);
			aux->cantLeer++;
			log_info(logFile,"se leyo >%s< desde el proceso con pid %d del archivo %s con fd %d",*buffer,pid,entrada->path,fd);
			return true;
		}else{
			log_error(logFile,"se obtiene error de FS al escribir el archivo con FD %d del proceso con PID %d por no tener los permisos necesarios",fd,pid);
			return false;
		}
	}
	else{
		log_error(logFile,"no se pudo leer el archivo con FD %d del proceso con PID %d por no tener los permisos necesarios",fd,pid);

		return false;
	}
}

///operaciones auxiliares de operaciones de archivos
t_entradaPorPFT* crearEntradaEnPFT(uint32_t pid,t_flags flags,uint32_t fd,uint32_t globalfd){
	log_debug(logFile,"creando entrada en la tabla de archivos con FD %d del proceso con pid %d",fd,pid);
	t_entradaPorPFT* nuevaEntrada=malloc(sizeof(t_entradaPorPFT));
	nuevaEntrada->flags=flags;
	nuevaEntrada->cursor=0;
	nuevaEntrada->fd=fd;
	nuevaEntrada->pid=pid;
	nuevaEntrada->globalFD=globalfd;
	nuevaEntrada->flags=flags;
	list_add(tablaPFT,nuevaEntrada);

	return nuevaEntrada;

}

int agregarEntradaGFT(char*path){
	log_debug(logFile,"agregando nueva entrada a la tabla global de archivos, con path : %s",path);

	auxiliarArchivo->pathAuxiliar=malloc(strlen(path));
	auxiliarArchivo->pathAuxiliar=string_duplicate(path);
	t_entradaPorGFT* entrada;

	if(list_find(tablaGFT,(void*)tieneElPath)==NULL)
	{
		return crearEntradaGFT(path);
	}else{
		entrada=list_find(tablaGFT,(void*)tieneElPath);
		entrada->open++;

		return entrada->globalFD;
	}
}

int crearEntradaGFT(char* path)
{
	log_debug(logFile,"creando nueva entrada a la tabla global de archivos, con path : %s",path);
	t_entradaPorGFT* entrada;
	entrada=malloc(sizeof(t_entradaPorGFT)+strlen(path));
	entrada->globalFD=nuevoGFD();
	entrada->path=strdup(path);
	entrada->open=1;
	list_add(tablaGFT,entrada);

	return entrada->globalFD;
}

int nuevoGFD()
{
	int i=0;
	int nuevoFD=0;
	t_entradaPorGFT* entradaAux;
	list_sort(tablaGFT,(void*)fdMenorGFT);
	while(i<list_size(tablaGFT)){
		entradaAux=list_get(tablaGFT,i);
		if(nuevoFD <= entradaAux->globalFD){
		i++;
		nuevoFD++;
		}else
			return nuevoFD;
	}
	return nuevoFD;
}

int nuevoFD(uint32_t pid)
{
	t_list* listaPorPID;
	listaPorPID=list_filter(tablaPFT,(void*)esDelProceso);
	list_sort(listaPorPID,(void*)fdMenorPFT);
	int nuevoFD=3;
	t_entradaPorPFT* entradaAux;
	int i=0;
	while(i<list_size(tablaPFT)){
		entradaAux=list_get(tablaPFT,i);
		if(nuevoFD < entradaAux->fd){
			return nuevoFD;
		}else
		i++;
		nuevoFD++;
	}
	return nuevoFD;
}

bool tieneElPath(t_entradaPorGFT* entrada){
	return string_equals_ignore_case(entrada->path,auxiliarArchivo->pathAuxiliar);
}

bool esDelProceso(t_entradaPorPFT* entrada){
	return entrada->pid==auxiliarArchivo->pidActual;
}

bool fdMenorGFT(t_entradaPorGFT *fd, t_entradaPorGFT *mayorfd)
{
	return fd->globalFD < mayorfd->globalFD;
}

bool fdMenorPFT(t_entradaPorPFT *fd, t_entradaPorPFT *mayorfd)
{
	return fd->fd < mayorfd->fd;
}

int validarArchivo(char* path)
{
	t_validarArchivo* opera=malloc(sizeof(t_validarArchivo));
	opera->path=strdup(path);
	opera->resultado=1;
	header_t* cabeza=malloc(sizeof(header_t));
	cabeza->type=VALIDAR_ARCHIVO;
	char* amandar;
	t_resultado* resultado;
	char* recibido;
	amandar=validarArchivoSerializer(opera,&cabeza->length);

	sockets_send(socketFS,cabeza,amandar);
	recv(socketFS,cabeza,sizeof(header_t),0);

	if(cabeza->type==RESULTADO)
	{
		recibido=malloc(cabeza->length);
		recv(socketFS,recibido,cabeza->length,0);
		resultado=validarArchivoDesSerializer(recibido);
		log_info(logFile, "se encontro un resultado al validar el archivo con ruta:%s\n",opera->path);
	}

	else{
		log_error(logFile, "error al recibir respuesta en validarArchivo con ruta:%s\n",opera->path);
		free(opera->path);
		free(opera);
		return -1;

	}

	if(resultado->resultado==EXISTE)
	{
		log_info(logFile, "Existe el archivo con "
				"ruta:%s\n",opera->path);
		free(opera->path);
		free(opera);
		free(resultado);
		free(recibido);
	return true;
	}

	else{
		log_error(logFile, "no existe  el archivo con ruta:%s\n",opera->path);
		free(opera->path);
		free(opera);
		free(resultado);
		free(recibido);
		return false;
	}
}

int crearArhivoFS(char* path){
	t_crearArchivo* opera=malloc(sizeof(t_borrarAchivo));
	opera->path=strdup(path);
	opera->resultado=1;
	header_t cabeza;
	cabeza.type=CREAR_ARCHIVO;
	char* amandar;
	t_resultado* resultado;
	char* recibido;
	amandar=validarArchivoSerializer(opera,&cabeza.length);

	sockets_send(socketFS,&cabeza,amandar);
	recv(socketFS,&cabeza,sizeof(header_t),0);

	if(cabeza.type==RESULTADO)
	{
		recibido=malloc(cabeza.length);
		recv(socketFS,recibido,cabeza.length,0);
		resultado=validarArchivoDesSerializer(recibido);
		log_info(logFile, "se obtuvo un resultado al crear el archivo con ruta:s\n",opera->path);
	}
		else{
			log_error(logFile, "No se obtuvo un resultado al crear el archivo con ruta:s\n",opera->path);

			free(opera->path);
			free(opera);
			return -1;
		}

	if(resultado->resultado==EXITO)
	{
			log_info(logFile,"el archivo con ruta:%s fue creado con exito\n", opera->path);
				  free(opera);
				  free(resultado);
				  free(recibido);
			return 1;
			}

		else{
			log_error(logFile, "no se pudo crear el archivo con ruta:%s\n",opera->path);
			free(opera->path);
			free(opera);
			free(resultado);
			free(recibido);
			return -2;
		}
}

int borrarArchivoFS(char* path){
	t_borrarAchivo* opera=malloc(sizeof(t_borrarAchivo));
		opera->path=strdup(path);
		opera->resultado=1;
		header_t cabeza;
		cabeza.type=BORRAR_ARCHIVO;
		char* amandar;
		char* recibido;
		t_resultado* resultado;
		amandar=validarArchivoSerializer(opera,&cabeza.length);

		sockets_send(socketFS,&cabeza,amandar);
		recv(socketFS,&cabeza,sizeof(header_t),0);

		if(cabeza.type==RESULTADO)
		{
				recibido=malloc(cabeza.length);
				recv(socketFS,recibido,cabeza.length,0);
				resultado=validarArchivoDesSerializer(recibido);
				log_info(logFile, "se obtuvo un resultado al borrar el archivo con ruta:s\n",
						opera->path);
		}
			else{
				log_error(logFile, "No se obtuvo un resultado al borrar el archivo con ruta:s\n",
						opera->path);

				free(opera->path);
				free(opera);
				return -1;
			}

			if(resultado->resultado==EXITO)
			{
				log_info(logFile, "el archivo con ruta:s fue borrado exitosamente\n",opera->path);

				free(opera->path);
				free(opera);
				free(resultado);
				free(recibido);
				return 1;
				}
			else{
				log_error(logFile, "No se pudo borrar el archivo con ruta:s\n",opera->path);

				free(opera->path);
				free(opera);
				free(resultado);
			    free(recibido);
				return -2;
			}
}

int guardarDatosFS(char* path, uint16_t offset,uint16_t size,char* buffer){
	t_guardarDatos* opera=malloc(sizeof(t_guardarDatos));
	//opera->buffer=malloc(strlen(buffer));
	//opera->path=malloc(strlen(path));
	opera->buffer=strdup(buffer);
	opera->tamBuffer=strlen(buffer);
	opera->path=strdup(path);
	opera->tamPath=strlen(path);
	opera->offset=offset;
	opera->size=size;
	char* aMandar;
	char* recibido;
	t_resultado* resultado;
	header_t cabeza;
	cabeza.type=GUARDAR_DATOS;
	aMandar=guardarDatosSerializer(opera,&cabeza.length);

	sockets_send(socketFS,&cabeza,aMandar);
	recv(socketFS,&cabeza,sizeof(header_t),0);

	if(cabeza.type==RESULTADO)
	{
		recibido=malloc(cabeza.length);
		recv(socketFS,recibido,cabeza.length,0);
		resultado=validarArchivoDesSerializer(recibido);
		log_info(logFile, "Se obutvieron resultados al guardarDatos del archivo con ruta:%s\n",
				opera->path);
	}
	else{
		log_error(logFile, "no se obutvo resultado al guardarDatos del archivo con ruta:%s\n",
				opera->path);

		free(opera->path);
		free(opera);
		return -1;
	}

	if(resultado->resultado==EXITO)
	{
		log_info(logFile,"se guardaron datos del archivo con ruta:%s exitosamente\n",opera->path);
		free(opera->path);
		free(opera);
		free(resultado);
		free(recibido);
		return 1;
	}
	else{
		log_error(logFile, "no se pudieron guardar los datos del archivo con ruta:%s\n", opera->path);
		free(opera->path);
		free(opera);
		free(resultado);
		free(recibido);
		return -2;
	}

}

void borrarEntradaPFT(uint32_t pid,uint32_t fd){
	log_debug(logFile,"borrando Entrada del FD %d de la tabla de archivos del proceso con PID %d",fd,pid);

	list_remove_and_destroy_by_condition(tablaPFT,(void*)aBorrarPFT,(void*)destroyPFT);
}

void destroyPFT(t_entradaPorPFT* self){
	free(self);
}

void destroyGFT(t_entradaPorGFT* self){
	free(self->path);
	free(self);
}

char* obtenerDatos(char* path, uint16_t offset,uint16_t size){
	t_obtenerDatos* operacion=malloc(sizeof(t_obtenerDatos));
	operacion->offset=offset;
	operacion->size=size;
	operacion->path=strdup(path);
	operacion->tamPath=strlen(path);
	char* aMandar;
	header_t cabeza;
	cabeza.type=OBTENER_DATOS;
	aMandar=obtenerDatosSerializer(operacion,&cabeza.length);

	sockets_send(socketFS,&cabeza,aMandar);
	char* recibido;
	char* buffer=string_new();
	t_resultado* resultado;

	recv(socketFS,&cabeza,sizeof(header_t),0);

	if(cabeza.type==RESULTADO)
	{
		recibido=malloc(cabeza.length);
		recv(socketFS,recibido,cabeza.length,0);
		resultado=validarArchivoDesSerializer(recibido);
		buffer=strdup(resultado->path);
		log_info(logFile, "Se obtuvieron resultados al obtenerDatos del archivo con ruta:%s\n",
					operacion->path);
	}
	else{
		log_error(logFile, "No se obtuvieron resultados al obtenerDatos del archivo con ruta:%s\n",
				operacion->path);
		free(operacion);
		free(operacion->path);
		return NULL;
	}

	if(resultado->resultado==EXITO)
	{
		log_info(logFile, "se obtuvieron exitosamente los datos del archivo con ruta:%s\n"
				,operacion->path);
		free(operacion);
		free(resultado);
		free(recibido);
		return buffer;
	}
	else{
		log_error(logFile, "No se pudieron obtener datos del archivo con ruta:%s\n", operacion->path);
		free(operacion);
		free(operacion->path);
		free(resultado);
		free(recibido);
		return NULL;
	}
}

bool aBorrarPFT(t_entradaPorPFT* entrada){
	return (entrada->fd==auxiliarArchivo->fdActual)&&(entrada->pid==auxiliarArchivo->pidActual);
}

bool aBorrarGFT(t_entradaPorGFT* entrada){
	return (entrada->open==0);

}

t_entradaPorPFT* entradaPorPFT(){
	return list_find(tablaPFT,(void*)esDelFD);
}

t_entradaPorGFT* entradaPorGFT(){
	return list_find(tablaGFT,(void*)esDelGFD);
}

bool esDelFD(t_entradaPorPFT* entrada){
	return (entrada->fd==auxiliarArchivo->fdActual)&&(entrada->pid==auxiliarArchivo->pidActual);
}

bool esDelGFD(t_entradaPorGFT* entrada){
	return (entrada->globalFD==auxiliarArchivo->globalFDActual);
}
