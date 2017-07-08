/*
 * SocketsCliente.c
 *
 *  Created on: 7/4/2017
 *      Author: utnso
 */

#include "SocketsCliente.h"

	/* Conecta con un servidor remoto a traves de socket INET
	  y me devuelve el descriptor al socket creado.
	  CON ESTA EL SOCKET YA ESTA CONECTADO EN ESPERA DE MANDAR Y RECIBIR AL SERVIDOR
	*/

int AbrirConexion(char *ip, int puerto)
	{
		struct sockaddr_in direccionServidor;
		int nuevoSocket;
		// Se carga informacion del socket DE DESTINO
		direccionServidor.sin_family = AF_INET;
		direccionServidor.sin_addr.s_addr = inet_addr(ip);
		direccionServidor.sin_port = htons(puerto);

		// Crear un socket CLIENTE:
		// AF_INET, SOCK_STREM, 0
		nuevoSocket = socket (AF_INET, SOCK_STREAM, 0);
		if (nuevoSocket < 0)
			return -1;
		// Conectar el socket con la direccion 'socketInfo'.
		while(connect (nuevoSocket,(struct sockaddr *)&direccionServidor,sizeof (direccionServidor)) != 0)
		{
			perror("No se pudo conectar, Reintento en 5seg...");
			sleep(5);
		}
/*
		while (1) {
				char mensaje[1000];
				scanf("%s", mensaje);

				send(nuevoSocket, mensaje, strlen(mensaje), 0);
			}
*/
			return nuevoSocket;
	}

int aceptarConexion(int socket, int handshake)
{

    header_t header;
    recv(socket, &header, sizeof(header_t), MSG_WAITALL);

	if (header.type==handshake)
		return 0;
	else{
		close(socket);
		return -1;
	}
}
