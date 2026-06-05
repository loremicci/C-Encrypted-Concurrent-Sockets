#pragma once


#include "funzioni.h"
#include <unistd.h>
#include <fcntl.h>
#include <time.h>


// ::::::::::::::::::::::::::::::::::::::: ATTRIBUTI ::::::::::::::::::::::::::::::::::::::::::::::
// struct contenente gli attributi del server
typedef struct S_attr{
    unsigned short p;   // numero di thread per ogni client
    char s[256];        // prefisso del file in output
    unsigned short l;   // massime connessioni in parallelo 
} S_attr;

/**
 * @brief Valorizza con gli argomenti del main gli attributi del server.
 * @param argc Il numero di argomenti del main del server.
 * @param args Gli argomenti del main del server.
 * @return Gli attributi del server.
 */
S_attr* init(int argc, char** args);


// ::::::::::::::::::::::::::::::::::::::: BASE FILE :::::::::::::::::::::::::::::::::::::::::::::::
/**
 * @brief Scrive il testo nel file.
 * Se il file già conteneva del testo viene sovrascritto.
 * @param f Il file di destinazione.
 * @param text Il testo da inserire nel file.
 * @return true se e' andato a buon fine, false altrimenti.
 */
int file_write(FILE* f, const char* text);

/**
 * @brief Concatena src alla fine di dest.
 * @param dest Stringa che ricevera' la concatenazione.
 * @param src Stringa da concatenare.
 * @return true se ha successo, false altrimenti.
 */
int str_append(char** dest, const char* src);


// ::::::::::::::::::::::::::::::::::::::: SOCKET :::::::::::::::::::::::::::::::::::::::::::::::::
// struct contenente i dati per gestire i thread di un client.
typedef struct T_c_data{
    int* client;                // client padre dei thread
    unsigned short p;           // Thread per ogni client
    char s[256];                // prefisso del file in output
    char ip[INET_ADDRSTRLEN];   // ip del client
} T_c_data;

/**
 * @brief Crea una socket TCP.
 * @param ip Puntatore all'IP del server.
 * @param port Porta da associare.
 * @param l Puntatore al valore delle massime connessioni in parallelo.
 * @return Puntatore al file descriptor della socket.
 */
int* create_server_socket_tcp(const char* ip, unsigned short port, unsigned short l);

/**
 * @brief Accetta una connessione.
 * @param soc Puntatore alla socket ascoltata dal server.
 * @return Puntatore al file descriptor del client se ha accettato la connessione, NULL altrimenti.
 */
int* accept_client(int* soc, char** cli_ip);

/**
 * @brief Gestisce il client.
 * @param arg Puntatore al file descriptor del client.
 * @return NULL.
 */
void* client_handler(void* args);

/**
 * @brief Crea un nuovo thread per gestire il client.
 * @param client Puntatore al file descriptor del client.
 * @param p Thread per ogni client.
 * @param s Prefisso per il file di output.
 * @return true se il client e' stato gestito correttamente, false altrimenti.
 */
void create_client_thread(int* client, unsigned short p, const char* s, char* ip);