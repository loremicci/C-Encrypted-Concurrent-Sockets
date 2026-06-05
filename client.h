#pragma once


#include "funzioni.h"


// ::::::::::::::::::::::::::::::::::::::: ATTRIBUTI ::::::::::::::::::::::::::::::::::::::::::::::
// struct contenente gli attributi del server.
typedef struct C_attr{
    char name[256];             // dimensione massima nome di un file in Linux
    unsigned long long K;       // chiave
    unsigned short p;           // numero di thread
    char IP[INET_ADDRSTRLEN];   // (xxx.xxx.xxx.xxx) IP server
    unsigned short port;        // porta del server [0-65535]
} C_attr;

/**
 * @brief Valorizza con gli argomenti del main gli attributi del client.
 * @param argc Il numero di argomenti del main del client.
 * @param args Gli argomenti del main del client.
 * @return Ritorna gli attributi del client.
 */
C_attr* init(const int argc, char** args);


// ::::::::::::::::::::::::::::::::::::::: BASE FILE :::::::::::::::::::::::::::::::::::::::::::::::
/**
 * @brief Ritorna il numero di byte del file.
 * @param f Il file.
 * @return Il numero di byte del file.
 */
size_t file_get_size(FILE* f);

/**
 * @brief Legge il testo del file.
 * @param f Il file.
 * @return Il testo del file.
 */
char* file_get_text(FILE* f);


// ::::::::::::::::::::::::::::::::::::::: SOCKET :::::::::::::::::::::::::::::::::::::::::::::::::
/**
 * @brief Crea una socket TCP.
 * @param serv_addr Puntatore alla struttura della socket.
 * @param ip Puntatore all'IP del server.
 * @param port Porta da associare.
 * @return Puntatore al file descriptor della socket.
 */
int* create_client_socket_tcp(struct sockaddr_in* serv_addr, const char* ip, const unsigned short port);

/**
 * @brief Si connette alla socket.
 * @param soc Puntatore alla socket.
 * @param serv_addr Puntatore alla struttura della socket.
 */
void connect_socket(const int* soc, struct sockaddr_in* serv_addr);