#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_TOPICS 10
#define MAX_SUBS 20

typedef struct {
    struct sockaddr_in addr;
} Subscriber;

typedef struct {
    char name[50];
    Subscriber subscribers[MAX_SUBS];
    int sub_count;
} Topic;

Topic topics[MAX_TOPICS];
int topic_count = 0;

/**
 * Compara dos direcciones UDP (IP + puerto)
 */
int same_client(struct sockaddr_in *a, struct sockaddr_in *b) {
    return (a->sin_family == b->sin_family &&
            a->sin_port == b->sin_port &&
            a->sin_addr.s_addr == b->sin_addr.s_addr);
}

/**
 * Busca un topic existente por nombre.
 * Si no existe, lo crea y lo devuelve.
 */
Topic *get_or_create_topic(char *name) {
    for (int i = 0; i < topic_count; i++) {
        if (strcmp(topics[i].name, name) == 0) {
            return &topics[i];
        }
    }

    if (topic_count >= MAX_TOPICS) {
        printf("[ERROR] Maximo de topics alcanzado\n");
        return NULL;
    }

    strcpy(topics[topic_count].name, name);
    topics[topic_count].sub_count = 0;
    topic_count++;

    return &topics[topic_count - 1];
}

/**
 * Suscribe un cliente UDP a un topic
 */
void subscribe_client(struct sockaddr_in *client_addr, char *topic_name) {
    Topic *topic = get_or_create_topic(topic_name);
    if (topic == NULL) return;

    // Evitar suscripción duplicada
    for (int i = 0; i < topic->sub_count; i++) {
        if (same_client(&topic->subscribers[i].addr, client_addr)) {
            printf("[SUB] Cliente ya suscrito a %s (%s:%d)\n",
                   topic_name,
                   inet_ntoa(client_addr->sin_addr),
                   ntohs(client_addr->sin_port));
            return;
        }
    }

    if (topic->sub_count >= MAX_SUBS) {
        printf("[ERROR] Maximo de suscriptores alcanzado para topic %s\n", topic_name);
        return;
    }

    topic->subscribers[topic->sub_count].addr = *client_addr;
    topic->sub_count++;

    printf("[SUB] %s:%d -> %s\n",
           inet_ntoa(client_addr->sin_addr),
           ntohs(client_addr->sin_port),
           topic_name);
}

/**
 * Publica un mensaje en un topic y lo envía a todos los suscriptores
 */
void publish_message(int server_fd, char *topic_name, char *message) {
    for (int i = 0; i < topic_count; i++) {
        if (strcmp(topics[i].name, topic_name) == 0) {

            printf("[PUB] %-15s -> \"%s\" (%d suscriptor/es)\n",
                   topic_name, message, topics[i].sub_count);

            char outbuf[BUFFER_SIZE];
            snprintf(outbuf, sizeof(outbuf), "(%s) %s", topic_name, message);

            for (int j = 0; j < topics[i].sub_count; j++) {
                sendto(server_fd,
                       outbuf,
                       strlen(outbuf),
                       0,
                       (struct sockaddr *)&topics[i].subscribers[j].addr,
                       sizeof(topics[i].subscribers[j].addr));
            }
            return;
        }
    }

    printf("[WARN] Topic \"%s\" no existe. Mensaje descartado.\n", topic_name);
}

/**
 * Procesa un comando UDP:
 *   SUB topic
 *   PUB topic mensaje...
 */
void handle_message(int server_fd, char *buffer, struct sockaddr_in *client_addr) {
    char command[10];
    char topic[50];
    char msg[900];

    memset(command, 0, sizeof(command));
    memset(topic, 0, sizeof(topic));
    memset(msg, 0, sizeof(msg));

    // Quitar salto de línea final si existe
    buffer[strcspn(buffer, "\r\n")] = '\0';

    int parsed = sscanf(buffer, "%9s %49s %899[^\n]", command, topic, msg);

    if (parsed >= 2) {
        if (strcmp(command, "SUB") == 0) {
            subscribe_client(client_addr, topic);
        } else if (strcmp(command, "PUB") == 0) {
            if (parsed == 3) {
                publish_message(server_fd, topic, msg);
            } else {
                printf("[WARN] PUB sin mensaje\n");
            }
        } else {
            printf("[WARN] Comando desconocido: %s\n", command);
        }
    } else {
        printf("[WARN] Mensaje invalido: %s\n", buffer);
    }
}

int main() {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("===================================\n");
    printf("  BROKER UDP - Puerto %d\n", PORT);
    printf("===================================\n\n");

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        memset(&client_addr, 0, sizeof(client_addr));

        int n = recvfrom(server_fd,
                         buffer,
                         BUFFER_SIZE - 1,
                         0,
                         (struct sockaddr *)&client_addr,
                         &client_len);

        if (n < 0) {
            perror("recvfrom");
            continue;
        }

        buffer[n] = '\0';

        printf("[RX ] %s:%d -> %s\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port),
               buffer);

        handle_message(server_fd, buffer, &client_addr);
    }

    close(server_fd);
    return 0;
}