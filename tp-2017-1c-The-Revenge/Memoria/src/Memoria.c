#include <Sockets/StructsUtils.h>
#include <Sockets/Serializer.h>
#include <Sockets/socket.h>
#include <stdio.h>
#include <semaphore.h>
#include "Memoria.h"
#include "configMemoria.h"

parametrosMemoria parametros;
metadataMemoria metadata;
bool fin;

sem_t semTablaPaginas,semCache;

t_log* ptrLog;

t_log* logTabla,*logCache;

pthread_t h_EscuchaTec, h_EscuchaConexiones;

t_list *paginasXprocesos; //TABLA PARA SABER NUMERO DE NUEVA PAGINA
//TABLAS ADMINISTRATIVA CACHE
t_list *cacheLibre;
t_list *cacheOcupada; //ESTA MANTIENE EL ORDEN LRU: PRIMERAS POSICIONES, MAS VIEJAS

void *memoria, *primerMarcoDatos,*cache;

uint32_t cantidadFramesLibres, cantidadFramesDatos,cantidadFramesAdministracion,primerFrameDatos;
uint32_t socketReceptorKernel, socketReceptorCPU;

//CALCULO CUANTO FRAMES NECESITO PARA LAS ESTRUCTURAS ADMINISTRATIVAS
uint32_t calcularFramesParaAdministracion() {
	//REDONDEO PARA ARRIBA
	uint32_t cantMarcos = metadata.marcos,tamanioStructAdmin = sizeof(estructuraAdministrativa),tamanioMarco = metadata.marcos_size;

	uint32_t resultado = ceil( ((double)(cantMarcos * tamanioStructAdmin))/((double)tamanioMarco) );
	return resultado;
}

uint32_t contarEntradasPidCache(uint32_t pid) //SIN SINCRO
{
	uint32_t i,cantidadEntradasProceso = 0;
	entradaCache* aux;

	for(i=0;i<list_size(cacheOcupada);i++)
	{
		aux = list_get(cacheOcupada,i);
		if(aux->pid == pid)
			cantidadEntradasProceso ++;
	}

	return cantidadEntradasProceso;
}

bool maximoDeEntradasCachePorProcesoAlcanzado(uint32_t pid) //SIN SINCRO
{
	return (contarEntradasPidCache(pid) == (metadata.cache_x_proceso));
}

uint32_t obtenerIndiceEntradaLRUPorPidCache(uint32_t pid) {  //SIN SINCRO

	uint32_t i;
	entradaCache* aux;

	for(i=0;i<list_size(cacheOcupada);i++)
	{
		aux = list_get(cacheOcupada,i);
		if((aux->pid) == pid)
			return i;
	}

	return -1;
}

void cargarEntradasLibresCache() //SIN SINCRO
{
	entradaCache* aux;
	uint32_t i;

	for(i=0;i< metadata.entradas_cache;i++)
	{
		aux = malloc(sizeof(entradaCache));
		aux->contenido = cache + i*metadata.marcos_size;
		aux->nroPag = -1;
		aux->pid = -1;
		list_add(cacheLibre,aux);
	}
}

void inicializarCache()
{
	cache = malloc(metadata.entradas_cache * metadata.marcos_size);

	cacheLibre = list_create();

	cacheOcupada = list_create();

	cargarEntradasLibresCache();
}

void inicializarEspacioMemoria() //CON SINCRO
{
	sem_init(&semTablaPaginas, 0, 1);

	sem_init(&semCache, 0, 1);

	sem_wait(&semTablaPaginas);

	sem_wait(&semCache);

	uint32_t i;

	paginasXprocesos = list_create();

	memoria = malloc(metadata.marcos * metadata.marcos_size);

	inicializarCache();

	primerFrameDatos = cantidadFramesAdministracion = calcularFramesParaAdministracion();

	cantidadFramesLibres=metadata.marcos - cantidadFramesAdministracion;

	cantidadFramesDatos = metadata.marcos - cantidadFramesAdministracion;

	primerMarcoDatos = memoria + (cantidadFramesAdministracion * metadata.marcos_size);

	estructuraAdministrativa* aux;

	for(i=0;i<metadata.marcos;i++)
	{
		aux = memoria + i*sizeof(estructuraAdministrativa);
		aux->frame = i;
		aux->pagina = -1;
		aux->pid = -1;
	}

	sem_post(&semTablaPaginas);

	sem_post(&semCache);
}

uint32_t funcionHash(uint32_t pid, uint32_t nroPag)  //ESTA GENERA COLISIONES, MIRAR FRAMEPIDPAGINA
{
	uint64_t temp;

	uint32_t frame;

	temp = (uint64_t)nroPag << 32;

	temp = temp + (uint64_t) pid;

	frame = (uint32_t)(temp % cantidadFramesDatos);

	return frame + cantidadFramesAdministracion;
}

uint32_t framePidPagina(uint32_t pid, uint32_t nroPagina, bool(*criterio)(uint32_t,uint32_t,uint32_t) ) // SIN SINCRO
{
	uint32_t frameFuncion = funcionHash(pid, nroPagina), i;

	if (criterio(pid, nroPagina, frameFuncion)) {
		return frameFuncion;
	} else {
		if(frameFuncion == metadata.marcos -1) //SI JUSTO ERA EL ULTIMO
			i = primerFrameDatos;	//SIGO BUSCANDO DESDE EL PRINCIPIO
		else
			i = frameFuncion + 1;

		while (1) {
			if (i == frameFuncion) //SI EMPIEZO A RECORRER DE VUELTA
				break;
			if (criterio(pid, nroPagina, i))
			{
				return i;
			}
			if (i == metadata.marcos -1)  //SI LLEGO AL FINAL SIGO DESDE EL PRIMERO
				i = primerFrameDatos;
			else
				i++;

		}
		return -1;
	}
}

void* insertarEntradaAdministrativa(uint32_t frame, uint32_t pid, uint32_t nroPagina) //SIN SINCRO
{

	estructuraAdministrativa* aux = memoria + frame * sizeof(estructuraAdministrativa);
	aux->frame = frame;
	aux->pagina = nroPagina;
	aux->pid = pid;

	return aux;
}

bool criterioBusqueda(uint32_t pid, uint32_t nroPagina, uint32_t indice) //SIN SINCRO
{
	estructuraAdministrativa* aux;

	aux = &((estructuraAdministrativa*)memoria)[indice];

	return ((aux->pid == pid) && (aux->pagina == nroPagina));
}

bool criterioAlmacenamiento(uint32_t pid, uint32_t nroPagina, uint32_t indice) { //SIN SINCRO
	estructuraAdministrativa* aux;
	aux = &((estructuraAdministrativa*)memoria)[indice];
	return (aux->pid == -1);
}

void* solicitarBytesMemoria(uint32_t pid, uint32_t nroPagina, uint32_t offset) //CON SINCRO
{
	void* aux = -1;

	sem_wait(&semTablaPaginas);

	uint32_t frame = framePidPagina(pid, nroPagina, criterioBusqueda);

	sem_post(&semTablaPaginas);

	if (frame != -1)
	{
		aux = memoria + frame * metadata.marcos_size + offset;
	}

	sleep(0.001*metadata.retardo_memoria);

	return aux;
}

uint32_t numeroEntradaCache(uint32_t pid, uint32_t nroPagina) { //SIN SINCRO
	uint32_t i;

	entradaCache* aux;

	for (i = 0; i < list_size(cacheOcupada); i++) {
		aux = list_get(cacheOcupada,i);
		if ((aux->pid == pid) && (aux->nroPag == nroPagina))
			return i;
	}
	return -1;
}

void* solicitarBytesCache(uint32_t pid, uint32_t nroPag, uint32_t offset) //SIN SINCRO PORQUE DEVUELVO REFERENCIA DIRECTA AL DATO
{
	uint32_t indice = -1;

	if(metadata.entradas_cache > 0 && metadata.cache_x_proceso > 0)
	{
		entradaCache* aOcupar;

		indice = numeroEntradaCache(pid, nroPag);

		if(indice!=-1)
		{
			aOcupar = list_get(cacheOcupada,indice);
			list_remove(cacheOcupada, indice);
			list_add(cacheOcupada, aOcupar);

			return aOcupar->contenido + offset;
		}

	}

	return indice;
}

void cargarEntradaCache(uint32_t pid, uint32_t nroPag, void*buffer) //SIN SINCRO
{

	if(metadata.entradas_cache > 0 && metadata.cache_x_proceso > 0)
	{
		entradaCache * aOcupar;
		uint32_t indice;

		if (maximoDeEntradasCachePorProcesoAlcanzado(pid)) //MAXIMO DE ENTRADA POR PID ALCANZADO, REEMPLAZO LRU PAG DE PID
		{
			indice = obtenerIndiceEntradaLRUPorPidCache(pid);

			aOcupar = list_get(cacheOcupada,indice);

			list_remove(cacheOcupada, indice);
		}

		else //EL PROCESO NO SUPERO LA CANTIDAD MAXIMA DE ENTRADAS EN CACHE POR PROCESO
		{
			//VERIFICO SI HAY ESPACIO LIBRE EN CACHE
			if (list_size(cacheLibre)) //NUEVA ENTRADA CACHE Y HAY ESPACIO EN CACHE
			{
					aOcupar = list_get(cacheLibre,0);
					list_remove(cacheLibre,0);

			}
			else //NUEVA ENTRADA CACHE, NO HAY ESPACIO EN CACHE, REEMPLAZO PAGINA POR LRU DE CUALQUIER PID
			{
				indice = 0;
				aOcupar = list_get(cacheOcupada,indice);
				list_remove(cacheOcupada, indice);
			}
			aOcupar->pid = pid;
		}

		aOcupar->nroPag = nroPag;
		memcpy(aOcupar->contenido,buffer,metadata.marcos_size);
		list_add(cacheOcupada, aOcupar);
	}
}

void* obtenerEntradaPagXProc(uint32_t pid) //SIN SINCRO
{
	uint32_t i;
	paginasXproceso* aux;
	for(i=0;i<list_size(paginasXprocesos);i++)
	{
		aux = list_get(paginasXprocesos,i);
		if(aux->pid == pid)
			return aux;
	}
	return -1;
}

bool existeProceso(uint32_t pid) //SIN SINCRO
{
	return (((uint32_t)obtenerEntradaPagXProc(pid))!= -1);
}

uint32_t asignarPaginasProceso(uint32_t pid, uint32_t cantidadPaginas) //CON SINCRO
{
	sem_wait(&semTablaPaginas);

	if (cantidadFramesLibres >= cantidadPaginas) {
		cantidadFramesLibres -= cantidadPaginas;

		uint32_t cantidadAAsignar = cantidadPaginas, nroPag, frame;

		paginasXproceso* aux;

		if(!existeProceso(pid))
		{
			aux = malloc(sizeof(paginasXproceso));
			aux->cantidadPags = 0;
			aux->pid = pid;
			list_add(paginasXprocesos, aux);
		}
		else
			aux = (paginasXproceso*)obtenerEntradaPagXProc(pid);

		sleep(0.001*metadata.retardo_memoria);

		while (cantidadAAsignar) {
			nroPag = aux->cantidadPags;
			frame = framePidPagina(pid, nroPag, criterioAlmacenamiento);

			insertarEntradaAdministrativa(frame, pid, nroPag);
			cantidadAAsignar--;

			aux->cantidadPags+=1;
		}

		sem_post(&semTablaPaginas);

		return 0;
	}
	else
	{
		sem_post(&semTablaPaginas);

		return -1;
	}
}

bool offsetDentroDePagina(uint32_t inicio,uint32_t offset)
{
	return ((offset + inicio)<=metadata.marcos_size);
}

////////////////////////INTERFACES////////////////////////
uint32_t inicializarPrograma(uint32_t idPrograma, uint32_t cantidadPaginasRequeridas) { //SIN SINCRO

	return (asignarPaginasProceso(idPrograma, cantidadPaginasRequeridas));
}

bytesSolicitud* solicitarBytes(uint32_t pid, uint32_t nroPagina, uint32_t offset, uint32_t tamanio) // CON SINCRO
{
	if(offsetDentroDePagina(offset,tamanio))
	{
		sem_wait(&semCache);

		void* aux = solicitarBytesCache(pid,nroPagina,offset);

		if(aux!=-1)
		{
			bytesSolicitud* respuesta = malloc(sizeof(bytesSolicitud));

			respuesta->memoria = malloc(tamanio);
			memcpy(respuesta->memoria,aux,tamanio);

			respuesta->tamanio = tamanio;
			respuesta->cacheMiss = false;

			sem_post(&semCache);

			return respuesta;
		}
		else
		{
			aux = solicitarBytesMemoria(pid,nroPagina,offset);
		}

		if(aux!=-1)
		{
			bytesSolicitud* respuesta = malloc(sizeof(bytesSolicitud));

			cargarEntradaCache(pid,nroPagina,aux - offset);

			sem_post(&semCache);

			respuesta->memoria = malloc(tamanio);
			memcpy(respuesta->memoria,aux,tamanio);

			respuesta->tamanio = tamanio;
			respuesta->cacheMiss = true;

			log_info(ptrLog, "Cache miss");

			return respuesta;
		}
		else
		{
			sem_post(&semCache);

			return -1;
		}
	}
	else
		return -1;
}

void* almacenarBytes(uint32_t pid, uint32_t nroPagina, uint32_t offset, uint32_t tamanio,void* buffer) //CON SINCRO
{
	bool estaEnCache = false;
	if(offsetDentroDePagina(offset,tamanio))
	{
		sem_wait(&semCache);

		void* aux = solicitarBytesCache(pid, nroPagina, offset);
		if(aux!=-1)
		{
			memcpy(aux, buffer, tamanio);
			estaEnCache = true;

		}

		aux = solicitarBytesMemoria(pid, nroPagina, offset);
		if(aux!=-1)
		{
			memcpy(aux, buffer, tamanio);
			if(!estaEnCache)
				cargarEntradaCache(pid,nroPagina,(aux - offset));
		}

		sem_post(&semCache);

		return aux;
	}
	else
		return -1;
}

void darDeBajaProgramaCache(uint32_t pid) { //SIN SINCRO
	uint32_t i;
	entradaCache*aux;

	for (i = 0; i < list_size(cacheOcupada); i++)
	{
		aux = list_get(cacheOcupada,i);

		if(aux->pid == pid)
		{
			list_remove(cacheOcupada,i);
			list_add(cacheLibre,aux);
			aux->nroPag = -1;
			aux->pid = -1;
			i--;
		}

	}

}

uint32_t darDeBajaEstructurasAdministrativas(uint32_t pid) { //SIN SINCRO
	uint32_t i;
	uint32_t encontrado = -1;

	estructuraAdministrativa* aux = memoria + cantidadFramesAdministracion * sizeof(estructuraAdministrativa);

	sleep(0.001*metadata.retardo_memoria);

	for (i = 0; i < cantidadFramesDatos; i++)
	{
		if (aux->pid == pid)
		{
			aux->pid = -1;
			aux->pagina = -1;
			cantidadFramesLibres++;
			encontrado = 1;
		}
		aux = (estructuraAdministrativa*)((void*)aux + sizeof(estructuraAdministrativa));
	}

		return encontrado;
}

uint32_t darDeBajaEntradaPaginaXProceso(uint32_t pid) { //SIN SINCRO
	uint32_t i;
	paginasXproceso* aux;

	for (i = 0; i < paginasXprocesos->elements_count; i++) {
		aux = list_get(paginasXprocesos, i);
		if (aux->pid == pid) {
			list_remove_and_destroy_element(paginasXprocesos, i, free);
			return 1;
		}
	}
	return -1;
}

uint32_t finalizarPrograma(uint32_t pid) //CON SINCRO
{
	uint32_t respuesta = -1;

	sem_wait(&semTablaPaginas);

	if(existeProceso(pid))
	{
		sem_wait(&semCache);

		darDeBajaProgramaCache(pid);

		sem_post(&semCache);

		darDeBajaEstructurasAdministrativas(pid);

		respuesta = darDeBajaEntradaPaginaXProceso(pid);

		sem_post(&semTablaPaginas);

		return respuesta;
	}

	sem_post(&semTablaPaginas);

	return respuesta;
}

void modificarRetardo()
{
	uint32_t nuevoRetardo;
	do
	{
		printf("Ingrese nuevo retardo de memoria\n");
		scanf("%d",&nuevoRetardo);
		if(nuevoRetardo>0)
			metadata.retardo_memoria = nuevoRetardo;
		else
			printf("Ingrese un valor positivo\n");
	}while(nuevoRetardo<=0);
}

void dumpCache()
{
	uint32_t i, tamanioCache = list_size(cacheOcupada);
	entradaCache* auxCache;

	if(!list_size(cacheOcupada))
	{
		log_info(logCache, "Cache vacia");

		printf("Cache vacia\n");
	}

	else
	{
		log_info(logCache, "Cache:");

		log_info(logCache, "PID   NROPAGINA");

		printf("Cache:\n\n");
		printf("PID   NROPAGINA\n");

		for (i = 0; i < list_size(cacheOcupada); i++)
		{
			auxCache = list_get(cacheOcupada,i);
			log_info(logCache, "%d   %d", auxCache->pid, auxCache->nroPag);
			printf("%d   %d\n", auxCache->pid, auxCache->nroPag);
		}

		for (i = 0; i < list_size(cacheLibre); i++)
		{
			auxCache = list_get(cacheLibre,i);
			log_info(logCache, "%d   %d", auxCache->pid, auxCache->nroPag);
			printf("%d   %d\n", auxCache->pid, auxCache->nroPag);
		}
	}
}

void dumpAdministracion()
{
	uint32_t i;
	estructuraAdministrativa* aux;
	paginasXproceso* aux2;


	log_info(logTabla, "Administracion");

	log_info(logTabla, "FRAME     PID     NROPAGINA");

	printf("Administracion:\n\n");
	printf("FRAME     PID     NROPAGINA\n");

	for (i = 0; i < metadata.marcos; i++)
	{
		aux = memoria + i * sizeof(estructuraAdministrativa);
		log_info(logTabla, "%d          %d          %d", aux->frame, aux->pid, aux->pagina);
		printf("%d          %d          %d\n",aux->frame,aux->pid,aux->pagina);
	}

	if(list_size(paginasXprocesos))
	{
		log_info(logTabla, "Listado de procesos");
		log_info(logTabla, "PID");
		printf("PID\n");

		for(i=0;i<paginasXprocesos->elements_count;i++)
		{
			aux2 = list_get(paginasXprocesos,i);
			log_info(logTabla, "%d",(int)aux2->pid);
			printf("%d\n",aux2->pid);
		}

	}
	else
	{
		log_info(logTabla, "No hay procesos");
		printf("No hay procesos\n");
	}
}

void DumpHex(void* data, size_t size) {
	char ascii[71];
	size_t i, j;
	ascii[70] = '\0';
	for (i = 0; i < size; ++i) {

//		printf("%02X ", ((unsigned char*)data)[i]);
//		log_info(logTabla, "%02X ", ((unsigned char*)data)[i]);

		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 70] = ((unsigned char*)data)[i];
		} else {
			ascii[i % 70] = ' ';
		}
		if ((i+1) % 35 == 0 || i+1 == size) {
			//printf(" ");
			if ((i+1) % 70 == 0) {

				printf("%s\n", ascii);
				log_info(logTabla, "%s", ascii);

			} else if (i+1 == size) {
				ascii[(i+1) % 70] = '\0';
				if ((i+1) % 70 <= 8) {
//					printf(" ");
				}
				for (j = (i+1) % 70; j < 70; ++j) {
//					printf("   ");
				}
				printf("%s\n", ascii);
				log_info(logTabla, "%s", ascii);
			}
		}
	}
}

void dumpContenido()
{
	uint32_t i,j;
	estructuraAdministrativa* aux;
	paginasXproceso* aux2;
	char * aux3;
	char * buffer = malloc(metadata.marcos_size +1);
	buffer[metadata.marcos_size] = '\0';

	bool pag =false;
	log_info(logTabla, "Contenido Frames Asignados");

	printf("\nContenido Frames Asignados\n");

	for (i = 0; i < metadata.marcos; i++)
	{
		aux = memoria + i * sizeof(estructuraAdministrativa);

		if(aux->pid != -1)
		{
			aux3 = (char *)(memoria + i * metadata.marcos_size);
			j=0;
			while(j<metadata.marcos_size)
			{
				if(aux3[0] != '\0')
					buffer[j]= aux3[0];

				else
					buffer[j]= (char)32;

				aux3 +=1;
				j++;
			}
			log_info(logTabla, "FRAME %d PID %d PAGINA %d Contenido ", i,aux->pid,aux->pagina);
			printf("FRAME %d PID %d PAGINA %d Contenido \n", i,aux->pid,aux->pagina);
			DumpHex(buffer,metadata.marcos_size);
			printf("\n");
			pag = true;
		}

	}

	if(!pag)
	{
		log_info(logTabla, "No se asignaron paginas\n");
		printf("No se asignaron paginas\n");
	}
	free(buffer);
}

void dumpContenidoPid(uint32_t pid)
{
	uint32_t i,j;
	estructuraAdministrativa* aux;
	paginasXproceso* aux2;
	char * aux3;
	char * buffer = malloc(metadata.marcos_size +1);
	buffer[metadata.marcos_size] = '\0';

	bool pag =false;
	log_info(logTabla, "Contenido Frames Asignados");

	printf("\nContenido Frames Asignados\n");

	for (i = 0; i < metadata.marcos; i++)
	{
		aux = memoria + i * sizeof(estructuraAdministrativa);

		if(aux->pid == pid)
		{
			aux3 = (char *)(memoria + i * metadata.marcos_size);
			j=0;
			while(j<metadata.marcos_size)
			{
				if(aux3[0] != '\0')
					buffer[j]= aux3[0];

				else
					buffer[j]= (char)32;

				aux3 +=1;
				j++;
			}
			log_info(logTabla, "FRAME %d PID %d PAGINA %d Contenido ", i,aux->pid,aux->pagina);
			printf("FRAME %d PID %d PAGINA %d Contenido \n", i,aux->pid,aux->pagina);
			DumpHex(buffer,metadata.marcos_size);
			pag = true;
		}

	}

	if(!pag)
	{
		log_info(logTabla, "No tiene paginas");
		printf("No tiene paginas");
	}
}

void flushCache()
{
	uint32_t i;

	sem_wait(&semCache);

	if(list_size(cacheOcupada))
	{
		list_clean_and_destroy_elements(cacheOcupada,free);
	}

	if(list_size(cacheLibre))
	{
		list_clean_and_destroy_elements(cacheLibre,free);
	}

	list_destroy(cacheOcupada);

	list_destroy(cacheLibre);

	free(cache);

	inicializarCache();

	sem_post(&semCache);

	printf("Cache limpia\n");
}

void sizeMemoria()
{
	uint32_t cantidadFramesOcupados = metadata.marcos - cantidadFramesLibres;

	t_log* logger = log_create("sizeMemoria", "Memoria.c", false, LOG_LEVEL_INFO);

	log_info(logger, "Frames Totales       Frames Ocupados       Frames Libres");

	log_info(logger, "Cantidad Peso        Cantidad Peso         Cantidad Peso\n");

	printf("Frames Totales       Frames Ocupados       Frames Libres\n");
	printf("Cantidad Peso        Cantidad Peso         Cantidad Peso\n");

	log_info(logger, "  %d     %d           %d      %d            %d      %d", metadata.marcos, metadata.marcos * metadata.marcos_size, cantidadFramesOcupados,cantidadFramesOcupados* metadata.marcos_size,cantidadFramesLibres,cantidadFramesLibres * metadata.marcos_size);
	printf("%d      %d          %d     %d           %d     %d\n\n", metadata.marcos, metadata.marcos * metadata.marcos_size, cantidadFramesOcupados,cantidadFramesOcupados* metadata.marcos_size,cantidadFramesLibres,cantidadFramesLibres * metadata.marcos_size);
}

void sizePid(uint32_t pid)
{
	t_log* logger = log_create("sizePid", "Memoria.c", false, LOG_LEVEL_INFO);

	paginasXproceso* entradaProceso= obtenerEntradaPagXProc(pid);

	if(entradaProceso != -1)
	{
		log_info(logger, "PID Nro: %d \nCantidad de paginas: %d\nTamanio pagina:%d\nTamanio total del proceso:%d\n",pid,entradaProceso->cantidadPags,metadata.marcos_size,metadata.marcos_size*entradaProceso->cantidadPags);
		printf("PID Nro: %d \nCantidad de paginas: %d\nTamanio pagina:%d\nTamanio total del proceso:%d\n",pid,entradaProceso->cantidadPags,metadata.marcos_size,metadata.marcos_size*entradaProceso->cantidadPags);
	}
	else
	{
		printf("No existe el proceso en el sistema\n");
		log_info(logger,"No existe el proceso en el sistema\n");
	}
}

void atenderConexion(uint32_t socket) {

	header_t header;

	log_info(ptrLog,"Se inicia hilo de conexion para recibir Kernel o Cpu\n");

	recv(socket, &header, sizeof(header_t), MSG_WAITALL);

	switch(header.type){
		case HANDSHAKE_KERNEL:
			enviarHandshake(socket,HANDSHAKE_MEMORIA);
			atenderKernel(socket);
			break;
		case HANDSHAKE_CPU:
			enviarHandshake(socket,HANDSHAKE_MEMORIA);
			atenderCpu(socket);
			break;
		default:
			;
			log_error(ptrLog,"Protocolo Inesperado finaliza hilo\n");
	}
}

uint32_t enviarTamanioPagina(uint32_t socket)
{
	header_t header;
	header.type=TAMANIO_PAGINA;
	uint32_t *notificacion_tamanioPagina=malloc(sizeof(uint32_t));
	*notificacion_tamanioPagina=metadata.marcos_size;

	char *serialized = uint32_serializer(notificacion_tamanioPagina,&header.length);

	uint32_t nbytes=sockets_send(socket,&header,serialized);

	free(notificacion_tamanioPagina);
	free(serialized);

	return nbytes;
}

uint32_t enviarError(uint32_t socket)
{
	header_t header;
	header.type = ERROR;
	header.length = 0;

	uint32_t nbytes=sockets_send(socket, &header, '\0');

	return nbytes;
}

uint32_t enviarOk(uint32_t socket)
{
	header_t header;
	header.type = SUCCESS;
	header.length = 0;

	uint32_t nbytes=sockets_send(socket, &header, '\0');

	return nbytes;
}

uint32_t enviarBytesLeidos(uint32_t socket,void* datos,uint32_t tamanio)
{
	header_t header;
	header.type= SUCCESS;
	header.length=tamanio;

	//char * buffer = bytesLeidos_serializer(datos,&header.length);

	uint32_t nbytes=sockets_send(socket,&header,datos);

	return nbytes;
}

void atenderKernel(uint32_t socket)
{
	log_info(ptrLog,"Atender kernel socket: %d\n", socket);
	if(enviarTamanioPagina(socket)<=0)
		log_error(ptrLog,"Error al enviar tamanio de pagina a la Kernel\n");

	header_t header;
				uint32_t resultado, respuestaSolicitud,pid;
				bool salida=false;
				t_solicitudPaginas* solicitudPaginas;
				t_solicitudEscritura* solicitudEscritura;
				char* buffer;

				while(!salida)
				{
					resultado = recv(socket, &header, sizeof(header_t), 0);
					if((resultado== 0)||(resultado== -1))
					{
						close(socket);
						salida = true;
					}
					else
					{
						switch(header.type)
						{
						case(INICIAR_PROGRAMA):
							buffer = malloc(header.length);
							resultado = recv(socket, buffer, header.length, 0);
							if((resultado== 0)||(resultado== -1))
							{
								free(buffer);
								close(socket);
								salida = true;
								break;
							}
							else
							{
								solicitudPaginas = solicitudPaginas_deserializer(buffer);
								log_debug(ptrLog,"Solicitud IniciarPrograma (Kernel) - solicitudPaginas.pid= %d, solicitudPaginas.cantidadPaginas= %d ",solicitudPaginas->pid,solicitudPaginas->cantidadPaginas);
								free(buffer);
								respuestaSolicitud = inicializarPrograma(solicitudPaginas->pid,solicitudPaginas->cantidadPaginas);
								free(solicitudPaginas);
								if(respuestaSolicitud != -1)
								{
									resultado = enviarOk(socket);
									log_debug(ptrLog,"Se reservaron correctamente las paginas solicitadas");
									if((resultado== 0)||(resultado== -1))
									{
										close(socket);
										salida = true;
										break;
									}
								}
								else
								{
									resultado = enviarError(socket);
									log_error(ptrLog,"No hay espacio suficiente para la reserva de paginas solicitadas");
									if((resultado== 0)||(resultado== -1))
									{
										close(socket);
										salida = true;
										break;
									}
								}
							}
							break;

						case(ESCRIBIR_PAGINA):
							buffer = malloc(header.length);
							resultado = recv(socket, buffer, header.length, 0);
							if((resultado== 0)||(resultado== -1))
							{
								free(buffer);
								close(socket);
								salida = true;
								break;
							}
							else
							{
								solicitudEscritura = solicitudEscritura_deserializer(buffer);
								free(buffer);
								log_debug(ptrLog,"Solicitud de Escritura (Kernel): Pagina: Pid: %d Pagina: %d Offset: %d Tamanio:%d buffer: %s",solicitudEscritura->pid,solicitudEscritura->pagina,solicitudEscritura->offset,solicitudEscritura->tamanio,solicitudEscritura->buffer);
								buffer = almacenarBytes(solicitudEscritura->pid,solicitudEscritura->pagina,solicitudEscritura->offset,solicitudEscritura->tamanio,solicitudEscritura->buffer);

								if((uint32_t)buffer != -1)
								{
									resultado = enviarOk(socket);
									log_debug(ptrLog,"Se escribio correctamente la pagina: %d para el pid: %d, ",solicitudEscritura->pagina,solicitudEscritura->pid);
									free(solicitudEscritura->buffer);
									free(solicitudEscritura);
									if((resultado== 0)||(resultado== -1))
									{
										close(socket);
										salida = true;
										break;
									}
								}
								else
								{
									resultado = enviarError(socket);
									if((resultado== 0)||(resultado== -1))
									{
										close(socket);
										salida = true;
										break;
									}
								}
							}
							break;
						case(ASIGNAR_PAGINAS):
									buffer = malloc(header.length);
									resultado = recv(socket, buffer, header.length, 0);
									if((resultado== 0)||(resultado== -1))
									{
										free(buffer);
										close(socket);
										salida = true;
										break;
									}
									else
									{
										solicitudPaginas = solicitudPaginas_deserializer(buffer);
										free(buffer);
										respuestaSolicitud = asignarPaginasProceso(solicitudPaginas->pid,solicitudPaginas->cantidadPaginas);
										free(solicitudPaginas);

										if(respuestaSolicitud != -1)
										{
											resultado = enviarOk(socket);

											if((resultado== 0)||(resultado== -1))
											{
												close(socket);
												salida = true;
												break;
											}
										}
										else
										{
											resultado = enviarError(socket);
											if((resultado== 0)||(resultado== -1))
											{
												close(socket);
												salida = true;
												break;
											}
										}
									}
									break;
						case(FINALIZAR_PROGRAMA):
											buffer = malloc(header.length);
											resultado = recv(socket, buffer, header.length, 0);
											if((resultado== 0)||(resultado== -1))
											{
												free(buffer);
												close(socket);
												salida = true;
												break;
											}
											else
											{
												pid = (int)(*buffer);
												resultado = finalizarPrograma(pid);
												free(buffer);

//												if(resultado != -1)
//												{
//													resultado = enviarOk(socket);
//
//													if((resultado== 0)||(resultado== -1))
//													{
//														close(socket);
//														salida = true;
//														break;
//
//													}
//												}
//												else
//												{
//													resultado = enviarError(socket);
//													if((resultado== 0)||(resultado== -1))
//													{
//														close(socket);
//														salida = true;
//														break;
//													}
//												}
											}

											break;
						default:
							resultado = enviarError(socket);
							if((resultado== 0)||(resultado== -1))
							{
								close(socket);
								salida = true;
								break;
							}
						}
					}
				}
}

void atenderCpu(uint32_t socket)
{
	log_info(ptrLog,"Atender CPU socket: %d", socket);

	if(enviarTamanioPagina(socket)<=0)
		log_error(ptrLog,"Error al enviar tamanio de pagina a la CPU\n");

	else
		{
			header_t header;
			uint32_t resultado;
			bool salida=false;
			bytesSolicitud* respuestaSolicitud;
			t_solicitudLectura* solicitudLectura;
			t_solicitudEscritura* solicitudEscritura;
			char* buffer;

			while(!salida)
			{
				resultado = recv(socket, &header, sizeof(header_t), 0);
				if((resultado== 0)||(resultado== -1))
				{
					close(socket);
					salida = true;
				}
				else
				{
					switch(header.type)
					{
					case(LEER):
						buffer = malloc(header.length);
						resultado = recv(socket, buffer, header.length, 0);
						if((resultado== 0)||(resultado== -1))
						{
							free(buffer);
							close(socket);
							salida = true;
							break;
						}
						else
						{
							solicitudLectura = solicitudLectura_deserializer(buffer);
							free(buffer);
							log_debug(ptrLog,"Solicitud Lectura (CPU) - solicitudPaginas.pid= %d, solicitudPaginas.pagina= %d, solicitudPaginas.inicio= %d, solicitudPaginas.offset= %d",solicitudLectura->pid,solicitudLectura->pagina,solicitudLectura->inicio,solicitudLectura->offset);
							respuestaSolicitud = solicitarBytes(solicitudLectura->pid,solicitudLectura->pagina,solicitudLectura->inicio,solicitudLectura->offset);
							free(solicitudLectura);
							if((uint32_t)respuestaSolicitud != -1)
							{
								resultado = enviarBytesLeidos(socket,respuestaSolicitud->memoria,respuestaSolicitud->tamanio);
								free(respuestaSolicitud->memoria);
								free(respuestaSolicitud);
								if((resultado== 0)||(resultado== -1))
								{
									close(socket);
									salida = true;
									break;
								}
							}
							else
							{
								resultado = enviarError(socket);
								if((resultado== 0)||(resultado== -1))
								{
									close(socket);
									salida = true;
									break;
								}
							}
						}
						break;
					case(ESCRIBIR_PAGINA):
								buffer = malloc(header.length);
								resultado = recv(socket, buffer, header.length, 0);
								if((resultado== 0)||(resultado== -1))
								{
									free(buffer);
									close(socket);
									salida = true;
									break;
								}
								else
								{
									solicitudEscritura = solicitudEscritura_deserializer(buffer);
									free(buffer);
									log_debug(ptrLog,"Solicitud de Escritura (CPU): Pagina: Pid: %d Pagina: %d Offset: %d Tamanio:%d buffer: %s",solicitudEscritura->pid,solicitudEscritura->pagina,solicitudEscritura->offset,solicitudEscritura->tamanio,solicitudEscritura->buffer);
									buffer = almacenarBytes(solicitudEscritura->pid,solicitudEscritura->pagina,solicitudEscritura->offset,solicitudEscritura->tamanio,solicitudEscritura->buffer);

									if((uint32_t)buffer != -1)
									{
										resultado = enviarOk(socket);
										free(solicitudEscritura->buffer);
										free(solicitudEscritura);
										if((resultado== 0)||(resultado== -1))
										{
											close(socket);
											salida = true;
											break;
										}
									}
									else
									{
										resultado = enviarError(socket);
										if((resultado== 0)||(resultado== -1))
										{
											close(socket);
											salida = true;
											break;
										}
									}
								}
								break;
					default:
						resultado = enviarError(socket);
						if((resultado== 0)||(resultado== -1))
						{
							close(socket);
							salida = true;
							break;
						}
					}
				}
			}
		}

}
uint32_t recibirConexiones(uint32_t socket) {

	uint32_t client, hret;
	pthread_t h_Client;

	while (!fin) {
		//printf("MEMORIA escuchando en el puerto: %d, Socket: %d\n", metadata.puerto,socket);
		log_info(ptrLog, "MEMORIA escuchando en el puerto: %d, Socket: %d", metadata.puerto,socket);
		if (!fin && (client = sockets_accept(socket))) {
			//printf("Se recibio una conexion desde: %d\n",client);
			log_info(ptrLog, "Se recibio conexion desde: %d",client);

			hret = pthread_create(&h_Client, NULL, (void*) atenderConexion, (void*) client);
			if (hret < 0) {
				//mostrarPorPantalla("No se pudo crear el hilo de CPU.");
				log_error(ptrLog, "No se pudo crear el hilo de CPU.");
				return EXIT_FAILURE;
			}
		}
	}
	close(socket);
	shutdown(socket, -2);
	return 0;
}


uint32_t escucharConexiones() {

	uint32_t socketServ;

	socketServ = socket_createServer(metadata.puerto,metadata.backlog);

	if (socketServ < 0) {
		return EXIT_FAILURE;
	}

	return recibirConexiones(socketServ);
}

void consolaMemoria()
{
	uint32_t opcion,opcion2,opcion3;
	printf("Elija una opcion\n");
	do
	{
		printf("1-Cambiar retardo\n2-Dump\n3-Flush cache\n4-Size\n");
		scanf("%d",&opcion);
		switch(opcion)
		{
			case(1):
					modificarRetardo();
					break;
			case(2):
					do
					{
						printf("Elija:\n1-Dump cache\n2-Dump Estructuras Administrativas\n3-Contenido de Memoria\n4-Salir\n");
						scanf("%d",&opcion2);
						switch(opcion2)
						{
						case(1):
							dumpCache();
							break;
						case(2):
							dumpAdministracion();
							break;
						case(3):
							do
							{
								printf("Elija:\n1-Contenido General\n2-Contenido PID\n3-Salir\n");
								scanf("%d",&opcion3);
								switch(opcion3)
								{
								case(1):
									dumpContenido();
									break;
								case(2):
									printf("Ingrese identificador de proceso\n");
									uint32_t pid2 = 0;
									scanf("%d",&pid2);
									dumpContenidoPid(pid2);
									break;
								case(3):
									break;
								default:
									printf("Ingrese opcion valida \n");
									break;
								}
							}while(opcion3!=3);

							break;
						case(4):
							break;
						default:
							printf("Ingrese opcion valida \n");
							break;
						}
					}while(opcion2!=4);

					break;
			case(3):
					flushCache();
					break;
			case(4):
									do
									{
										printf("Elija:\n1-Size Memoria\n2-Size pid\n3-Salir\n");
										scanf("%d",&opcion2);
										switch(opcion2)
										{
										case(1):
											sizeMemoria();
											break;
										case(2):
											printf("Ingrese identificador de proceso\n");
											uint32_t pid = 0;
											scanf("%d",&pid);
											sizePid(pid);
											break;
										case(3):
											break;
										default:
											printf("Ingrese opcion valida\n");
										}

									}while(opcion2!=3);
									break;
			default:
				printf("Ingrese un valor valido\n");
				break;
		}
	}	while(1);
}

////////////////////////MAIN////////////////////////
int main(int argc, char** argv)
{
	verificarParametros(argc);
	parametros = leerParametrosMemoria(argv);
	metadata = leerMetadataMemoria(parametros);

	inicializarEspacioMemoria();

	//printf("\nEl valor del puerto leido es: %d\n", metadata.puerto);
	//printf("El valor del cache por proceso es: %i\n", metadata.cache_x_proceso);
	//printf("El valor de las entradas cache es: %i\n", metadata.entradas_cache);
	//printf("El valor de los marcos es: %i\n", metadata.marcos);
	//printf("El valor del tamaño de marco es: %i\n", metadata.marcos_size);
	//printf("El valor del algoritmo de cache es: %s\n", metadata.reemplazo_cache);
	//printf("El valor del retardo de memoria es: %i\n", metadata.retardo_memoria);


	uint32_t retTec;
	uint32_t ret;

	remove(LOG_PATH);
	ptrLog = log_create(LOG_PATH, "MEMORIA", false, metadata.detallelog);

	logTabla = log_create("dumpTabla.txt", "Memoria.c", false, LOG_LEVEL_INFO);

	logCache = log_create("dumpCache.txt", "Memoria.c", false, LOG_LEVEL_INFO);

 	ret = pthread_create(&h_EscuchaConexiones, NULL, (void*) escucharConexiones, NULL );
  	if (ret < 0) {
  		//mostrarPorPantalla("Error al crear el hilo de escucha.\n El programa finalizara.");
  //	log_error(ptrLog, "Error al crear el hilo escucha. El programa finalizara.");
  		return EXIT_FAILURE;
  	}

	consolaMemoria();

	pthread_join(h_EscuchaConexiones, NULL);
	return 0;
}
