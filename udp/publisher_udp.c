#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PUERTO        8080
#define TAMANO_BUFFER 1024

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Uso: %s <tema>\n", argv[0]);
        printf("Ejemplo: %s partido_A\n", argv[0]);
        return 1;
    }

    char *tema = argv[1];

    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (socket_fd < 0) {
        perror("Error creando socket");
        return 1;
    }

    struct sockaddr_in direccion_broker;
    memset(&direccion_broker, 0, sizeof(direccion_broker));

    direccion_broker.sin_family      = AF_INET;
    direccion_broker.sin_port        = htons(PUERTO);
    direccion_broker.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("Publicador UDP listo. Tema: %s\n", tema);
    printf("(UDP: sin conexion persistente — cada mensaje es un datagrama)\n");
    printf("Escribe mensajes (CTRL+C para salir):\n\n");

    char entrada[900];
    char buffer[TAMANO_BUFFER];

    int contador_mensajes = 0;

    while (1) {
        printf("[%s] > ", tema);
        fflush(stdout);

        if (fgets(entrada, sizeof(entrada), stdin) == NULL) break;

        entrada[strcspn(entrada, "\n")] = '\0';

        if (strlen(entrada) == 0) continue;

        snprintf(buffer, sizeof(buffer), "PUB %s %s\n", tema, entrada);

        ssize_t bytes_enviados = sendto(socket_fd,
                                        buffer,
                                        strlen(buffer),
                                        0,
                                        (struct sockaddr *)&direccion_broker,
                                        sizeof(direccion_broker));

        if (bytes_enviados < 0) {
            perror("Error enviando datagrama");
            break;
        }

        contador_mensajes++;
        printf("Datagrama #%d enviado (%zd bytes).\n", contador_mensajes, bytes_enviados);
    }

    close(socket_fd);
    printf("\nTotal de mensajes enviados: %d\n", contador_mensajes);

    return 0;
}