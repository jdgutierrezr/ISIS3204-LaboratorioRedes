#include <stdio.h>      // printf
#include <stdlib.h>     // utilidades básicas
#include <string.h>     // strlen, snprintf, memset
#include <unistd.h>     // close
#include <arpa/inet.h>  // sockets, inet_addr, sockaddr_in

#define PORT 8080
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {

    // Se esperan uno o más topics:
    if (argc < 2) {
        printf("Uso: %s <topic1> [topic2] ...\n", argv[0]);
        printf("Ejemplo: %s partido_A partido_B\n", argv[0]);
        return 1;
    }

    // Crear socket TCP
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        perror("Error creando socket");
        return 1;
    }

    // Configurar direccion del Broker
    struct sockaddr_in broker_addr;

    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(PORT);
    broker_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // IP local (localhost)

    // Conectar al Broker, establece conexión TCP (handshake)
    if (connect(sock, (struct sockaddr *)&broker_addr, sizeof(broker_addr)) < 0) {
        perror("Error conectando al broker");
        return 1;
    }

    printf("Subscriber conectado al broker.\n");

    // Suscripción a múltiples topics
    char buffer[BUFFER_SIZE];

    for (int i = 1; i < argc; i++) {

        // Construye comando: SUB <topic>\n
        snprintf(buffer, sizeof(buffer), "SUB %s\n", argv[i]);

        // Enviar comando al broker
        if (send(sock, buffer, strlen(buffer), 0) < 0) {
            perror("Error suscribiendose");
            close(sock);
            return 1;
        }

        printf("Suscrito a: %s\n", argv[i]);
    }

    printf("\nEsperando mensajes...\n\n");

    while (1) {
        memset(buffer, 0, sizeof(buffer));

        // Recibe datos del broker
        int valread = recv(sock, buffer, BUFFER_SIZE - 1, 0);

        // Si <= 0 → conexión cerrada o error
        if (valread <= 0) {
            printf("Conexion cerrada por el broker.\n");
            break;
        }

        // Imprime el mensaje recibido
        printf("[Mensaje recibido] %s\n", buffer);
        fflush(stdout);
    }

    // Cierre de conexion
    close(sock);

    return 0;
}