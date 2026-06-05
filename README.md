---

# 🔒 C-Encrypted-Concurrent-Sockets

Questo progetto è un'applicazione client-server ad alta concorrenza sviluppata in C. Dimostra l'utilizzo avanzato di **socket TCP** per la comunicazione di rete, **pthreads** sia per l'elaborazione parallela che per la gestione multi-client, e **mutex** per la thread-safety.

L'applicazione permette a un client di leggere un file, cifrarlo parallelamente utilizzando un cifrario XOR a 64-bit, e inviare i dati binari al server. Il server, **progettato per gestire connessioni multiple simultaneamente**, riceve i dati, li decifra in parallelo e salva il contenuto in chiaro su un nuovo file formattato con timestamp.

---

## 🎯 Funzionalità Principali

* **Architettura Client-Server Multipla**: Utilizza socket TCP standard in modalità non bloccante (`O_NONBLOCK`) sul server per accettare connessioni continue.
* **Gestione Concorrente dei Client (Extra)**: A differenza delle implementazioni base, il server gestisce fino a `l` client simultanei. Per ogni connessione accettata, viene generato un thread distaccato (`pthread_detach`) per gestire in modo asincrono la comunicazione, mentre il thread principale continua ad ascoltare senza interrompersi.
* **Cifratura Parallela (Client)**: Il client utilizza un numero specificato (`p`) di thread per suddividere il file in blocchi da 8 byte (64-bit) e cifrarli in modo concorrente tramite un'operazione XOR.
* **Decifratura Parallela (Server)**: Il server riceve il testo cifrato e utilizza a sua volta `p` thread per decifrare i blocchi in parallelo, prima di ricostruire il file originale rimuovendo eventuale padding.
* **Trasmissione Binaria Sicura**: I dati vengono inviati e ricevuti in formato binario puro, garantendo l'integrità attraverso l'invio preventivo delle dimensioni esatte del pacchetto (`c_len` e la lunghezza del testo originale `L`).
* **Protezione dei Segnali e Risorse Condivise**: Il server blocca attivamente i segnali (es. `SIGINT`, `SIGTERM`) tramite `block_signals()` durante l'elaborazione dei client per evitare corruzioni. Inoltre, un `pthread_mutex_t` protegge l'accesso alla variabile globale `cli_count`.

---

## 🧩 Architettura e Componenti Principali

Il progetto è modulato su tre componenti principali (con i rispettivi header):

* `client.c` / `client.h`: Gestisce la logica del client. Inizializza i parametri di connessione, gestisce la lettura del file, coordina i thread per la cifratura XOR, impacchetta i metadati (chiave, lunghezze) e li invia al server, mettendosi in attesa di un segnale di `ACK`.
* `server.c` / `server.h`: Il core del server. Implementa un ciclo di ascolto continuo. Quando arriva una connessione, usa `accept_client` e delega la richiesta a `client_handler` tramite un nuovo thread. Si occupa infine di scrivere il risultato decifrato su un file di output generato dinamicamente (IP + Timestamp).
* `funzioni.c` / `funzioni.h`: Una libreria condivisa potente che astrae la logica di base per client e server. Contiene le operazioni sui file (`file_open`, `file_get_text`), la suddivisione e parallelizzazione in blocchi (`create_blocks`, `thread_modify_blocks`), il reperimento dell'IP locale (`get_ip`), l'invio/ricezione sicura su socket (`send_arg`, `recive`) e la maschera dei segnali.

---

## ⚙️ Come Funziona: Il Flusso dei Dati

### 1. Lato Client (Cifratura e Invio)

1. **Configurazione Iniziale**: Il client acquisisce via linea di comando il file di input, una chiave di 8 byte (`K`), il livello di parallelismo (`p`), IP e porta del server (salvati in `C_attr`).
2. **Elaborazione e Cifratura**:
* Il file viene caricato in memoria e suddiviso in blocchi da 8 byte tramite `create_blocks`. Se l'ultimo blocco è incompleto, viene riempito con padding (`\0`).
* Viene richiamata `thread_modify_blocks`, che istanzia `p` thread.
* Ogni thread elabora un sottoinsieme di blocchi eseguendo un'operazione XOR a 64-bit tra il blocco e la chiave (`targ->bl->blocks[i] ^= *targ->K`).


3. **Trasmissione**:
* I blocchi cifrati vengono ri-concatenati in un array binario.
* Il client invia sequenzialmente al server: la dimensione del pacchetto (`c_len`), il buffer cifrato (`C`), la lunghezza del file originale (`L`) e la chiave (`K`).
* Esegue uno `shutdown` in scrittura e attende l' `ACK` dal server per terminare.



### 2. Lato Server (Ricezione e Decifratura)

1. **Configurazione Iniziale**: Il server inizializza i parametri con numero di thread per client (`p`), prefisso file (`s`) e connessioni massime (`l`). La porta TCP è impostata rigidamente a `49153`.
2. **Gestione Connessioni**:
* Il socket è impostato su `O_NONBLOCK`. Il loop principale accetta connessioni finché `cli_count < l`.
* Per ogni connessione, viene lanciato `create_client_thread` che delega l'intero processo a `client_handler`, permettendo al server di scalare e ascoltare altri client.


3. **Decifratura**:
* Il thread del client riceve i metadati e ricostruisce i blocchi in base a `c_len`.
* Riapplica in parallelo (con `p` thread) la medesima logica XOR, annullando la cifratura.
* I blocchi in chiaro vengono concatenati e il testo viene troncato alla dimensione originale `L` per rimuovere il padding.


4. **Scrittura e Conferma**:
* Viene inviato il codice `ACK` (42) al client per confermare la corretta esecuzione.
* Il risultato viene scritto in un file nominato secondo lo schema: `prefisso + IP_Client + : + Timestamp + .txt`.
* Il thread termina chiudendo il socket e decurtando in modo thread-safe `cli_count`.


---
## 💻 Istruzioni per l'Esecuzione

1. Compila il Server e il Client (è obbligatorio linkare la libreria `pthread`):

   ```sh
   # Compilazione Server
   gcc src/funzioni.c src/server.c -o server -Wall -Wextra -pthread
   
   # Compilazione Client
   gcc src/funzioni.c src/client.c -o client -Wall -Wextra -pthread
   ```


2. Avvia il Server:
*(Nota: La porta del server è preconfigurata nel codice a `49153`)*
   ```sh
   # Sintassi: ./server <thread-decifratura> <prefisso-output> <client-paralleli>
   # Esempio: 4 thread per client, prefisso "out_", massimo 10 connessioni
   ./server 4 "out_" 10
   ```


3. Avvia il Client in un altro terminale:
   ```sh
   # Sintassi: ./client <file> <chiave_esatta_8_byte> <thread-cifratura> <IP-server> <porta-server>
   # Esempio: f.txt, chiave "12345678", 4 thread, localhost, porta 49153
   ./client f.txt 12345678 4 "127.0.0.1" 49153

   ```



Il server processerà la richiesta e salverà un file simile a `out_127.0.0.1:2026-06-05;19:40:06.txt`.


## 🧰 Requisiti

* Compilatore C (`gcc` o `clang`)
* Libreria POSIX Threads (`pthread`)
* Sistema operativo Linux/Unix-like (per la gestione di `ifaddrs.h` e le API di socket networking).

## 👥 Autori

* Lorenzo Micci
* Filippo Pierbattisti
* Lorenzo Mercuri