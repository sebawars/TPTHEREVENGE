/*
 * socket.c
 *
 *  Created on: 4/4/2017
 *      Author: utnso
 */
#include "socket.h"

void socket_closeConection(int socket, fd_set *master) {
	printf("selectserver: socket %d hung up\n", socket);
	close(socket); // bye!
	FD_CLR(socket, master); // eliminar del conjunto maestro
}


int sockets_getSocket(void) {
	int activado = 1;
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(int)) == -1)
	{
		close(sockfd);
		return -1;
	}

	return sockfd;
}

int sockets_bind(int sockfd, int port) {
	struct sockaddr_in my_addr;

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);

	int h= bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr));
	return h;
}

int socket_createServer(int port, int backlog)
{
	int sockFd= sockets_getSocket();

	if(sockets_bind(sockFd,port)!=0)
	{
		close(sockFd);
		return -1;
	}

	if (listen(sockFd,backlog)==-1){
		close(sockFd);
		return -2;
	}
	return sockFd;
}

/**
 * @NAME: sockets_accept
 * @DESC: Acepta una conexion al socket enviado por parámetro y retorna el nuevo descriptor de socket.
 * Retorna -1 en caso de error.
 */
int sockets_accept(int sockfd) {
	struct sockaddr_in their_addr;
	int sin_size = sizeof(struct sockaddr_in);

	return accept(sockfd, (struct sockaddr *) &their_addr,
			(socklen_t *) &sin_size);
}

int enviarHandshake(int socket, int handshake)
{
	header_t header;
	header.type = handshake;
	header.length = 0;

	return sockets_send(socket, &header, '\0');
}

int enviarHeader(int socket, int tipo)
{
	header_t header;
	header.type = tipo;
	header.length = 0;

	return sockets_send(socket, &header, '\0');
}


/**
 * @NAME: sockets_send
 * @DESC: Serializa el header y el string de datos y lo envía al socket pasado por parámetro.
 * Retorna la cantidad de bytes enviados.
 */
int sockets_send(int sockfd, header_t *header, char *data) {
	int bytesEnviados, offset = 0, tmp_len = 0;
	char* packet = malloc(sizeof(header_t) + header->length);

	memcpy(packet, &header->type, tmp_len = sizeof(int8_t));
	offset = tmp_len;

	memcpy(packet + offset, &header->length, tmp_len = sizeof(uint32_t));
	offset += tmp_len;

	memcpy(packet + offset, data, tmp_len = header->length);
	offset += tmp_len;

	bytesEnviados = send(sockfd, packet, offset, MSG_NOSIGNAL);
	free(packet);

	return bytesEnviados;
}

char* recibirDatos(int socket, uint32_t ** op, uint32_t ** id) {
	int leido = 0;
	uint32_t longitud = 3 * sizeof(uint32_t);
	char* buffer = malloc(longitud);
	/*
	 * Comprobacion de que los parametros de entrada son correctos
	 */
	if ((socket < 0) || (buffer == NULL))
		return "ERROR";

	/* HEADER
	 * Se reciben primero los datos necesarios que dan informacion
	 * sobre el verdadero buffer a recibir
	 */
	if ((leido = leer(socket, buffer, longitud)) > 0) {
		if (op == NULL) {
			memcpy(id, buffer + sizeof(uint32_t), sizeof(uint32_t));
			memcpy(&longitud, buffer + (2 * sizeof(uint32_t)), sizeof(uint32_t));
		} else {
			memcpy(op, buffer, sizeof(uint32_t));
			memcpy(id, buffer + sizeof(uint32_t), sizeof(uint32_t));
			memcpy(&longitud, buffer + (2 * sizeof(uint32_t)), sizeof(uint32_t));
		}
		free(buffer);
		buffer = malloc(longitud);
	} else {
		free(buffer);
		return "ERROR";
	}

	if ((leido = leer(socket, buffer, longitud)) < 0) {
		free(buffer);
		return "ERROR";
	}

	if (leido != longitud)
		printf("No se han podido leer todos los datos del socket!!");

	return buffer;
}


/*
 * Escribe dato en el socket cliente. Devuelve numero de bytes escritos,
 * o -1 si hay error.
 */
int enviarDatos(int socket, char* datos, uint32_t tamanio, uint32_t op, uint32_t id) {
	int escrito = 0;

	/*
	 * Comprobacion de los parametros de entrada
	 */
	if ((socket == -1) || (tamanio < 1))
		return -1;

	/* HEADER
	 * Se envian primero los datos necesarios que dan informacion
	 * sobre el verdadero buffer a enviar
	 */
	char* buffer = malloc(tamanio + (3 * sizeof(uint32_t))); //1B de op y 4B de long
	memcpy(buffer, &op, sizeof(uint32_t));
	memcpy(buffer + sizeof(uint32_t), &id, sizeof(uint32_t));
	memcpy(buffer + (2 * sizeof(uint32_t)), &tamanio, sizeof(uint32_t));
	memcpy(buffer + (3 * sizeof(uint32_t)), datos, tamanio);

	/*
	 * Envio de Buffer [tamanio B]
	 */
	escrito = escribir(socket, buffer, tamanio + (3 * sizeof(uint32_t)));
	free(buffer);

	/*
	 * Devolvemos el total de caracteres leidos
	 */
	return escrito;
}

int finalizarConexion(int socket) {
	close(socket);
	return 0;
}

int escribir(int socket, char* buffer, int longitud) {
	int escrito = 0, aux = 0;
	/*
	 * Bucle hasta que hayamos escrito todos los caracteres que nos han
	 * indicado.
	 */
	while (escrito < longitud && escrito != -1) {
		aux = send(socket, buffer + escrito, longitud - escrito, 0);
		if (aux > 0) {
			/*
			 * Si hemos conseguido escribir caracteres, se actualiza la
			 * variable Escrito
			 */
			escrito = escrito + aux;
		} else {
			/*
			 * Si se ha cerrado el socket, devolvemos el numero de caracteres
			 * leidos.
			 * Si ha habido error, devolvemos -1
			 */
			if (aux == 0)
				return escrito;
			else
				switch (errno) {
				case EINTR:
				case EAGAIN:
					usleep(100);
					break;
				default:
					escrito = -1;
				}
		}
	}
	return escrito;
}

int leer(int socket, char* buffer, int longitud) {
	int leido = 0, aux = 0;
	/*
	 * Se recibe el buffer con los datos
	 * Mientras no hayamos leido todos los datos solicitados
	 */
	while (leido < longitud && leido != -1) {
		aux = recv(socket, buffer + leido, longitud - leido, 0);
		if (aux > 0) {
			leido = leido + aux;
		} else {
			if (aux == 0)
				break;
			if (aux == -1) {
				switch (errno) {
				case EINTR:
				case EAGAIN:
					usleep(100);
					break;
				default:
					leido = -1;
				}
			}
		}
	}
	return leido;
}

int AbrirSocketServidor(int puerto) {

	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = INADDR_ANY;
	direccionServidor.sin_port = htons(puerto);

	int servidor = socket(AF_INET, SOCK_STREAM, 0);

	int activado = 1;
	setsockopt(servidor, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado));

	if (bind(servidor, (void*) &direccionServidor, sizeof(direccionServidor))
			!= 0) {
		perror("Falló el bind");
		return 1;
	}

	printf("Estoy escuchando....\n");
	listen(servidor, 100);

	//------------------------------

	struct sockaddr_in direccionCliente;
	//direccionCliente.sin_family=1;
	//direccionCliente.sin_port=0;
	unsigned int len = sizeof(direccionCliente);
	int cliente = accept(servidor, (void*) &direccionCliente, &len);

	printf("Recibí una conexión en %d!! \n", cliente);
//	send(cliente, "Hola ", 13, 0);
//	send(cliente, ":)\n", 4, 0);

	//------------------------------

	char* buffer = malloc(1000);
	while (1) {
		//printf("\n imprime ");
		int bytesRecibidos = recv(cliente, buffer, 1000, 0);
		if (bytesRecibidos <= 0) {
			perror("El chabón se desconectó o bla.\n");
			return 1;
		}

		buffer[bytesRecibidos] = '\0';
		printf("\n mensaje: %s", buffer);
	}
	free(buffer);

	// uint32_t tamaño;
	// recv(cliente, &tamaño, 4, 0);

	// char* buffer = malloc(tamaño);
	// recv(cliente, buffer, tamaño, MSG_WAITALL);

	return 0;
}
