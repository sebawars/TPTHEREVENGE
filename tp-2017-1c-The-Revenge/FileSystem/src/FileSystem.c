/*
 ============================================================================
 Name        : FileSystem.c
 Author      : The Revenge
 Version     :
 Copyright   :
 Description : FS
 ============================================================================
 */

#include "FileSystem.h"
#include "configFS.h"

int main(int argc, char** argv) {
	char *ruta;

	verificarParametros(argc);
	ruta = leerParametros(argv);
	int server;
	archCFS = leerArchivoConfiguracion(ruta);
	remove(LOG_PATH);
	logFile = log_create(LOG_PATH, "Proceso File System", false, archCFS->DETALLE_LOG);
	metadata = leerArchivoMetadata(archCFS->PUNTO_MONTAJE);
	guardarBitArray();
	server = crearServer(archCFS->PUERTO, 1);
	socketKernel = handShake(server);
	//AbrirServidor(archCFS->PUERTO);
	while (1) {
		//t_operacionKernel* operacion;
		header_t header;
		header.type=0;
		int cant;
		log_debug(logFile,"Esperando operacion de Kernel");
		cant = recv(socketKernel, &header, sizeof(header_t),0);
		t_validarArchivo* validar;
		t_crearArchivo* crear;
		t_borrarAchivo* borrar;
		t_guardarDatos* guardar;
		t_obtenerDatos* obtener;
		char* buffer = malloc(header.length);
		switch (header.type) {

	case VALIDAR_ARCHIVO:
		cant=recv(socketKernel, buffer, header.length, 0);
		validar = validarArchivoDesSerializer(buffer);
		validarArchivo(validar->path);
		free(buffer);
		break;

	case CREAR_ARCHIVO:
		cant=recv(socketKernel, buffer, header.length, 0);
		crear = validarArchivoDesSerializer(buffer);
		crearArchivo(crear->path);
		free(buffer);
		break;

	case BORRAR_ARCHIVO:
		cant=recv(socketKernel, buffer, header.length, 0);
		borrar = validarArchivoDesSerializer(buffer);
		borrarArchivo(borrar->path);
		free(buffer);
		break;

	case OBTENER_DATOS:
		cant=recv(socketKernel, buffer, header.length, 0);
		obtener = obtenerDatosDesSerializer(buffer);
		obtenerDatos(obtener->path, obtener->offset, obtener->size);
		free(buffer);
		break;

	case GUARDAR_DATOS:
		cant=recv(socketKernel, buffer, header.length, 0);
		guardar = guardarDatosDesSerializer(buffer);
		guardarDatos(guardar->path, guardar->offset, guardar->size,
				guardar->buffer);
		free(buffer);
		break;

	default:
		log_error(logFile,"Protocolo Inesperado se cierra conexion con Kernel");
		return 0;
		};

	};
	return 0;
}

//establece la comunicacion con el kernel
int handShake(int server) {
	log_debug(logFile,"handhsake con:%d",server);
	int socketAux;
	do {
		socketAux = sockets_accept(server);
		printf("esperando Kernel en socket %d",server);
	} while (aceptarConexion(socketAux, HANDSHAKE_KERNEL) != 0);
	enviarHandshake(socketAux, HANDSHAKE_FILESYSTEM);
	log_info(logFile,"Se conecto con kernel en el puerto: %d",socketAux);
	return socketAux;
}

//setea la variable global bitArray
void guardarBitArray() {
	int fdBitMap = abrirBitmap();
	if(fdBitMap>0){
		log_info(logFile,"se abrio correctamente el Bitmap");

		//bitArray = (t_bitarray*)malloc((sizeof(t_bitarray)) + ((metadata->CANT_BLOQUES)/8));
//		printf("cantidad de bloques: %d \n", sizeof(t_bitarray) + metadata->CANT_BLOQUES);
		int b;
		map = mmap(NULL, (metadata->CANT_BLOQUES) / 8, PROT_READ | PROT_WRITE,MAP_SHARED, fdBitMap, 0);
		if (map == MAP_FAILED)
			log_error(logFile,"::error al crear mmap");
		bitArray = bitarray_create_with_mode(map, (metadata->CANT_BLOQUES) / 8,MSB_FIRST);
		b = bitarray_get_max_bit(bitArray);
		b=strlen(map);
	}else{
		log_info(logFile,"no existe archivo bitmap, se crea");
		fdBitMap=crearBitmap();
//		printf("cantidad de bloques: %d \n", sizeof(t_bitarray) + metadata->CANT_BLOQUES);
		int b;
		map = mmap(NULL, (metadata->CANT_BLOQUES) / 8, PROT_READ | PROT_WRITE,MAP_SHARED, fdBitMap, 0);
		if (map == MAP_FAILED)
			log_error(logFile,"error al crear mmap");
		bitArray = bitarray_create_with_mode(map, (metadata->CANT_BLOQUES) / 8,MSB_FIRST);
		cleanBitArray();
		b = bitarray_get_max_bit(bitArray);
		b=strlen(map);

	}

}

//limpia el bitarray
void cleanBitArray(){
	int i=0;
	while(i<bitarray_get_max_bit(bitArray)){
		bitarray_clean_bit(bitArray,i);
		i++;
	}
	log_info(logFile,"se limpio bitArray de tamanio=%d",i+1);
}

//abro archivo bitmap.bin
int abrirBitmap() {
	log_debug(logFile,"abriendo archivo Bitmap.bin");
	int escritos;
	char* rutaAux = malloc(300);

	strcpy(rutaAux, archCFS->PUNTO_MONTAJE);
	strcat(rutaAux, "Metadata/Bitmap.bin");
	char* bitmap = malloc((metadata->CANT_BLOQUES) / 8);
	memset(bitmap, '0', (metadata->CANT_BLOQUES) / 8);
	int fdBitmap = open(rutaAux, O_RDWR);	//abro bitmap.bin
	free(rutaAux);
	return fdBitmap;
}

//crea el bitmap.bin
int crearBitmap(){
	log_debug(logFile,"creando archivo Bitmap.bin");
	int escritos;
	char* rutaAux = malloc(300);

	strcpy(rutaAux, archCFS->PUNTO_MONTAJE);
	strcat(rutaAux, "Metadata/Bitmap.bin");
	char* bitmap = malloc((metadata->CANT_BLOQUES) / 8);
	memset(bitmap, '0', (metadata->CANT_BLOQUES) / 8);
	int fdBitmap;
	fdBitmap = creat(rutaAux, 0777);	//creo bitmap al no estar creado
	close(fdBitmap);
	fdBitmap = open(rutaAux, O_RDWR);	//obtengo fd del bitmap
	if (fdBitmap == -1)
		log_error(logFile,"-> no se pudo crear Bitmap");
	escritos = write(fdBitmap, bitmap, (metadata->CANT_BLOQUES) / 8);//seteo el bitmap creado
	log_info(logFile,"-> se creo correctamente el bitmap");
	free(rutaAux);
	return fdBitmap;
}
//verifica que exista el archivo pasado por parametro y envia el resultado al kernel
int validarArchivo(char* ruta) {
	log_debug(logFile,"validando archivo: %s",ruta);
	header_t header;
	header.type = RESULTADO;
	header.length = sizeof(2);
	int nbytes;
	char* path=malloc(strlen(ruta));
	path=string_duplicate(archCFS->PUNTO_MONTAJE);
	string_append(&path,"Archivos/");
	string_append(&path,string_substring_from(ruta,1));
	char* aMandar = malloc(sizeof(t_results));
	FILE* archivo = fopen(path, "r");
	t_validarArchivo* resultado=malloc(sizeof(t_validarArchivo));
	if (archivo == NULL) {
		log_info(logFile,"no existe el archivo, se envia mensaje a Kernel");
		resultado->resultado=NO_EXISTE;
		resultado->path=strdup("");
		aMandar=validarArchivoSerializer(resultado,&header.length);
		nbytes = sockets_send(socketKernel, &header, aMandar);
		return-1;
	} else {
		log_info(logFile,"existe el archivo, se envia mensaje a Kernel");
		resultado->resultado=EXISTE;
		resultado->path=strdup("");
		aMandar=validarArchivoSerializer(resultado,&header.length);
		nbytes = sockets_send(socketKernel,&header, aMandar);
	}
	fclose(archivo);
	free(aMandar);
	free(resultado);
	return 0;
}

//crea archivo y le asigna un bloque de memoria y envia el resultado al kernel
void crearArchivo(char* ruta) {
	log_debug(logFile,"creando archivo: %s",ruta);
	header_t header;
	header.type = RESULTADO;
	header.length = sizeof(t_results);
	int nbytes;
	int bloque;
	char* path=malloc(strlen(ruta));
	path=string_duplicate(archCFS->PUNTO_MONTAJE);
	string_append(&path,"Archivos/");
	string_append(&path,string_substring_from(ruta,1));
	char* aMandar;
	char** desmontado;
	char* aux;
	aux=string_reverse(path);
	desmontado=string_n_split(aux,2,"/");
	aux=string_reverse(desmontado[1]);
	//printf("\n desmontado:%s\n",aux);
	struct stat st={0};
	if(stat(aux,&st)==-1){
		log_info(logFile,"No existe el direcctorio %s que contiene al archivo y se crea",aux);
		//printf("no existe, se crea");
		_mkdir(aux);
	}
	FILE* archivo = fopen(path, "w+");
	t_crearArchivo* resultado=malloc(sizeof(t_crearArchivo));
	if (archivo != NULL) {
		fputs("TAMANIO=0",archivo);
		fputc('\n',archivo);
		fputs("BLOQUES=[]",archivo);
		fclose(archivo);
		bloque=bloqueVacio();
		if(bloque<0){
			log_info(logFile,"no se pudo crear el archivo %s por no haber bloques disponibles",ruta);
			resultado->resultado=ERROR_CREACION;
			resultado->path=strdup("");
			remove(path);
			aMandar=validarArchivoSerializer(resultado,&header.length);
			nbytes = sockets_send(socketKernel, &header, aMandar);

		}else{
			log_info(logFile,"se crea el archivo %s correctamente y se le asigna el bloque %d",ruta,bloque);
			asignarBloque(path,bloque);
			resultado->resultado=EXITO;
			resultado->path=strdup("");
			aMandar=validarArchivoSerializer(resultado,&header.length);
			nbytes = sockets_send(socketKernel, &header, aMandar);
		}
	} else {
		log_info(logFile,"no se pudo crear el archivo %s por la funcion fopen",ruta);
		resultado->resultado=ERROR_CREACION;
		resultado->path=strdup("");
		aMandar=validarArchivoSerializer(resultado,&header.length);
		nbytes = sockets_send(socketKernel, &header, aMandar);
	}
	//free(resultado);
	free(aMandar);
}

//elimina el archivo pasado por parametro y liberar los bloques (los borra)
void borrarArchivo(char* ruta) {
	log_debug(logFile,"borrando archivo: %s",ruta);
	header_t header;
	header.type = RESULTADO;
	header.length = sizeof(t_results);
	int nbytes;
	char* aMandar;
	char* path=malloc(strlen(ruta));
	path=string_duplicate(archCFS->PUNTO_MONTAJE);
	string_append(&path,"Archivos/");
	string_append(&path,string_substring_from(ruta,1));
	t_borrarAchivo* resultado=malloc(sizeof(t_borrarAchivo));
	t_infoArchivo* info=leerInfoArchivo(path);
	if (remove(path)>=0) {
		log_info(logFile,"se borro el archivo %s correctamente",ruta);
		liberarBloques(info);
		resultado->resultado=EXITO;
		resultado->path=strdup("");
		aMandar=validarArchivoSerializer(resultado,&header.length);
		nbytes = sockets_send(socketKernel, &header, aMandar);
	} else {
		log_info(logFile,"no se pudo borrar el archivo %s",ruta);
		resultado->resultado=ERROR_BORRAR;
						resultado->path=strdup("");
						aMandar=validarArchivoSerializer(resultado,&header.length);
						nbytes = sockets_send(socketKernel, &header, aMandar);
	}
	free(aMandar);
//	free(resultado);
	free(info);
}


//envia los datos que se desean al kernel
void obtenerDatos(char*ruta, uint16_t offset, uint16_t size) {
	log_debug(logFile,"obteniendo datos de tamanio %d con offset %d del archivo: %s ",size,offset,ruta);
	uint16_t tamanio=size;
	char*buffer = string_new();
	header_t header;
	header.type = RESULTADO;
	char* path=malloc(strlen(ruta));
	path=string_duplicate(archCFS->PUNTO_MONTAJE);
	string_append(&path,"Archivos/");
	string_append(&path,string_substring_from(ruta,1));
	char* aMandar;
	int nbytes;
	int offsetBin=(offset%metadata->TAM_BLOQUES);
	int bloqueALeer=(offset/metadata->TAM_BLOQUES);
	int faltante;
	t_resultado* resultado=malloc(sizeof(t_resultado));
	if(validarArchivoSinEnvio(path)<0){
		log_info(logFile,"no existe el archivo %s de donde se quiere obtener datos",ruta);
		resultado->resultado=NO_EXISTE;
		resultado->path=strdup("");
		aMandar=validarArchivoSerializer(resultado,&header.length);
		nbytes = sockets_send(socketKernel, &header, aMandar);
	}else{
		t_infoArchivo* info=leerInfoArchivo(path);
		do{
			log_info(logFile,"se encontro el archivo %s y se procede a leer los datos del mismo",ruta);

			faltante=leerDatosBin(atoi(info->bloques[bloqueALeer]),offsetBin,&tamanio,&buffer);
			//if(faltante>0)
			bloqueALeer++;
			offsetBin=0;
		}while(faltante>0);
		log_info(logFile,"Se leyo correctamente >%s< del archivo %s",buffer,ruta);
		resultado->resultado=EXITO;
		resultado->path=buffer;
		aMandar= validarArchivoSerializer(resultado,&header.length);
		nbytes = sockets_send(socketKernel, &header, aMandar);
		free(aMandar);
//		free(buffer);
		free(resultado);
	}
}

//almacena el buffer en los bloques correspondientes, si no le alcanza los bloques se le asigna otro
int guardarDatos(char*ruta, uint16_t offset, uint16_t size, char* buffer) {
	log_debug(logFile,"guardando datos de tamanio %d con offset %d del archivo: %s ",size,offset,ruta);
	header_t header;
	header.type = RESULTADO;
	header.length = sizeof(t_results);
	int nbytes;
	char* path=malloc(strlen(ruta));
	path=string_duplicate(archCFS->PUNTO_MONTAJE);
	string_append(&path,"Archivos/");
	string_append(&path,string_substring_from(ruta,1));
	char* aMandar = malloc(sizeof(t_results));
	t_validarArchivo* resultado=malloc(sizeof(t_validarArchivo));
	//FILE* archivo = fopen(path, "w");
	int offsetBin=(offset%metadata->TAM_BLOQUES);
	int bloqueAEscribir=(offset/metadata->TAM_BLOQUES);
	int faltante;
	if(validarArchivoSinEnvio(path)<0){
		log_info(logFile,"no existe el archivo %s de donde se quiere obtener datos, se envia respuesta a Kernel",ruta);
		resultado->resultado=NO_EXISTE;
		resultado->path=strdup("");
		aMandar=validarArchivoSerializer(resultado,&header.length);
		nbytes = sockets_send(socketKernel, &header, aMandar);
		log_error(logFile,"no se escontro el archivo:%s",path);
		free(aMandar);
		return -1;
		}
	do{
		log_info(logFile,"se encontro el archivo %s y se procede a escribir los datos en el mismo",ruta);

		t_infoArchivo* info=leerInfoArchivo(path);

		faltante=escribirDatosBin(atoi(info->bloques[bloqueAEscribir]),offsetBin,&size,&buffer);
		bloqueAEscribir++;
		offsetBin=0;
		if((info->bloques[bloqueAEscribir]==NULL)&&(faltante>0)){
			log_debug(logFile,"se le asigna nuevo bloque al archivo %s ",ruta);
			int nuevoBloque=bloqueVacio();
				if(nuevoBloque<0){
					log_error(logFile,"No existen mas bloques disponibles para el archivo %s",ruta);
					resultado->resultado=NO_HAY_ESPACIO;
					resultado->path=strdup("");
					aMandar=validarArchivoSerializer(resultado,&header.length);
					nbytes = sockets_send(socketKernel, &header, aMandar);
					free(aMandar);
					return -2;
				}else{
					log_info(logFile,"se encontro libre el bloque %d",nuevoBloque);
					asignarBloque(path,nuevoBloque);
					offsetBin=0;
					log_info(logFile,"se le asigno correctamente el bloque %d al archivo %s",nuevoBloque,ruta);
				}
		}
	}while(faltante>0);
	aumentarTamanio(path);
	resultado->resultado=EXITO;
	resultado->path=strdup("");
	aMandar=validarArchivoSerializer(resultado,&header.length);
	log_info(logFile,"se guardo correctamente los datos en %s",path);
	nbytes = sockets_send(socketKernel, &header, aMandar);
	free(aMandar);
	return 1;

}

//crea el ssocket escucha
int crearServer(int port, int backlog) {
	int sockFd = sockets_getSocket();

	if (sockets_bind(sockFd, port) != 0) {
		close(sockFd);
		return -1;
	}

	if (listen(sockFd, backlog) == -1) {
		close(sockFd);
		return -2;
	}
	return sockFd;
}

// devuelte un t_Archivo seteado con las ṕropiedades del archivo pasado por parametro
t_infoArchivo* leerInfoArchivo(char* path) {
	t_infoArchivo* info = malloc(sizeof(t_infoArchivo));
	t_config* confArch = config_create(path);
	if (confArch == NULL)
		log_error(logFile,"no se puede encontrar el archivo de configuración del archivo %s",path);

	if (config_has_property(confArch, "TAMANIO"))
		info->tamanio = config_get_int_value(confArch, "TAMANIO");
	else
		log_error(logFile,
				"El archivo de configuracion del Archivo %s no contiene la clave TAMANIO",path);

	if (config_has_property(confArch, "BLOQUES"))
		info->bloques = config_get_array_value(confArch, "BLOQUES");
	else
		log_error(logFile,
				"El archivo de configuracion del Archivo %s no contiene la clave BLOQUES",path);
	return info;
}

//hace una copia de un archivo y la pisa con el nuevo bloque asignado
void asignarBloque(char* path,uint16_t bloque) {
	log_debug(logFile,"asignando bloque %d al archivo %s",bloque,path);
	t_infoArchivo* info=leerInfoArchivo(path);
	FILE* archivo;
	int i=0;
	//char* bloques= string_from_format("BLOQUES=");
	remove(path);
	archivo=fopen(path,"w+");
	char* tamanio= string_from_format("TAMANIO=");
	string_append(&tamanio, string_itoa(info->tamanio));
	fputs(tamanio,archivo);
	fputc('\n',archivo);
	fputs("BLOQUES=[",archivo);
	while(info->bloques[i]!=NULL){
		fputs(info->bloques[i],archivo);
		fputc(',',archivo);
		i++;
	}
//	if(bloque>=0){
	bitarray_set_bit(bitArray,bloque);
	crearBloque(bloque);
	fputs(string_itoa(bloque),archivo);
	fputc(']',archivo);
	fputc('\n',archivo);
//	}else{
//		fputc(']',archivo);
//		fputc('\n',archivo);
//	}
	fclose(archivo);

}

void aumentarTamanio(char* path){
	log_debug(logFile,"se actualiza tamanio del archivo %s",path);
	t_infoArchivo* info=leerInfoArchivo(path);
	FILE* archivo;
	int i=0;
	//char* bloques= string_from_format("BLOQUES=");
	remove(path);
	archivo=fopen(path,"w+");
	int tam=0;
	char* pathAux;
	while(info->bloques[i]!=NULL){
			pathAux=binAPath(atoi(info->bloques[i]));
			tam+=tamanioArchivo(pathAux);
			i++;
		}
	i=0;
	char* tamanio= string_from_format("TAMANIO=");
	string_append(&tamanio, string_itoa(tam));
	fputs(tamanio,archivo);
	fputc('\n',archivo);
	fputs("BLOQUES=[",archivo);
	fputs(info->bloques[i],archivo);
	i++;
	while(info->bloques[i]!=NULL){
		fputc(',',archivo);
		fputs(info->bloques[i],archivo);
		i++;
	};
	fputc(']',archivo);
	fputc('\n',archivo);
	fclose(archivo);
}

//Retorna el tamanio de un archivo en bytes
uint8_t tamanioArchivo(char*path) {
	FILE* arch;
	int tamanio;
	arch = fopen(path, "rb+"); // abro el archivo de solo lectura.
	fseek(arch, 0L, SEEK_END);            // me ubico en el final del archivo.
	tamanio = ftell(arch);                     // obtengo su tamanio en BYTES.
	fclose(arch);                               // cierro el archivo.
	return tamanio;
}

///Retorna la posicion mas proxima, en el bitarray, que este vacio
int bloqueVacio() {
	int i = 0;
	while (i <= bitarray_get_max_bit(bitArray)&&(metadata->CANT_BLOQUES>i)) {
	if (!bitarray_test_bit(bitArray, i))
		return i;
	i++;
	}
	return -1;
}

//crea el .bin del bloque
void crearBloque(uint16_t bloque){
	char* path=binAPath(bloque);
	char** desmontado;
	char* aux;
	aux=string_reverse(path);
	desmontado=string_n_split(aux,2,"/");
	aux=string_reverse(desmontado[1]);
//	printf("\n desmontado:%s\n",aux);
	struct stat st={0};
	if(stat(aux,&st)==-1){
//		printf("no existe, se crea");
		_mkdir(aux);
	}
	int archivo=creat(path, 0777);	//creo .bin
	//ftruncate(archivo,64);
	close(archivo);
	free(path);
}

//elimina los .bin asociados a los bloques de un archivo y hace clean al bit asociado al .bin
void liberarBloques(t_infoArchivo* archivo){
	int i=0;
	char* path;
	int bloque;
	while(archivo->bloques[i]!=NULL){
		path=string_new();
		bloque=atoi(archivo->bloques[i]);
		string_append(&path,archCFS->PUNTO_MONTAJE);
		string_append(&path,"Bloques/");
		string_append(&path,archivo->bloques[i]);
		string_append(&path,".bin");
		remove(path);
		bitarray_clean_bit(bitArray,bloque);
		i++;
		free(path);
	}
}

//retorna el bloque como path
char* binAPath(int bloque){
	char* path=string_new();
		string_append(&path,archCFS->PUNTO_MONTAJE);
		string_append(&path,"Bloques/");
		string_append(&path,string_itoa(bloque));
		string_append(&path,".bin");
		return path;
}

//lee los datos de un binario y devuelve el tamaño de lo que falta leer debido a que termino el bloque
int leerDatosBin(int bin, uint16_t offset,uint16_t* size,char** buffer){
	char* path=binAPath(bin);
	char* leido=malloc(*size+1);
	int faltaLeer;
	int cant=*size;
	FILE* archivo =fopen(path,"r");
	rewind(archivo);
	if(offset>0)
		fseek(archivo,offset,SEEK_SET);

	int i, c;
    i = 0;
	while(!feof(archivo)){
		c = fgetc(archivo);
		leido[i] = c;
		i++;
	}
	leido[i-1]='\0';
	faltaLeer=cant-i+1;
//	if (strlen(leido)==0) //se comenta porque se supone que siemrpe van a pedir datos de tamaño coherente en una posicion coherente
//		return-1;
	string_append(&(*buffer),leido);
	*size=faltaLeer;
	log_info(logFile,"se leyo >%s< del bloque %d.bin",leido,bin);
	fclose(archivo);
	free(leido);
	return faltaLeer;
}

//escribe caracter a caracter en un archivo, modificando el buffer con lo que falta leer y retornando el tam de lo que falta leer
int escribirDatosBin(int bin, uint16_t offset,uint16_t* size,char** buffer){
	char* path=binAPath(bin);
	char* bufferAux=*buffer;
	int resto=*size,disponible;
	disponible=tamanioArchivo(binAPath(bin));
	int espacio=metadata->TAM_BLOQUES-offset;
	FILE* archivo=fopen(path,"rb+");
	rewind(archivo);		//vuelvo al principio del archivo
	if(string_length(bufferAux)<resto){
		char* basura=string_repeat('$',resto-string_length(bufferAux));
		string_append(&bufferAux,basura);
		free(basura);
	}
	if(offset>0)
		fseek(archivo,offset,SEEK_SET);
	int i=0;
	while((espacio>0)&&(resto>0)){
		//fseek(archivo,1,SEEK_CUR);
//		if(bufferAux[i]!='\0')
//			fputc('c',archivo);
//		else
			fputc(bufferAux[i],archivo);
		espacio--;
		resto--;
		i++;

	}

 	*buffer=string_substring_from(bufferAux,i);
 	char* escrito=string_substring_until(bufferAux,i);
//	if(string_is_empty(bufferAux)&&(resto>0)){
//		char*  basura= string_repeat('t', resto);
//		string_append(&(bufferAux),basura);
//		*buffer=bufferAux;
		//return 1;
//	}
	*size=resto;
//	*buffer=bufferAux;
	log_info(logFile,"se escribe >%s< en %d.bin",escrito,bin);
	fclose(archivo);
	free(path);
	return resto;

}

//crear directorio cosas raras que funcionan
void _mkdir(char* dir){
	char tmp[256];
	char *p=NULL;
	size_t len;
	snprintf(tmp,sizeof(tmp),"%s",dir);
	len=strlen(tmp);
	if(tmp[len-1]=='/')
		tmp[len-1]=0;
	for(p=tmp+1; *p;p++)
		if(*p=='/'){
			*p=0;
			mkdir(tmp,0777);
			*p='/';
		}
	mkdir(tmp,0777);
	log_info(logFile,"se creo el directorio %s", dir);
}

int validarArchivoSinEnvio(char* path){
	header_t header;
		header.type = RESULTADO;
		header.length = sizeof(2);
		int nbytes;
		char* aMandar = malloc(sizeof(t_results));
		FILE* archivo = fopen(path, "r");
		t_validarArchivo* resultado;
		if (archivo == NULL) {
			return-1;
		}
		fclose(archivo);
		free(aMandar);
	//	free(resultado);
		return 0;
}
