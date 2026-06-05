#include "client.h"


// ::::::::::::::::::::::::::::::::::::::: ATTRIBUTI ::::::::::::::::::::::::::::::::::::::::::::::
C_attr* init(const int argc, char** args){
    if(argc != 6){
        perror("Errore: numero di argomenti errato\n");
        exit(EXIT_FAILURE);
    }

    // CREA LA STRUCT PER GLI ATTRIBUTI DEL CLIENT
    C_attr* c_a = (C_attr*) malloc(sizeof(C_attr));
    fatal_error();
    *c_a = (C_attr) {{'\0'}, 0, 0, {'\0'}, 0};

    // VALORIZZA GLI ATTRIBUTI
    strcpy(c_a->name, args[1]);
    if(strlen(args[1]) > 255){
        fprintf(stderr, "Errore: nome del file in input troppo lungo\n");
        exit(EXIT_FAILURE);
    }

    if(strlen(args[2]) != LEN_BLOCK){
        fprintf(stderr, "Errore: chiave non valida\n");
        exit(EXIT_FAILURE);
    }
    memcpy(&c_a->K, args[2], LEN_BLOCK);

    c_a->p = atoi(args[3]);
    if(c_a->p <= 0){
        fprintf(stderr, "Errore: numero di thread non valido\n");
        exit(EXIT_FAILURE);
    }

    strcpy(c_a->IP, args[4]);
    if(strlen(args[4]) > 15){
        fprintf(stderr, "Errore: IP del server non valido\n");
        exit(EXIT_FAILURE);
    }

    c_a->port = atoi(args[5]);
    if(c_a->port <= 0){
        fprintf(stderr, "Errore: numero di porta del server non valida\n");
        exit(EXIT_FAILURE);
    }

    return c_a;
}


// ::::::::::::::::::::::::::::::::::::::: BASE FILE :::::::::::::::::::::::::::::::::::::::::::::::
size_t file_get_size(FILE* f){
    if(!f) return 0;

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f); // sposta il puntatore all'ultimo carattere del file
    file_fatal_error(f);
    rewind(f); // sposta il puntatore al primo carattere del file
    return size;
}

char* file_get_text(FILE* f){
    if(!f) return NULL;

    size_t size = file_get_size(f);
    if(size == 0) return NULL;
    char* out = (char*)malloc(size + 1);
    fatal_error();

    fread(out, sizeof(char), size / sizeof(char), f);
    file_fatal_error(f);
    out[size] = '\0';
    return out;
}


// ::::::::::::::::::::::::::::::::::::::: SOCKET :::::::::::::::::::::::::::::::::::::::::::::::::
int* create_client_socket_tcp(struct sockaddr_in* serv_addr, const char* ip, const unsigned short port){
    if(!serv_addr || !ip){
        fprintf(stderr, "Argomenti non validi\n");
        exit(EXIT_FAILURE);
    }

    // CREA LA SOCKET TCP
    int* soc = malloc(sizeof(int));
    fatal_error();
    *soc = socket(AF_INET, SOCK_STREAM, 0); // ipv4, full-duplex, protocollo scelto automaticamente
    fatal_error();
    printf("Socket TCP creato con successo\n");

    // ASSOCIA LA SOCKET TCP
    socklen_t serv_len = sizeof(*serv_addr);
    memset(serv_addr, 0, serv_len); // azzera

    serv_addr->sin_family = AF_INET;
    serv_addr->sin_port = htons(port); // converte da host byte order a network byte order ed imposta la porta

    if(inet_pton(AF_INET, ip, &serv_addr->sin_addr) <= 0) { // converte un IP testuale in formato binario per la socket
        fprintf(stderr, "Indirizzo non raggiungibile\n");
        exit(EXIT_FAILURE);
    }
    printf("Socket TCP associato con successo a %s:%hu\n", ip, port);

    return soc;
}

void connect_socket(const int* soc, struct sockaddr_in* serv_addr){
    if(!soc || !serv_addr){
        fprintf(stderr, "Argomenti non validi\n");
        exit(EXIT_FAILURE);
    }

    connect(*soc, (struct sockaddr *) serv_addr, sizeof(*serv_addr));
    fatal_error();
    printf("Connesso con successo alla socket\n");
}


// ::::::::::::::::::::::::::::::::::::::::: TEST :::::::::::::::::::::::::::::::::::::::::::::::::
int main(int argc, char* args[]) {
    errno = 0;

    // PARAMETRI INIZIALI
    printf("[=============={ PARAMETRI }==============]\n");
    C_attr* c_a = init(argc, args);
    printf("Nome del file in input: %s\n", c_a->name);
    printf("Chiave: %llu\n", c_a->K);
    printf("Thread utilizzabili: %hu\n", c_a->p);

    char* ip = get_ip();
    printf("IP del client (io): %s\n", ip);
    printf("IP del server: %s\n", c_a->IP);
    printf("Porta del server: %hu\n", c_a->port);

    // TESTO DEL FILE
    FILE* file = file_open(c_a->name, "r");
    if(!file){
        fprintf(stderr, "Errore nell'apertura del file\n");
        exit(EXIT_FAILURE);
    }
    const size_t L = file_get_size(file);
    char* F = file_get_text(file);
    if(!F){
        fprintf(stderr, "Errore nella lettura del file\n");
        exit(EXIT_FAILURE);
    }
    if(file_close(file) != 0){
        fprintf(stderr, "Errore nella chiusura del file\n");
        exit(EXIT_FAILURE);
    }
    printf("\n[================{ TESTO }================]\n");
    printf("%s\n", F);

    // BLOCCHI
    Blocks* bl = create_blocks(F, L);
    if(!bl){
        fprintf(stderr, "Errore nella creazione dei blocchi\n");
        exit(EXIT_FAILURE);
    }
    free(F);
    
    printf("\n[==============={ BLOCCHI }===============]\n");
    print_blocks(bl);
    printf("\n");
    
    sigset_t block, oldset;
    block_signals(&block, &oldset); // blocca i segnali
    
    printf("\n[==============={ THREADS }===============]\n");
    if(thread_modify_blocks(bl, c_a->p, c_a->K) != 0){
        fprintf(stderr, "Errore nella gestione dei thread");
        exit(EXIT_FAILURE);
    }
    
    unblock_signals(&oldset); // sblocca i segnali
    
    printf("\n[==========={ BLOCCHI CIFRATI }===========]\n");
    print_blocks(bl);

    // TESTO CIFRATO
    printf("\n[============{ TESTO CIFRATO }============]\n");
    size_t c_len = bl->size * LEN_BLOCK;
    char C[c_len];
    memcpy(C, concatenate_blocks(bl), c_len);
    free_blocks(bl);
    print_text(C, c_len);

    // CONNESSIONE AL SERVER
    printf("\n[============={ CONNESSIONE }=============]\n");
    struct sockaddr_in serv_addr;
    int* soc = create_client_socket_tcp(&serv_addr, c_a->IP, c_a->port);
    connect_socket(soc, &serv_addr);
    
    // INVIO
    send_arg(*soc, &c_len, sizeof(c_len));
    send_arg(*soc, C, c_len);
    send_arg(*soc, &L, sizeof(L));
    send_arg(*soc, &c_a->K, sizeof(c_a->K));
    
    printf("Tutti i pacchetti inviati con successo\n");
    shutdown(*soc, SHUT_WR); // il client ha terminato di inviare ma aspetta l'ack del server
    fatal_error();
    
    // ASPETTA L'ACK PER POTER TERMINARE
    printf("In attesa dell'ACK del server\n");
    int ack = -1;
    while(ack != ACK)
        recive(*soc, &ack, sizeof(ack));
    printf("ACK rivevuto, chiusura della connessione avvenuta con successo\n");
    return 0;

    /* gcc funzioni.c client.c -o client
    ./client filename chiave thread-cifratura IP-server porta-server*/
}