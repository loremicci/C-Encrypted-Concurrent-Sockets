#include "server.h"


// ::::::::::::::::::::::::::::::::::::::: ATTRIBUTI ::::::::::::::::::::::::::::::::::::::::::::::
#define PORT_TCP 49153 // porte libere [49152-65535]
unsigned short cli_count = 0;
pthread_mutex_t mutex;

S_attr* init(int argc, char** args){
    if(argc != 4){
        perror("Errore: numero di argomenti errato\n");
        exit(EXIT_FAILURE);
    }

    // CREA LA STRUCT PER GLI ATTRIBUTI DEL SERVER
    S_attr *serv_attr = (S_attr*) malloc(sizeof(S_attr));
    fatal_error();
    *serv_attr = (S_attr) {0, {'\0'}, 0};

    // VALORIZZA GLI ATTRIBUTI
    serv_attr->p = atoi(args[1]);
    if(serv_attr->p <= 0){
        fprintf(stderr, "Errore: numero di thread non valido\n");
        exit(EXIT_FAILURE);
    }

    strcpy(serv_attr->s, args[2]);
    if(strlen(args[2]) > 255){
        fprintf(stderr, "Errore: prefisso troppo lungo\n");
        exit(EXIT_FAILURE);
    }

    serv_attr->l = atoi(args[3]);
    if(serv_attr->l <= 0){
        fprintf(stderr, "Errore: numero massimo di connessioni non valido\n");
        exit(EXIT_FAILURE);
    }

    return serv_attr;
}


// ::::::::::::::::::::::::::::::::::::::: BASE FILE :::::::::::::::::::::::::::::::::::::::::::::::
int file_write(FILE* f, const char* text){
    if(!f || !text) return 1;

    fprintf(f, "%s", text);

    return file_error(f);
}

int str_append(char** dst, const char* src){
    if(!dst || !src) return 1;
    
    size_t len_src = strlen(src);
    size_t len_dest = *dst ? strlen(*dst) : 0;
    size_t len_res = len_dest + len_src;

    *dst = (char*) realloc(*dst, (len_res + 1) * sizeof(char));
    if(error()) return 1;

    memcpy(*dst + len_dest, src, len_src);
    (*dst)[len_res] = '\0';

    return 0;
}

// ::::::::::::::::::::::::::::::::::::::: SOCKET :::::::::::::::::::::::::::::::::::::::::::::::::
int* create_server_socket_tcp(const char* ip, unsigned short port, unsigned short l){
    if(!ip){
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
    struct sockaddr_in serv_addr;
    socklen_t serv_len = sizeof(serv_addr);
    memset(&serv_addr, 0, serv_len); // azzera

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip); // converte da stringa a in_addr_t ed imposta l'IP
    serv_addr.sin_port = htons(port); // converte da host byte order a network byte order ed imposta la porta
    
    bind(*soc, (struct sockaddr*) &serv_addr, serv_len); // associa la socket all’IP e porta, (socket, struct, dimensione)
    fatal_error();
    printf("Socket TCP associato con successo %s:%hu\n", ip, port);

    // AVVIA L'ASCOLTO TCP
    listen(*soc, l); // socket, numero massimo di client
    fatal_error();
    int flags = fcntl(*soc, F_GETFL, 0); // flag precedenti + socket non bloccante
    fatal_error();
    fcntl(*soc, F_SETFL, flags | O_NONBLOCK); // imposta la socket su non bloccante
    fatal_error();
    printf("Ascolto TCP iniziato con successo\n");

    return soc;
}

int* accept_client(int* soc, char** cli_ip){
    if(!soc) return NULL;

    // CONNESSIONE DI UN NUOVO CLIENT
    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);

    int* client = malloc(sizeof(int)); // crea lo spazio per un nuovo client
    if(error()) return NULL;
    
    *client = accept(*soc, (struct sockaddr*) &cli_addr, &cli_len); // accetta una nuova connessione
    if(errno == EAGAIN || errno == EWOULDBLOCK){ // ignorare, semplicemente non ha connesioni attive
        errno = 0;
        return NULL;
    }
    if(error()) return NULL;

    *cli_ip = strdup(inet_ntoa(cli_addr.sin_addr));
    
    printf("\r                       ");
    fflush(stdout);
    printf("\n[================{ CLIENT }===============]");
    printf("\nConnessione accetta con successo da %s:%d\n", *cli_ip, ntohs(cli_addr.sin_port));
    
    return client;
}
char* get_timestamp(){
    char* result = (char*) malloc(20 * sizeof(char));
    if(error()) return NULL;
    
    time_t rawtime;
    struct tm* timeinfo;
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(result, 20, "%Y-%m-%d;%H:%M:%S", timeinfo);

    return result;
}

void close_handler(int client){
    if(client < 0) return;
    close(client);

    pthread_mutex_lock(&mutex);
    cli_count--;
    pthread_mutex_unlock(&mutex);
}

void* client_handler(void* args){
    if(!args) return NULL;
    
    T_c_data* t_arg = (T_c_data*) args;
    int client = *t_arg->client;
    free(t_arg->client);

    // ARGOMENTI DEL CLIENT RICEVUTI
    size_t c_len;
    recive(client, &c_len, sizeof(c_len));
    
    char* C = (char*) malloc(c_len * sizeof(char)); 
    if(error()){
        free(t_arg);
        close_handler(client);
        return NULL;
    }
    
    recive(client, C, c_len);
    
    size_t L;
    recive(client, &L, sizeof(L));
    
    unsigned long long K;
    recive(client, &K, sizeof(K));
    
    printf("\n[============={ ATTRIBUTI }=============]\n");
    printf("[Thread] Client ID: %d\n", client);
    printf("Chiave di decifratura: %llu\n", K);
    printf("Lunghezza del testo originale: %zu\n", L);
    printf("Lunghezza del testo cifrato: %ld\n", c_len);

    printf("\n[==========={ TESTO CIFRATO }===========]\n");
    print_text(C, c_len);
    
    // BLOCCHI
    Blocks* bl = create_blocks(C, c_len);
    if(!bl){
        fprintf(stderr, "Errore nella creazione dei blocchi\n");
        free(t_arg);
        free(C);
        close_handler(client);
        return NULL;
    }
    free(C);

    printf("\n[==============={ THREADS }===============]\n");
    if(thread_modify_blocks(bl, t_arg->p, K) != 0){
        free(t_arg);
        free_blocks(bl);
        close_handler(client);
        return NULL;
    }

    // TESTO DECIFRATO
    printf("\n[==========={ TESTO DECIFRATO }===========]\n");

    char* D = (char*) malloc((L + 1) * sizeof(char));
    if(error()){
        free_blocks(bl);
        free(t_arg);
        close_handler(client);
        return NULL;
    }

    char* concat = concatenate_blocks(bl);
    if(!concat){
        fprintf(stderr, "Errore nella concatenazione dei blocchi\n");
        free_blocks(bl);
        free(t_arg);
        free(D);
        close_handler(client);
        return NULL;
    }

    memcpy(D, concat, L);
    free(concat);
    D[L] = '\0';

    free_blocks(bl);
    printf("%s\n", D);

    // ACK
    int ack = ACK;
    send_arg(client, &ack, sizeof(ack)); // invia l'ack al client di ricezione e decifrazione avvenuta con successo

    // FILE OUTPUT
    char* name = strdup(t_arg->s);
    if(error()){
        close_handler(client);
        free(t_arg);
        free(D);
        return NULL;
    }

    if(str_append(&name, t_arg->ip) != 0){
        fprintf(stderr, "Errore nella concatenazione dell'IP del client al nome del file\n");
        free(t_arg);
        free(name);
        close_handler(client);
        return NULL;
    }

    free(t_arg);

    if(str_append(&name, ":") != 0){
        fprintf(stderr, "Errore nella concatenazione del ':' al nome del file\n");
        free(name);
        close_handler(client);
        return NULL;
    }

    char* timestamp = get_timestamp();
    if(!timestamp){
        fprintf(stderr, "Errore nella generazione del timestamp\n");
        free(name);
        free(D);
        close_handler(client);
        return NULL;
    }
    
    if(str_append(&name, timestamp) != 0){
        fprintf(stderr, "Errore nella concatenazione del timestamp al nome del file\n");
        free(timestamp);
        free(name);
        free(D);
        close_handler(client);
        return NULL;
    }

    free(timestamp);

    if(str_append(&name, ".txt") != 0){
        fprintf(stderr, "Errore nella concatenazione dell'estensione al nome del file\n");
        free(name);
        free(D);
        close_handler(client);
        return NULL;
    }

    FILE* file = file_open(name, "w"); // crea tutto il percorso se non esiste
    if(!file){
        fprintf(stderr, "Errore nell'apertura del file\n");
        free(name);
        free(D);
        close_handler(client);
        return NULL;
    }

    
    if(file_write(file, D) != 0){
        fprintf(stderr, "Errore nella scrittura del file\n");
        free(D);
        close_handler(client);   
        return NULL;
    }

    free(D);

    if(file_close(file) != 0){
        fprintf(stderr, "Errore nella chiusura del file\n");
        close_handler(client);
        return NULL;
    }
    
    close_handler(client);
    printf("\n>>> File di output creato con successo: %s\n\n", name);
    free(name);

    return NULL;
}

void create_client_thread(int* client, unsigned short p, const char* s, char* cli_id){
    if(!client) return;

    // ARGOMENTI GESTIONE CLIENT
    T_c_data* th_data = (T_c_data*) malloc(sizeof(T_c_data));
    if(error()) return;

    th_data->client = client;
    th_data->p = p;
    strcpy(th_data->s, s);
    strcpy(th_data->ip, cli_id);

    // GESTIONE THREAD CLIENT
    pthread_t th;
    if(error()) return;

    if(pthread_create(&th, NULL, &client_handler, th_data) != 0){ // nuovo thread per il client
        fprintf(stderr, "Errore nella creazione del thread per il client\n");
        return;
    }

    pthread_detach(th);
}


// ::::::::::::::::::::::::::::::::::::::::: TEST :::::::::::::::::::::::::::::::::::::::::::::::::
int main(int argc, char* args[]){
    errno = 0;

    // PARAMETRI INIZIALI
    printf("[=============={ PARAMETRI }==============]\n");
    S_attr* serv_attr = init(argc, args);
    printf("Thread per ogni client: %hu\n", serv_attr->p);
    printf("Prefisso file output: %s\n", serv_attr->s);
    printf("Massime connessioni in parallelo: %hu\n", serv_attr->l);

    char* ip = get_ip();
    printf("IP server: %s\n", ip);
    printf("Porta in ascolto dal server: %hu\n", PORT_TCP);

    printf("\n[============={ CONNESSIONE }=============]\n");
    int* soc = create_server_socket_tcp(ip, PORT_TCP, serv_attr->l);

    // MENTRE ASCOLTA
    const char* dots[] = {"", ".", "..", "..."};
    size_t len_dots = sizeof(dots)/ sizeof(dots[0]), dots_state = 0;
    pthread_mutex_init(&mutex, NULL);

    int block_sig = 0;
    sigset_t block, oldset;
    
    while(1){
        errno = 0;
        if(serv_attr->l - cli_count > 0){ // se ha connessioni disponibili cerca client
            char* cli_ip;
            int* client = accept_client(soc, &cli_ip);
            if(client != NULL){
                if(block_sig != 1){
                    block_sig = 1;
                    block_signals(&block, &oldset); // blocca i segnali
                }

                cli_count++;
                create_client_thread(client, serv_attr->p, serv_attr->s, cli_ip);
                free(cli_ip);
            }
        } else{
            printf("\rMassimo numero di client simultanei raggiunto");
            fflush(stdout);
        }

        if(cli_count == 0){ // se non sta eseguendo nessun client
            if(block_sig != 0){
                block_sig = 0;
                unblock_signals(&oldset); // sblocca i segnali
            }
            
            printf("\rAspettando un client%-3s", dots[dots_state]);
            fflush(stdout);
            dots_state = (++dots_state) % len_dots;
        }

        usleep(500000); // aspetta 0,5 secondi
    }

    return 0;

    /* gcc funzioni.c server.c -o server
    ./server thread-decifratura prefisso client-paralleli */
}