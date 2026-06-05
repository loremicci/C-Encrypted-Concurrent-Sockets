#pragma once
// argomenti comuni a client e server

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <pthread.h>
#include <signal.h>

#define ACK 42

// :::::::::::::::::::::::::::::::::::::::::: ERRORI ::::::::::::::::::::::::::::::::::::::::::::::
/**
 * @brief Stampa il possibile errore ed interrompe l'esecuzione.
 */
void fatal_error();

/**
 * @brief Stampa il possibile errore, non interrompe l'esecuzione.
 * @return true se ha rilevato un errore, false altrimenti
 */
int error();

/**
 * @brief Stampa il possibile errore durante l'operazione su un file ed interrompe l'esecuzione.
 * @param f Il file.
 */
void file_fatal_error(FILE* f);

/**
 * @brief Stampa il possibile errore durante l'operazione su un file, non interrompe l'esecuzione.
 * @param f Il file.
 * @return true se ha rilevato un errore, false altrimenti
 */
int file_error(FILE* f);

/**
 * @brief Blocca i segnali SIGINT, SIGALRM, SIGUSR1, SIGUSR2 e SIGTERM.
 */
void block_signals(sigset_t* block, sigset_t* oldset);

/**
 * @brief Sblocca i segnali SIGINT, SIGALRM, SIGUSR1, SIGUSR2 e SIGTERM.
*/
void unblock_signals(sigset_t* oldset);


// ::::::::::::::::::::::::::::::::::::::: ATTRIBUTI ::::::::::::::::::::::::::::::::::::::::::::::
#define FLAG 4163 // flags = 4163: up, broadcast, running, multicast

/**
 * @brief Prende l'IP pubblico attivo della macchina locale.
 * @return L'IP pubblico della macchina locale o NULL.
 */
char* get_ip();


// ::::::::::::::::::::::::::::::::::::::: BASE FILE ::::::::::::::::::::::::::::::::::::::::::::::
/**
 * @brief Apre il file.
 * @param path Il percorso del file da aprire.
 * @param mod La modalita` di apertura.
 * @return Il puntatore al file aperto o NULL se ha rilevato errore.
 */
FILE* file_open(const char* path, const char* mod);

/**
 * @brief Chiude il file.
 * @param path Il percorso del file da chiudere.
 * @return true se e' andato a buon fine, false altrimenti.
 */
int file_close(FILE* f);


// ::::::::::::::::::::::::::::::::::::::: CIFRATURA ::::::::::::::::::::::::::::::::::::::::::::::
#define LEN_BLOCK 8 // lunghezza di un blocco

// struct per gestire i blocchi.
typedef struct{
    char** blocks;            // lista dei blocchi
    size_t size;  // numero di blocchi
} Blocks;

// struct contenente gli argomenti di un thread per la cifratura.
typedef struct{
    Blocks* bl;                    // struttura di un blocco
    size_t start;      // inizio del blocco
    size_t end;        // fine del blocco
    const unsigned long long* K;   // chiave
} T_data;

/**
 * @brief Stampa il testo carattere per carattere (stampa anche eventuali \0).
 * @param text Puntatore al testo da stampare.
 * @param len Lunghezza del testo da stampare.
 */
void print_text(const char* text, const size_t len);

/**
 * @brief Suddivide il testo F in blocchi da LEN_BLOCK byte ciascuno in base alla sua lunghezza L.
 * @param F Testo F del file aperto.
 * @param L Lunghezza di F.
 * @return Puntatore alla struttura dati dei blocchi o NULL in caso di errore.
 */
Blocks* create_blocks(const char* F, const size_t L);

/**
 * @brief Stampa il contenuto dei blocchi.
 * @param bl Puntatore ai blocchi.
 */
void print_blocks(const Blocks* bl);

/**
 * @brief Unisce il testo dei blocchi in una stringa.
 * @param bl La struct dei blocchi.
 * @return Stinga con il testo di tutti i blocchi.
 */
char* concatenate_blocks(Blocks* bl);

/**
 * @brief Cifra/Decifra i blocchi assegneati al thread relativo. 
 * @param arg Puntatore alla struttura dati T_data contenente gli argomenti del thread. 
 * @return NULL.
 */
void* modify_blocks(void* arg);

/**
 * @brief Crea p thread per cifrare/decifrare i blocchi bl con la chiave K.
 * @param bl Blocchi da cifrare.
 * @param p Numero thread da creare.
 * @param K Chiave di cifratura.
 * @return true se tutti i blocchi sono stati cifrati/decifrati correttamente, false altrimenti.
 */
int thread_modify_blocks(Blocks* bl, const unsigned short p, const unsigned long long K);

/**
 * @brief Libera la memoria occupata dalla struct dei blocchi.
 * @param bl La struct dei blocchi.
 */
void free_blocks(Blocks* bl);


// ::::::::::::::::::::::::::::::::::::::: SOCKET :::::::::::::::::::::::::::::::::::::::::::::::::
/**
 * @brief Invia l'argomento alla socket specificata.
 * @param soc Socket dove inviare il Package.
 * @param arg Argomento da inviare.
 * @param size Dimensione dell'argomento da inviare.
 * @return true se la ricezione e' andata a buon fine, false altrimenti.
 */
int send_arg(const int soc, const void* arg, const size_t size);

/**
 * @brief Riceve l'argomento alla socket specificata.
 * @param soc Socket dove ricevere il Package.
 * @param arg Argomento da ricevere.
 * @param size Dimensione dell'argomento da ricevere.
 * @return true se la ricezione e' andata a buon fine, false altrimenti.
 */
int recive(const int soc, void* arg, const size_t size);