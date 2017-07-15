/*
 * Kernel.h
 *
 *  Created on: 7/4/2017
 *      Author: utnso
 */

#ifndef SRC_KERNEL_H_
#define SRC_KERNEL_H_

#define LOG_PATH "log.txt"


#include <Sockets/SocketsCliente.h>
#include <Sockets/socket.h>
#include <Sockets/StructsUtils.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>
#include <commons/log.h>
#include <commons/config.h>
#include <semaphore.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/config.h>
#include <parser/metadata_program.h>
#include <parser/parser.h>
#include <sys/inotify.h>
#include <sys/types.h>

#define EVENT_SIZE  ( sizeof (struct inotify_event) + 24 ) //ver bien el tamanio de caracteres que vamos a manejar
#define BUF_LEN     ( 1024 * EVENT_SIZE ) // 1024 eventos simultaneos

//SOCKET MEMORIA COMO GLOBAL
int socketMemoria;

int cantInstPrivilegiadas;

extern bool planificacion;

struct timeval tv;

t_list *listaCPU;

t_log *logFile;

int quantum;
int quantumSleep;

int cant_programas;

int gradoMultiprog;

uint32_t tamanioPagina;

sem_t progRunning;//para multiprocesamiento
sem_t progReady;//para multiporgramacion
sem_t mutexCPU;
sem_t CPULibres;
sem_t mutexConsola;
sem_t mutexLista;
sem_t mutex_lista_pcb;
sem_t mutexPid;
sem_t mutexNew;
sem_t cantNew;
sem_t mutex_GFT;
sem_t mutex_PFT;

pthread_mutex_t semPlanificacion;//para planificacion

int cantProgsEnSistema;

t_list *listaConsola;
t_queue * aSerBloqueado;
t_queue * aSerReplanificado;
t_list * aSerEliminado;

t_queue *colaNew;
t_list *colaReady;
t_list *colaExec;
t_queue *colaBlock;
t_queue *colaExit;

t_list *listaPCB;
t_list *listaProgramas;

pthread_t hiloPlanificacion;

pthread_t hiloConsolaKernel;

//MANEJO DE SEMAFOROS
t_list *listaSemaforos;

//VARIABLES COMPARTIDAS
t_list *listaVariablesCompartidas;

//VARIABLES DE INOTIFY
int file_descriptor;
int length_inotify;
int offset;
int watch_descriptor;
char buf[BUF_LEN];

typedef enum{
	FINALIZACION_CONSOLA,
	FINALIZACION_KERNEL,
	FINALIZACION_CPU,
	DEVOLUCION_CPU_CONSOLA,
	DEVOLUCION_CPU_KERNEL,
	GENERICO,
}enum_aviso;

typedef struct{
	t_PCB* pcb;
	uint32_t exitcode;
	int aviso;
}t_aSerEliminado;

typedef struct{
	char *nombreSemaforo;
	t_PCB* pcb;
}t_pcbBloqueado;

typedef struct{
	char *nombre;
	int valor;
}t_semaforo;

typedef struct{
	int PUERTO_PROG;
	int PUERTO_CPU;
	char* IP_MEMORIA;
	int PUERTO_MEMORIA;
	char* IP_FS;
	int PUERTO_FS;
	int QUANTUM;
	int QUANTUM_SLEEP;
	char* ALGORITMO;
	int GRADO_MULTIPROG;
	char** SEM_IDS;
	int* SEM_INIT;
	char** SHARED_VARS;
	int STACK_SIZE;
	char* IP_CPU;
	int backlog;
	t_log_level detallelog;
}archivoConfigKernel;

archivoConfigKernel *archC;
typedef struct{
	int pid;
	int cantAbrir;
	int cantCerrar;
	int cantLeer;
	int cantEscribir;
	int cantAlocar;
	int cantLiberar;
	int cantWait;
	int cantSignal;
	int cantBorrar;
	int cantMoverCursor;
}t_cantSyscalls;

t_list* tablaSyscalls;

typedef enum {
	FINALIZAR_CORRECTAMENTE,
	NO_RESERVO_RECURSOS,
	ARCHIVO_INEXISTENTE,
	LEER_SINPERMISOS,
	ESCRIBIR_SINPERMISOS,
	EXCEPCION_MEMORIA,
	DESCONEXION_CONSOLA,
	CONSOLA_FINALIZAR_PROGRAMA,
	MAYOR_MEMORIA,
	NO_ASIGNAR_PAGINAS,
	STACKOVERFLOW,
	KERNEL_FINALIZAR_PROGRAMA,
	//MENSAJE_ESCRIBIR_PAGINA,
	//MENSAJE_INICIAR_PROGRAMA,
	SIN_DEFINICION,
}exit_code;


typedef enum{
	OBTENER_LISTADO,
	CONSULTAR_ESTADO,
	OBTENER_TABLA_GLOBAL,
	MODIFICAR_GRADO_MULTIPROG,
	FINALIZAR_PROCESO,
	DETENER_PLANIFICACION,
	REANUDAR_PLANIFICACION,
}kernel_op;


enum t_estado{
	NEW,
	READY,
	RUNNING,
	WAITING,
	TERMINATED,
};

typedef struct{
	//uint32_t pid; asi sabemos donde estamos mandando
	int ocupado;
	int socketC;
	uint32_t PidEjecutando;
}t_cpu;

typedef struct{
	uint32_t pid;
	t_flags flags;
	uint32_t globalFD;
	uint32_t fd;
	uint32_t cursor;
}t_entradaPorPFT; //entrada por Process File Table

typedef struct{
	uint32_t globalFD;
	char* path;
	uint32_t open;
}t_entradaPorGFT;//entrada por Global File Table

typedef struct{
	uint32_t pidActual;
	uint32_t fdActual;
	uint32_t globalFDActual;
	char* pathAuxiliar;
}t_datosGlobalesArchivos;

t_datosGlobalesArchivos* auxiliarArchivo;

t_list* tablaPFT;
t_list* tablaGFT;

int socketFS;



t_PCB * crearPCB(char * codigo,int socket);

t_list * cargarLista(t_intructions*indiceCodigo, t_size cantInstrucciones);

int inicializarPrograma (int socketMemoria , t_PCB * pcb, int stack_size);

/*
 *  Ahora en planificador
void terminacionIrregular(t_PCB* pcb, uint32_t exitcode);

int crearHiloPlanificacion(int socketMemoria);

void algoritmo();

void planificarBloqueados();

void planificarNuevos(int socketMemoria);

void replanificarQuantum();

void planificarASerEliminados();

int planificar(int socketMemoria);


void pcbAColaExit(uint32_t pcbId);

void recorrerYMandar(t_cpu *cpuLista);

void envioPCBACpuDisponible(int socketCPU, t_PCB * unPCB);

*/

/*
 * ahora estan en consolaKernel
void obtenerListado(int pid);

void operacionKernel(uint32_t operacion, int pid, char* gradoMulti, uint32_t exit);

void consolaKernel();
*/

int actualizarPID();

void crearVariablesCompartidas(char **sharedVars);

t_op_varCompartida* obtenerVariable(t_op_varCompartida * varCompartida);

void crearSemaforos(char **semIds, char **semInit);

//int enviarCodigoAMemoria(int socketMemoria , int pid , int paginasCodigo, char * codigo); ahora en conexiones

void operacionConArchivos(int socketCPU, int tamanio, int operacion);

void operacionConVariableCompartida(int socketCPU, int tamanio, int operacion);

void operacionesConSemaforos(int socketCPU, int tamanio, int operacion);

void verificarModificacion();

int iniciarInotify();

/*

 * ahora en capaMemoria
void operacionesHeap(int socketCPU, int tamanio, int operacion, int socketMemoria);
*/

/*
 * ahora en capaFileSystem
 *
uint32_t abrirArchivo(uint32_t pid,char* path, t_flags permisos);

t_entradaPorPFT* crearEntradaEnPFT(uint32_t pid,t_flags flags,uint32_t fd,uint32_t globalfd);

int agregarEntradaGFT(char*path);

int crearEntradaGFT(char* path);

bool fdMenorGFT(t_entradaPorGFT *fd, t_entradaPorGFT *mayorfd);

bool fdMenorPFT(t_entradaPorPFT *fd, t_entradaPorPFT *mayorfd);

bool tieneElPath(t_entradaPorGFT* entrada);

bool esDelProceso(t_entradaPorPFT* entrada);

int nuevoGFD();

int agregarEntradaGFT(char*path);

int validarArchivo(char* path);

int crearArhivoFS(char* path);

int borrarArchivoFS(char* path);

int guardarDatosFS(char* path, uint16_t offset,uint16_t size,char* buffer);

char* obtenerDatos(char* path, uint16_t offset,uint16_t size);


t_entradaPorPFT* crearEntradaEnPFT(uint32_t pid,t_flags flags,uint32_t fd,uint32_t globalfd);

void destroyPFT(t_entradaPorPFT* self);

void destroyGFT(t_entradaPorGFT* self);

bool aBorrarPFT(t_entradaPorPFT* entrada);

bool aBorrarGFT(t_entradaPorGFT* entrada);

bool cerrarArchivo(uint32_t pid,uint32_t fd);

bool borrarArchivo(uint32_t pid, uint32_t fd);

void borrarEntradaPFT(uint32_t pid,uint32_t fd);

bool esDelFD(t_entradaPorPFT* entrada);

bool esDelGFD(t_entradaPorGFT* entrada);

bool moverCursor(uint32_t fd, uint32_t pid, uint32_t posicion);

bool escribirArchivo(uint32_t fd, uint32_t pid,char* info,uint32_t size);

bool leerArchivo(uint32_t pid, uint32_t fd, uint32_t size,char* buffer);

t_entradaPorGFT* entradaPorGFT();

t_entradaPorPFT* entradaPorPFT();

*/


#endif /* SRC_KERNEL_H_ */




