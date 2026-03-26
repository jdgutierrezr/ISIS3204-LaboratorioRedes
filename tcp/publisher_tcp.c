#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080           // Puerto del broker
#define BUFFER_SIZE 1024    // Tamaño del buffer de lectura

int main(int argc, char *argv[]) {

    // Se espera que el usuario pase el topic por consola
    if (argc < 2) {
        printf("Uso: %s <topic>\n", argv[0]);
        printf("Ejemplo: %s partido_A\n", argv[0]);
        return 1;
    }

    // Topic al que se publicarán los mensajes
    char *topic = argv[1];

    // Crear socket TCP
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    // Validación de error
    if (sock < 0) {
        perror("Error creando socket");
        return 1;
    }

    // Configurar dirección del Broker
    struct sockaddr_in broker_addr;

    broker_addr.sin_family = AF_INET;
    broker_addr.sin_port = htons(PORT);
    broker_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // IP local (localhost)

    // Conectar al Broker, establece conexión TCP (handshake)
    if (connect(sock, (struct sockaddr *)&broker_addr, sizeof(broker_addr)) < 0) {
        perror("Error conectando al broker");
        return 1;
    }

    printf("Publisher conectado al broker. Topic: %s\n", topic);
    printf("Escribe mensajes (CTRL+C para salir):\n\n");

    char input[900];          // Mensaje del usuario
    char buffer[BUFFER_SIZE]; // Buffer para enviar al broker

    while (1) {
        printf("[%s] > ", topic);
        fflush(stdout);

        // Leer entrada del usuario (stdin)
        if (fgets(input, sizeof(input), stdin) == NULL) break;

        // Eliminar salto de línea '\n' que deja fgets
        input[strcspn(input, "\n")] = '\0';

        // Ignorar mensajes vacíos
        if (strlen(input) == 0) continue;

        // Construye el mensaje según el protocolo del broker: "PUB <topic> <mensaje>\n"
        snprintf(buffer, sizeof(buffer), "PUB %s %s\n", topic, input);

        // Envio al Broker
        if (send(sock, buffer, strlen(buffer), 0) < 0) {
            perror("Error enviando mensaje");
            break;
        }

        printf("Mensaje enviado.\n");
    }

    // Cierre de conexion
    close(sock);

    return 0;
}