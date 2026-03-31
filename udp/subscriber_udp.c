#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PUERTO_BROKER  8080
#define TAMANO_BUFFER  1024

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Uso: %s <tema1> [tema2] ...\n", argv[0]);
        printf("Ejemplo: %s partido_A partido_B\n", argv[0]);
        return 1;
    }

    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (socket_fd < 0) {
        perror("Error creando socket");
        return 1;
    }

    struct sockaddr_in direccion_local;
    memset(&direccion_local, 0, sizeof(direccion_local));

    direccion_local.sin_family      = AF_INET;
    direccion_local.sin_addr.s_addr = INADDR_ANY;
    direccion_local.sin_port        = htons(0);

    if (bind(socket_fd, (struct sockaddr *)&direccion_local, sizeof(direccion_local)) < 0) {
        perror("Error en bind");
        close(socket_fd);
        return 1;
    }

    socklen_t longitud_local = sizeof(direccion_local);
    getsockname(socket_fd, (struct sockaddr *)&direccion_local, &longitud_local);
    int puerto_local = ntohs(direccion_local.sin_port);

    printf("Suscriptor UDP enlazado en puerto local: %d\n", puerto_local);
    printf("(UDP: el broker identificara este suscriptor por 127.0.0.1:%d)\n\n", puerto_local);

    struct sockaddr_in direccion_broker;
    memset(&direccion_broker, 0, sizeof(direccion_broker));

    direccion_broker.sin_family      = AF_INET;
    direccion_broker.sin_port        = htons(PUERTO_BROKER);
    direccion_broker.sin_addr.s_addr = inet_addr("127.0.0.1"); // IP local (localhost)

    char buffer[TAMANO_BUFFER];

    for (int i = 1; i < argc; i++) {

        snprintf(buffer, sizeof(buffer), "SUB %s\n", argv[i]);

        ssize_t bytes_enviados = sendto(socket_fd,
                                        buffer,
                                        strlen(buffer),
                                        0,
                                        (struct sockaddr *)&direccion_broker,
                                        sizeof(direccion_broker));

        if (bytes_enviados < 0) {
            perror("Error enviando SUB");
            close(socket_fd);
            return 1;
        }

        printf("Suscrito a: %s\n", argv[i]);
    }

    printf("\nEsperando mensajes...\n");
    printf("(UDP: los mensajes pueden llegar desordenados o perderse)\n\n");

    struct sockaddr_in direccion_emisor;
    socklen_t longitud_emisor = sizeof(direccion_emisor);
    int contador_mensajes = 0;

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        memset(&direccion_emisor, 0, sizeof(direccion_emisor));

        ssize_t bytes_recibidos = recvfrom(socket_fd,
                                           buffer,
                                           TAMANO_BUFFER - 1,
                                           0,
                                           (struct sockaddr *)&direccion_emisor,
                                           &longitud_emisor);

        if (bytes_recibidos < 0) {
            perror("Error en recvfrom");
            break;
        }

        buffer[bytes_recibidos] = '\0';

        contador_mensajes++;
        printf("[Mensaje #%d recibido] %s\n", contador_mensajes, buffer);
        fflush(stdout);
    }

    close(socket_fd);
    printf("\nTotal de mensajes recibidos: %d\n", contador_mensajes);

    return 0;
}