#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

// Configuración básica del broker
#define PORT 8080           // Puerto en el que escucha el servidor
#define MAX_CLIENTS 10      // Máximo número de clientes simultáneos
#define BUFFER_SIZE 1024    // Tamaño del buffer de lectura
#define MAX_TOPICS 10       // Máximo número de topics
#define MAX_SUBS 10         // Máximo suscriptores por topic

// Estructura que representa un topic
typedef struct {
    char name[50];
    int subscribers[MAX_SUBS];
    int sub_count;
} Topic;

Topic topics[MAX_TOPICS];
int topic_count = 0;


/**
 * Busca un topic existente por nombre.
 * Si no existe, lo crea y lo devuelve.
 */
Topic* get_or_create_topic(char *name) {
    // Buscar si el topic ya existe
    for (int i = 0; i < topic_count; i++)
        if (strcmp(topics[i].name, name) == 0)
            return &topics[i];

    // Si no existe, se crea uno nuevo
    strcpy(topics[topic_count].name, name);
    topics[topic_count].sub_count = 0;
    topic_count++;

    // Retorna el último creado
    return &topics[topic_count - 1];
}


/**
 * Suscribe un cliente (socket) a un topic
 */
void subscribe(int client_socket, char *topic_name) {
    // Obtiene el topic (o lo crea si no existe)
    Topic *topic = get_or_create_topic(topic_name);

    // Agrega el socket a la lista de suscriptores
    topic->subscribers[topic->sub_count++] = client_socket;

    printf("[SUB] Socket %-3d → %s\n", client_socket, topic_name);
}


/**
 * Publica un mensaje en un topic:
 * lo envía a todos los suscriptores
 */
void publish(char *topic_name, char *message) {
    // Buscar el topic
    for (int i = 0; i < topic_count; i++) {
        if (strcmp(topics[i].name, topic_name) == 0) {

            printf("[PUB] %-15s → \"%s\" (%d suscriptor/es)\n",
                   topic_name, message, topics[i].sub_count);

            // Enviar el mensaje a cada suscriptor
            for (int j = 0; j < topics[i].sub_count; j++) {

                char outbuf[BUFFER_SIZE];

                snprintf(outbuf, sizeof(outbuf), "(%s) %s\n", topic_name, message);

                // Enviar al socket del suscriptor
                send(topics[i].subscribers[j], outbuf, strlen(outbuf), 0);
            }
        }
    }
}


/**
 * Procesa los comandos enviados por un cliente.
 * Puede manejar múltiples comandos en un solo buffer.
 *
 * Formato esperado:
 *   SUB nombre_topic
 *   PUB nombre_topic mensaje
 */
void handle_message(int client_socket, char *buffer) {

    // Divide el buffer en líneas separadas por '\n'
    char *line = strtok(buffer, "\n");

    while (line != NULL) {

        char command[10], topic[50], msg[900];

        msg[0] = '\0';

        // Parseo de la línea:
        // command = SUB o PUB
        // topic = nombre del topic
        // msg = resto del mensaje (puede tener espacios)
        sscanf(line, "%s %s %[^\n]", command, topic, msg);

        // Decisión según comando
        if (strcmp(command, "SUB") == 0) {
            subscribe(client_socket, topic);
        } else if (strcmp(command, "PUB") == 0) {
            publish(topic, msg);
        }

        line = strtok(NULL, "\n");
    }
}


/**
 * Función principal:
 * - Configura el servidor TCP
 * - Acepta conexiones
 * - Maneja múltiples clientes con select()
 */
int main() {

    int server_fd, new_socket;
    int client_sockets[MAX_CLIENTS];

    struct sockaddr_in address;
    fd_set readfds;

    int addrlen = sizeof(address);

    // Inicializa todos los sockets como vacíos (0)
    for (int i = 0; i < MAX_CLIENTS; i++) client_sockets[i] = 0;

    // Crear socket TCP (AF_INET + SOCK_STREAM = TCP)
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Configuración de dirección
    address.sin_family = AF_INET;         // IPv4
    address.sin_addr.s_addr = INADDR_ANY; // Acepta cualquier IP
    address.sin_port = htons(PORT);       // Puerto en formato red

    // Asociar socket con IP y puerto
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));

    // Escuchar conexiones entrantes
    listen(server_fd, 3);

    printf("===================================\n");
    printf("  BROKER TCP — Puerto %d\n", PORT);
    printf("===================================\n\n");

    while (1) {

        FD_ZERO(&readfds);

        // Agregar el socket del servidor
        FD_SET(server_fd, &readfds);

        int max_sd = server_fd;

        // Agregar sockets de clientes al set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];

            if (sd > 0)
                FD_SET(sd, &readfds);

            if (sd > max_sd)
                max_sd = sd;
        }

        // Espera actividad en alguno de los sockets
        select(max_sd + 1, &readfds, NULL, NULL, NULL);

        // Si hay nueva conexión entrante
        if (FD_ISSET(server_fd, &readfds)) {

            new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);

            printf("[+]  Nueva conexion  — socket %d\n", new_socket);

            // Guardar el nuevo socket en la lista
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }

        // Revisar actividad en clientes
        for (int i = 0; i < MAX_CLIENTS; i++) {

            int sd = client_sockets[i];

            if (FD_ISSET(sd, &readfds)) {

                char buffer[BUFFER_SIZE] = {0};

                // Leer datos del cliente
                int valread = recv(sd, buffer, BUFFER_SIZE, 0);

                // Si recv retorna 0 → cliente desconectado
                if (valread == 0) {

                    printf("[-]  Socket %d desconectado\n", sd);

                    close(sd);
                    client_sockets[i] = 0;

                } else {
                    handle_message(sd, buffer);
                }
            }
        }
    }

    return 0;
}