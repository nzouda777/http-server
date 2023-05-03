#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 2048

void handle_get_request(SOCKET client_socket, char *request_path) {
    char buffer[BUFFER_SIZE];
    int file_size = 0;
    int bytes_read = 0;
    char *mime_type = "text/html";
    FILE *file;

    // Ouvre le fichier HTML correspondant à la requête
    file = fopen(request_path, "rb");
    if (!file) {
        printf("Erreur lors de l'ouverture du fichier %s\n", request_path);
        return;
    }

    // Détermine la taille du fichier
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Construit la réponse HTTP
    sprintf(buffer, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: %s\r\n\r\n", file_size, mime_type);
    send(client_socket, buffer, strlen(buffer), 0);

    // Envoie le contenu du fichier HTML
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }

    // Ferme le fichier
    fclose(file);
}

void handle_post_request(SOCKET client_socket, char *request_data) {
    // Traite les données de la requête POST
    printf("Données de la requête POST reçues : %s\n", request_data);

    // Construit la réponse HTTP
    char *message = "<html><head><title>Mon serveur Web</title></head><body><h1>Bienvenue sur mon serveur Web !</h1></body></html>";
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n%s", strlen(message), message);
    send(client_socket, buffer, strlen(buffer), 0);
}

void handle_request(SOCKET client_socket, char *request) {
    char method[10], path[255], protocol[10], *request_data;
    int i = 0, j = 0;

    // Récupère la méthode, le chemin et le protocole de la requête
    while (request[i] != ' ') {
        method[i] = request[i];
        i++;
    }
    method[i] = '\0';

    i++;
    while (request[i] != ' ') {
        path[j] = request[i];
        i++;
        j++;
    }
    path[j] = '\0';

    i += 2;
    j = 0;
    while (request[i] != '\r') {
        protocol[j] = request[i];
        i++;
        j++;
    }
    protocol[j] = '\0';

    // Traite la requête en fonction de la méthode HTTP
    if (strcmp(method, "GET") == 0) {
        handle_get_request(client_socket, "index.html");
    } else if (strcmp(method, "POST") == 0) {
        request_data = strstr(request, "\r\n\r\n");
        if (request_data) {
            request_data += 4;
            handle_post_request(client_socket, request_data);
        }
    }
}

int main() {
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    int iResult, addr_len = sizeof(struct sockaddr_in);

    // Initialise Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsa);
    if (iResult != 0) {
        printf("Erreur lors de l'initialisation de Winsock : %d\n", iResult);
        return 1;
    }

    // Crée une socket pour le serveur
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        printf("Erreur lors de la création de la socket : %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Initialise l'adresse IP et le port du serveur
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    // Associe la socket à l'adresse IP et au port du serveur
    iResult = bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (iResult == SOCKET_ERROR) {
        printf("Erreur lors de l'association de la socket à l'adresse IP et au port du serveur : %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    // Met la socket en écoute de connexions entrantes
    iResult = listen(server_socket, MAX_CLIENTS);
    if (iResult == SOCKET_ERROR) {
        printf("Erreur lors de la mise en écoute de la socket : %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    printf("Serveur HTTP en écoute sur le port 8080...\n");

    // Boucle d'attente de connexions entrantes
    while (1) {
        // Accepte une connexion entrante
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket == INVALID_SOCKET) {
            printf("Erreur lors de l'acceptation de la connexion entrante : %d\n", WSAGetLastError());
            closesocket(server_socket);
            WSACleanup();
            return 1;
        }

        printf("Connexion acceptée de %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Réception de la requête HTTP du client
        memset(buffer, 0, BUFFER_SIZE);
        iResult = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (iResult == SOCKET_ERROR) {
            printf("Erreur lors de la réception de la requête HTTP : %d\n", WSAGetLastError());
            closesocket(client_socket);
            continue;
        }

        // Traite la requête HTTP
        handle_request(client_socket, buffer);

        // Ferme la socket du client
        closesocket(client_socket);
    }

    // Ferme la socket du serveur
    closesocket(server_socket);
    WSACleanup();

    return 0;
}

