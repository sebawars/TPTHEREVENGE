/*
 * SocketsCliente.h
 *
 *  Created on: 7/4/2017
 *      Author: utnso
 */

#ifndef LIBRARY_SOCKETS_SOCKETSCLIENTE_H_
#define LIBRARY_SOCKETS_SOCKETSCLIENTE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "socket.h"


int AbrirConexion(char *ip, int puerto);
int aceptarConexion(int socket, int handshake);

#endif /* LIBRARY_SOCKETS_SOCKETSCLIENTE_H_ */
