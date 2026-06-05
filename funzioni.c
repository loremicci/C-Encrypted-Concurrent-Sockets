#include "funzioni.h"

// :::::::::::::::::::::::::::::::::::::::::: ERRORI ::::::::::::::::::::::::::::::::::::::::::::::
void fatal_error(){
    if(errno != 0){
        perror("");
        exit(EXIT_FAILURE);
    }
}

int error(){
    if(errno != 0){
        perror("");
        errno = 0;
        return 1;
    }
    return 0;
}

void file_fatal_error(FILE* f){
    if(!f) return;

    if(ferror(f)){
        perror("");
        exit(EXIT_FAILURE);
    }
}

int file_error(FILE* f){
    if(!f) return 1;

    if(ferror(f)){
        perror("");
        return 1;
    }

    return 0;
}

void block_signals(sigset_t* block, sigset_t* oldset){
    sigemptyset(block);
    sigaddset(block, SIGINT);
    sigaddset(block, SIGALRM);
    sigaddset(block, SIGUSR1);
    sigaddset(block, SIGUSR2);
    sigaddset(block, SIGTERM);
    sigprocmask(SIG_BLOCK, block, oldset);
    
    fatal_error();
}

void unblock_signals(sigset_t* oldset){
    sigprocmask(SIG_SETMASK, oldset, NULL);
    
    fatal_error();
}


// ::::::::::::::::::::::::::::::::::::::: ATTRIBUTI ::::::::::::::::::::::::::::::::::::::::::::::
char* get_ip(){
    struct ifaddrs *ifaddr, *ifa;
    getifaddrs(&ifaddr); // ottiene la lista di interfaccie di rete attive
    if(error()){
        freeifaddrs(ifaddr);
        fprintf(stderr, "Errore: mancanza di un indirizzo IP valido\n");
        exit(EXIT_FAILURE);
    }

    for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next){ // scorre tutte le interfacce
        if((ifa->ifa_addr != NULL) && // controlla che l'interfaccia abbia IP
           ((ifa->ifa_flags & FLAG) == FLAG) && // controlla la presenza di tutti i flag
           (ifa->ifa_addr->sa_family == AF_INET)){ // controlla che l'indirizzo sia IPv4
                char* ip = malloc(INET_ADDRSTRLEN); // dimensione standard IPv4
                fatal_error();

                struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
                inet_ntop(AF_INET, &sa->sin_addr, ip, INET_ADDRSTRLEN); // converte l'IP in stringa e lo salva in ip
                
                freeifaddrs(ifaddr);
                return ip;
            }
    }

    freeifaddrs(ifaddr);
    fprintf(stderr, "Errore: mancanza di un indirizzo IP valido\n");
    exit(EXIT_FAILURE);
}


// ::::::::::::::::::::::::::::::::::::::: BASE FILE ::::::::::::::::::::::::::::::::::::::::::::::
FILE* file_open(const char* path, const char* mod){
    if(!path) return NULL;

    FILE* f = fopen(path, mod);
    if(error()) return NULL;
    
    return f;
}

int file_close(FILE* f){
    if(!f) return 1;

    fclose(f);
    return error();
}


// ::::::::::::::::::::::::::::::::::::::: CIFRATURA ::::::::::::::::::::::::::::::::::::::::::::::
void print_text(const char* text, const size_t len){
    if(!text) return;

    for(size_t i = 0; i < len; ++i) printf("%c", text[i]);
}

Blocks* create_blocks(const char* F, const size_t L){
    if(!F || L == 0) return NULL;

    size_t n_blocks = L / LEN_BLOCK;
    size_t extra = L % LEN_BLOCK;

    Blocks* bl = (Blocks*) malloc(sizeof(Blocks));
    if(error()) return NULL;
    bl->size = n_blocks + (extra > 0 ? 1 : 0);
    bl->blocks = (char**) malloc((bl->size) * sizeof(char*));
    if(error()) return NULL;

    for(size_t i = 0; i < n_blocks; ++i){ // divide in blocchi
        bl->blocks[i] = (char*) malloc(LEN_BLOCK * sizeof(char));
        if(error()) return NULL;
        memcpy(bl->blocks[i], F + i * LEN_BLOCK, LEN_BLOCK);
    }

    if(extra > 0){ // ultimo blocco
        bl->blocks[n_blocks] = malloc(LEN_BLOCK * sizeof(char));
        if(error()) return NULL;
        memcpy(bl->blocks[n_blocks], F + n_blocks * LEN_BLOCK, extra);
        memset(bl->blocks[n_blocks] + extra, '\0', LEN_BLOCK - extra); // padding
    }
    
    return bl;
}

void print_blocks(const Blocks* bl){
    if(!bl || bl->size == 0) return;

    for(size_t b = 0; b < bl->size; ++b){
        printf(">>> Blocco %zu:\n", b + 1);
        print_text(bl->blocks[b], LEN_BLOCK);
        printf("\n");
    }
}

char* concatenate_blocks(Blocks* bl){
    if(!bl || bl->size == 0) return NULL;

    char* res = (char*) malloc((bl->size * LEN_BLOCK + 1) * sizeof(char));
    if(error()) return NULL;
    for(size_t i = 0; i < bl->size; ++i)
        memcpy(res + (i * LEN_BLOCK), bl->blocks[i], LEN_BLOCK);

    res[bl->size * LEN_BLOCK] = '\0';

    return res;
}

void* modify_blocks(void* args){
    if(!args) return NULL;
    
    T_data* targ = (T_data*) args; // Cifratura con lo xor

    for(size_t i = targ->start; i < targ->end; ++i){
        unsigned long long tmp;
        memcpy(&tmp, targ->bl->blocks[i], LEN_BLOCK);
        tmp ^= *targ->K; // xor
        memcpy(targ->bl->blocks[i], &tmp, LEN_BLOCK);
    }

    free(targ);
    return NULL;
}

int thread_modify_blocks(Blocks* bl, const unsigned short p, const unsigned long long K){
    if(!bl || !bl->blocks || bl->size == 0) return 1;

    pthread_t* threads = calloc(p, sizeof(pthread_t));
    if(error()) return 1;

    size_t chunk = bl->size / p; // quanti blocchi ogni thread p si prende in carico (manca l'extra)
    size_t extra = bl->size % p;

    size_t start = 0;
    
    for(unsigned short i = 0; i < p; ++i){
        size_t blocchi = (i < extra) ? chunk + 1 : chunk; // blocchi per thread

        T_data* th_data = malloc(sizeof(T_data));
        if(error()) return 1;
        th_data->bl = bl;
        th_data->start = start;
        th_data->end = start + blocchi;
        th_data->K = &K;
        
        printf("Tid: %d, numero blocchi: %zu\n", i, blocchi);
        if(pthread_create(&threads[i], NULL, &modify_blocks, th_data) != 0) // crea i thread
            return 1;

        start += blocchi;  // aggiorna per il prossimo thread
    }

    for(unsigned short i = 0; i < p; ++i){ // aspetta tutti i thread
        if(pthread_join(threads[i], NULL) != 0)
            return 1;
    }
    
    free(threads);

    return 0;
}

void free_blocks(Blocks* bl){
    if(!bl || !bl->blocks || bl->size == 0) return;

    for(size_t b = 0; b < bl->size; ++b)
        free(bl->blocks[b]);
    free(bl->blocks);
}


// ::::::::::::::::::::::::::::::::::::::: SOCKET :::::::::::::::::::::::::::::::::::::::::::::::::
int recive(const int soc, void* arg, const size_t size){
    if(!arg || size == 0) return 1;

    size_t sent = 0;
    
    while(sent < size){
        ssize_t sent_now = recv(soc, arg + (sent * size), size - sent, 0); // riceve, (socket, puntatore al buffer, dimensione del buffer, flag)
        if(error()) return 1;
        sent += sent_now;
    }
    return 0;
}

int send_arg(const int soc, const void* arg, const size_t size){
    if(!arg || size == 0) return 1;

    size_t sent = 0;
    
    while(sent < size){
        ssize_t sent_now = send(soc, arg + (sent * size), size - sent, 0); // invia, (socket, puntatore al buffer, dimensione del buffer, flag)
        if(error()) return 1;
        sent += sent_now;
    }
    return 0;
}