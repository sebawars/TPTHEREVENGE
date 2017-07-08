#ifndef HEADERS_SOCKET_H_
#define HEADERS_SOCKET_H_
#define PORT 9034

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <commons/collections/queue.h>
#include "StructsUtils.h"

enum enum_protocolo {
	HANDSHAKE_CONSOLA,
	HANDSHAKE_KERNEL,
	HANDSHAKE_CPU,
	HANDSHAKE_FILESYSTEM,
	HANDSHAKE_MEMORIA,	//Kernel>>>> MEMORIA
	EXECUTE_PCB,	//Kernel> CPU
	ASIGNAR_PAGINAS, // KERNEL > MEMORIA
	ESCRIBIR_PAGINA,
	ENVIO_PID, //Kernel > Hilo programa
	ENVIO_PCB, //CPU > Kernel
	LEER_VAR_COMPARTIDA, //CPU > Kernel
	ASIGNAR_VAR_COMPARTIDA, //CPU > Kernel
	ESCRIBIR_ARCHIVO, //CPU >> Kernel
	LEER_ARCHIVO, //CPU >> Kernel
	ABRIR_ARCHIVO, //CPU >> Kernel
	CERRAR_ARCHIVO, //CPU >> Kernel
	MOVER_CURSOR, //CPU >> Kernel
	QUANTUM_TERMINADO, //CPU >> Kernel
	FINALIZAR_SIGUSR1, //CPU >> Kernel
	ERROR_ESCRIBIR, // Kernel >> CPU
	ERROR_LECTURA, // Kernel >> CPU
	ERROR_MEMORIA, // CPU >> Kernel
	ERROR_APERTURA, // Kernel >> CPU
	INICIAR_PROGRAMA,	// Hilo Programa>Kernel   KERNEL > MEMORIA
	FINALIZAR_PROGRAMA, //Consola > Kernel   KERNEL > MEMORIA
	ERROR_STACKOVERFLOW, //CPU >> KERNEL
	WAIT, //CPU >> Kernel
	SIGNAL, //CPU >> Kernel
	NOTHING, //CPU >> Kernel
	CPU_RUNNING,
	EXIT, //Cpu >> Kernel
	ALOCAR, //Cpu >> Kernel
	LIBERAR, ////Cpu >> Kernel
	DESCONECTAR_CONSOLA,
	TAMANIO_STACK, //Kernel>CPU
	CAMBIO_PROCESO_ACTIVO, //Cpu>Memoria
	LEER, //Cpu> Memoria
	TAMANIO_PAGINA, //Memoria > Cpu
	ERROR,
	ERROR_HEAP,
	CPU_SIGUSR1,
	SUCCESS,
	BYTES_LEIDOS, //Memoria > CPU
	VALIDAR_ARCHIVO,	//Kernel>>>FS
	CREAR_ARCHIVO,	//Kernel>>>FS
	BORRAR_ARCHIVO,	//Kernel>>>FS	//CPU >> Kernel
	OBTENER_DATOS,	//Kernel>>>FS
	GUARDAR_DATOS,	//Kernel>>>FS
	RESULTADO,	//FS>>>Kernel
	IMPRIMIR_ARCHIVO,
	ALLOC_MAYOR_TAM_PAGINA,
	NO_PUDO_ASIGNAR_PAGINAS,
};
//OPS

struct timeval tv;

//fd_set fds_clientes,master; conjunto maestro de descriptores de fichero

int AbrirConexion(char* ip, int puerto);

int AbrirSocketServidor(int puerto);

int socket_addNewConection(int listener, fd_set *master, int *fdmax);

void socket_closeConection(int socket, fd_set *master);

int sockets_bind(int sockfd, int port);

int socket_createServer(int port, int backlog);

int sockets_send(int sockfd, header_t *header, char *data);

int sockets_getSocket(void);

int sockets_accept(int sockfd);

int enviarHandshake(int socket, int handshake);

int enviarHeader(int socket, int tipo);

char* recibirDatos(int socket, uint32_t ** op, uint32_t ** id);

int enviarDatos(int socket, char* datos, uint32_t tamanio, uint32_t op,
		uint32_t id);

int finalizarConexion(int socket);

int escribir(int socket, char* buffer, int longitud);

int leer(int socket, char* buffer, int longitud);

#endif /* HEADERS_SOCKET_H_ */

