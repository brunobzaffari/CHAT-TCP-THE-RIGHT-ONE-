#include <sys/select.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
//#define INATIVO "INATIVO"// Para o comando sai
//#define ATIVO "ATIVO"// Para o comando sai
#define MAX_LOGIN 3// Valor máximo de "Clientes logados"
#define MAX 6// Valor máximo de Clientes aceitos pelo listen do sock7 e sock8
int count_client;// Controle de quantos Clientes
// OBSERVAÇÃO: "Clientes logados" = Clientes com nome;
// Segmenta de acordo com a func----------------------------------------------------------------------
bool dessegmenta(char *envio, const char *nome_arq, const char *ext, int tam);
// Dessegmenta de acordo com a func(Para o envio de arq)----------------------------------------------
bool segmenta_recebeporta(char *recebeporta, char *cod, char *nome, char *mensagem, int *tam, char *ext, char *nome_arq);
//----------------------------------- Struct --------------------------------------------------------
typedef struct ClientNode ClientNode;

struct ClientNode {
    char ip_address[16];
    char nome[10];
    int client_sock7;
    int client_sock8;
    pthread_t thread;
    ClientNode* next;
};

ClientNode* head = NULL;// Cabeça da lista ligada
//---------------------------------------------------------------------------------------------------
void print_conexoes();
void print_logs();
int login_list_size();
int list_size();
void *handle_client(void *node);
void remove_client(char *ip_address);
bool insert_client_generic(char *ip_address, int sock7, int sock8);
bool refresh_client(char *ip_address, int sock7, int sock8);
bool name_not_in_list(const char *nome);
ClientNode* find_client_by_nome(const char *nome);
ClientNode* is_client_in_list(const char *ip_address);
void clear_client_list();
void send_to_all_clients(const void *envio, size_t len);
// double current_time_in_seconds();


//---------------------------------------------------------------------------


int main() {
    struct sockaddr_in server_address7, server_address8;
    struct sockaddr_in client_address;
    int client_sock7=0, client_sock8=0, sock7=0, sock8=0; 
    char client_ip2[16];// Buffer para armazenar o endereço IP em formato string
    char client_ip[16];// Buffer para armazenar o endereço IP em formato string
    int controle_a = 0;
    char nome_cli[20];
    ClientNode *temp;
    socklen_t len, len2;
    //int size;
    clear_client_list();// Pra ter certeza que comecara vazia
    count_client = 0;
    memset(&server_address8, 0, sizeof(server_address8));
    memset(&server_address7, 0, sizeof(server_address7));
    //----------------------------------- Cria Sokets ----------------------------
    sock7 = socket(AF_INET, SOCK_STREAM, 0);// Criar o socket 7 (porta 7) 50007 --
    if(sock7 == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    server_address7.sin_family = AF_INET;
    server_address7.sin_port = htons(50007);
    server_address7.sin_addr.s_addr = INADDR_ANY;
    if(bind(sock7, (struct sockaddr*)&server_address7, sizeof(server_address7)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if(listen(sock7, MAX)== -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }// ---------------------------------- Fim Criar o socket7 (porta 7) 50007 ---
    sock8 = socket(AF_INET, SOCK_STREAM, 0);// Criar o socket8 (porta 8) 50008 ---
    if(sock8 == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    server_address8.sin_family = AF_INET;
    server_address8.sin_port = htons(50008);
    server_address8.sin_addr.s_addr = INADDR_ANY;
    if(bind(sock8, (struct sockaddr*)&server_address8, sizeof(server_address8)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if(listen(sock8, MAX)== -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }// ---------------------------------- Fim Criar o socket8 (porta 8) 50008 ---
    // -------------------------------- Fim Cria Sokets --------------------------
    while(1){// ------------------------------ while -----------------------------
        memset(&client_sock7, 0, sizeof(client_sock7));
        memset(&client_sock8, 0, sizeof(client_sock8));
        len = sizeof(client_address);//------------- Aceita Cliente -----------------
        if((client_sock7 = accept(sock7, (struct sockaddr *)&client_address, &len))<0){// Escutar na porta 7 50007
            perror("accept sock7");
            exit(EXIT_FAILURE);
        }
        else{
            inet_ntop(AF_INET, &(client_address.sin_addr), client_ip, sizeof(client_ip));// Guarda o ip do cliente
            printf("ID: %s\n", client_ip);
            printf("%d\n", client_sock7);
            usleep(9);
            len2 = sizeof(client_address);
            if((client_sock8 = accept(sock8, (struct sockaddr *)&client_address, &len2))<0){// Escutar na porta 8 50008
                perror("accept sock8");
                exit(EXIT_FAILURE);
            }
            else{
                inet_ntop(AF_INET, &(client_address.sin_addr), client_ip2, sizeof(client_ip2));// Guarda o ip do cliente
                printf("ID: %s\n", client_ip2);
                printf("%d\n", client_sock8);
                if((strcmp(client_ip, client_ip2) == 0)){// Para garantir q o mesmo accept do 7 esta se conectando com o 8
                    send(client_sock8, "1", 1, 0);
                    count_client++;
                    controle_a = 1;
                }
                else {// Aceitou o 8, mas não são iguais
                    send(client_sock8, "0", 1, 0);
                    close(client_sock8);
                    close(client_sock7);
                }
            }
        }
        // ------------------------ Fim Aceita Cliente ---------------------
        if(controle_a==1){
            // Atualiza sockets em cliente e recria thread ------------------
            //printf("opa1\n");// Debug
            temp = is_client_in_list(client_ip);
            if(temp != NULL) {// Estava conectado
                //printf("opa2\n");// Debug
                refresh_client(client_ip, client_sock7, client_sock8);
                if(temp->nome[0]!= '\0'){// Já estava logado
                    //printf("opa3\n");// Debug
                    send(client_sock7, "1", 1, 0);// Já estava logado
                    strcpy(nome_cli, temp->nome);
                    send(client_sock7, nome_cli, strlen(temp->nome) + 1, 0);
                    memset(nome_cli, 0, sizeof(nome_cli));
                    controle_a = 0;
                }
                else{
                    //printf("opa4\n");// Debug
                    send(client_sock7, "0", 1, 0);// Não estava conectado
                    controle_a = 0;
                }
                
            }
            else {// Cira cliente sem nome ----------------------------------
                //printf("opa5\n");// Debug
                send(client_sock7, "0", 1, 0);// Não estava conectado
                //printf("client_sock7 %d\n", client_sock7);// Debug
                //printf("client_sock8 %d\n", client_sock8);// Debug
                insert_client_generic(client_ip, client_sock7, client_sock8);
                print_conexoes();
                controle_a = 0;
            }
        }
    }// ----------------------------------- Fim while ---------------------------
    close(sock7);
    close(sock8);
    return 0;
}
void print_logs() {
    ClientNode* temp = head;
    int count = 1;// Para numerar os clientes logados
    if(head == NULL) {
        printf("Lista de clientes está vazia.\n");
        return;
    }
    printf("\n------------ Lista de Clientes Logados: -----------\n");
    printf("---------------------------------------------------\n");
    while(temp != NULL) {
        if(temp->nome[0] != '\0') {
            printf("Cliente %d:\n", count);
            //printf("Estado: %s\n", temp->state);
            printf("Endereço IP: %s\n", temp->ip_address);
            printf("Nome: %s\n", temp->nome);
            printf("---------------------------------------------------\n");
        }
        count++;
       temp = temp->next;
    }
}
void print_conexoes() {
    ClientNode* temp = head;
    int count = 1;// Para numerar os clientes conectados
    if(head == NULL) {
        printf("Lista de conexoes está vazia.\n");
        return;
    }
    printf("\n---------------- Lista de Conexões: ---------------\n");
    printf("---------------------------------------------------\n");
    while(temp != NULL) {
        printf("Conexao: %d:\n", count);
        //printf("Estado: %s\n", temp->state);
        printf("Endereço IP: %s\n", temp->ip_address);
        printf("Nome: %s\n", temp->nome);
        printf("Client socket 7: %d\n", temp->client_sock7);
        printf("Client socket 8: %d\n", temp->client_sock8);
        printf("Thread: %ld\n", temp->thread);                                   
        printf("---------------------------------------------------\n");
        count++;
        temp = temp->next;
    }
}
void remove_client(char *ip_address) {
    ClientNode *temp = head, *prev;
    char string[1500];
    pthread_t t;
    int controle = 0;
    if((temp != NULL) && (strcmp(temp->ip_address, ip_address) == 0)) {
        if(temp->nome[0]!='\0') controle=1;// Ve se esta logado
        t = temp->thread;
        sprintf(string, "----------------> Remoção de cliente\n Cliente ip: %s Nome: %s\n\n", temp->ip_address, temp->nome);
        printf("%s", string);
        memset(temp->nome, 0, sizeof(temp->nome));
        head = temp->next;
        close(temp->client_sock8);
        close(temp->client_sock7);
        if(controle ==1) print_logs();
        else print_conexoes();
        free(temp);
        pthread_cancel(t);
        pthread_join(t, NULL);
        count_client--;
        return;
    }
    while(temp != NULL && strcmp(temp->ip_address, ip_address) != 0) {
        prev = temp;
        temp = temp->next;
    }
    if(temp == NULL) return;
    
    if(temp->nome[0]!='\0') controle=1;// Ve se esta logado
    t = temp->thread;
    sprintf(string, "----------------> Remoção de cliente\n Cliente ip: %s Nome: %s\n", temp->ip_address, temp->nome);
    printf("%s", string);
    memset(temp->nome, 0, sizeof(temp->nome));
    prev->next = temp->next;
    close(temp->client_sock8);
    close(temp->client_sock7);
    free(temp);
    if(controle ==1) print_logs();
    else print_conexoes();
    pthread_cancel(t);
    pthread_join(t, NULL);
    count_client--;
}
bool insert_client_generic(char *ip_address, int sock7, int sock8) {// Cria cliente genérico
    ClientNode* new_node = (ClientNode*)malloc(sizeof(ClientNode));
    //pthread_t t;
    if(new_node == NULL) return false;
    strcpy(new_node->ip_address, ip_address);// Copiar ip_address
    new_node->client_sock7 = sock7;
    new_node->client_sock8 = sock8;
    if(pthread_create(&(new_node->thread), NULL, handle_client, (void*)new_node) < 0) {
            remove_client(ip_address);
            perror("Could not create thread");
            return false;
    }
    //printf("%d\n", (int)new_node->thread);// Debug
    new_node->next = head;
    head = new_node;
    return true;
}
bool refresh_client(char *ip_address, int sock7, int sock8) {// Se um cliente tentar conexão mas ele já esta inserido
    ClientNode* temp = head;// Inicia o ponteiro temporário na cabeça da lista
    while(temp != NULL) {// Fecha sockets antigos, muda pelos novos e exclui tread antiga e faz uma nova
        if(strcmp(temp->ip_address, ip_address) == 0) {// Compara o endereço IP
            close(temp->client_sock8);
            close(temp->client_sock7);
            pthread_cancel(temp->thread);
            temp->client_sock7 = sock7;
            temp->client_sock7 = sock8;
            if(pthread_create(&(temp->thread), NULL, handle_client, (void*)temp) < 0) {
                remove_client(ip_address);
                perror("Could not create thread");
                return false;
            }
            return true;
        }
        temp = temp->next;// Move para o próximo nó
    }
    return false;// Retorna falso se o IP não for encontrado  
}
void send_to_all_clients(const void *envio, size_t len) {// Função para enviar um broadcast para todos os clientes na lista
    ClientNode* temp = head;
    do{
        if(temp->nome[0]!= '\0') {// Manda pra todos os "Clientes logados"
            if((send(temp->client_sock8, envio, len, 0)) == -1) {
                printf("Falha na conexão com o cliente %s <%s>\n", temp->ip_address, temp->nome);
                fflush(stdout);// Isso força a mensagem a aparecer no terminal imediatamente.
                remove_client(temp->ip_address);
            }// Checar por erros
            //else printf("Enviado %s", envio);
        }
        
        temp = temp->next;
        
    }while(temp != NULL);
}
void *handle_client(void *node) {
    // double start_time = current_time_in_seconds();
    // double end_time = start_time + 600;// Define o tempo limite para 60 segundos
    ClientNode *cli = (ClientNode*)node;// Desreferenciação
    int client_sock7 = cli->client_sock7;
    int client_sock8 = cli->client_sock8;
    char recebeporta7[128];
    char recebeporta8[128];
    ssize_t bytes, bytes2;
	char client_ip[16];// Buffer para armazenar o endereço IP em formato string
    char mensagem[112];// Mensagens mensagens de texto de clientes
    char envio[1501];// Mensagens para enviar
    fd_set readfds;
    char nome[10];
    char cod[5];
    // Para recebimento de arquivo ------------------------------------------
    char arquivo[1501];
    char nome_arq[20];
    char ext[10];
    int tam;
    int len;
    strcpy(client_ip, cli->ip_address);
    //print_conexoes();
    // printf("Cliente %s conectado\n", client_ip);
    // printf("client_sock7 %d\n", client_sock7);
    // printf("client_sock8 %d\n", client_sock8);
    // printf("Thread: %d\n", (int)cli->thread);
    int max_sock = (client_sock7 > client_sock8 ? client_sock7 : client_sock8) + 1;
    while(1) {
        // Select -----------------------------------------------------------
        FD_ZERO(&readfds);
        FD_SET(client_sock7, &readfds);
        FD_SET(client_sock8, &readfds);        
        // Zera variaveis para garantir -------------------------------------
        memset(recebeporta7, 0, sizeof(recebeporta7));
        memset(recebeporta8, 0, sizeof(recebeporta8));
        memset(mensagem, 0, sizeof(mensagem));
        memset(envio, 0, sizeof(envio));
        memset(nome, 0, sizeof(nome));
        memset(cod, 0, sizeof(cod));
        bytes2 = -2;
        bytes = -2;

        if(select(max_sock, &readfds, NULL, NULL, NULL) < 0) {// Select esperar por atividade num dos sockets
            perror("Erro no select");
            exit(EXIT_FAILURE);
        }
        if(FD_ISSET(client_sock7, &readfds)) {// Gerenciamentode cliente na porta 50007 /lin/sai/out
            len = recv(client_sock7, recebeporta7, sizeof(recebeporta7), 0);// Recebe texto (cliente -> 7)
            // printf("Recebido na porta 50007: %s\n", recebeporta7);// Debug
            // printf("Mensagem recebida de %s\n", client_ip);// Debug
            if(len <= 0) {
                perror("recv client_sock7");
                printf("\nFalha na conexão com o cliente %s <%s>\n", client_ip, cli->nome);
                fflush(stdout);// Isso força a mensagem a aparecer no terminal imediatamente.
                remove_client(client_ip);
                }
            else{
                printf("Recebido na porta 50007: %s\n", recebeporta7);// Debug
                printf("Mensagem recebida de %s\n", client_ip);// Debug
                segmenta_recebeporta(recebeporta7, cod, nome, mensagem, &tam, ext, nome_arq);
                if(strcmp(cod, "/lin") == 0) {// LIN ----------------------------
                    if((cli->nome[0])=='\0') {
                        if(login_list_size()<= MAX_LOGIN) {// Se não chegou no tamanho máximo (Definido como número máximo da lista 3).
                            if(name_not_in_list(nome)) {
                                printf("----------------> Inserção de cliente!\n");
                                strcpy(cli->nome, nome);// Salva o nome
                                print_logs();
                                if(send(client_sock7, "2", 1, 0) == -1) {// Código 2 de controle para avisar q foi inserido 
                                    printf("Falha na conexão com o cliente %s <%s>\n", client_ip, cli->nome);
                                    fflush(stdout);// Isso força a mensagem a aparecer no terminal imediatamente.
                                    remove_client(client_ip);
                                    
                                    //exit(EXIT_FAILURE);
                                }
                                else{
                                    sprintf(envio, "Você esta conectado com o nome %s", nome);
                                    if(send(client_sock7, envio, strlen(envio), 0) == -1){
                                        printf("Falha na conexão com o cliente %s <%s>\n", client_ip, cli->nome);
                                        fflush(stdout);// Isso força a mensagem a aparecer no terminal imediatamente.
                                        remove_client(client_ip);
                                        //exit(EXIT_FAILURE);
                                    }
                                }
                            }
                            else {// Nome Já em uso
                                if(send(client_sock7, "9", 1, 0) == -1) {// Código 9 de controle para avisar q o nome selecionado já esta em uso 
                                    printf("Falha na conexão com o cliente %s\n", client_ip);
                                    fflush(stdout);// Isso força a mensagem a aparecer no terminal imediatamente.
                                    remove_client(client_ip);
                                    //exit(EXIT_FAILURE);
                                }
                            }
                        }
                        else {// Lista cheia
                            if(send(client_sock7, "3", 1, 0) == -1) {// Código 3 de controle para avisar q a lista esta cheia 
                                printf("Falha na conexão com o cliente %s\n", client_ip);
                                fflush(stdout);// Isso força a mensagem a aparecer no terminal imediatamente.
                                remove_client(client_ip);
                                //exit(EXIT_FAILURE);
                            }
                        }
                    }
                    else {// Já esta na lista nome diferente de NULL
                        if(send(client_sock7, "4", 1, 0) == -1) {// Código 4 pra avisa q já esta na lista (cliente <-7)
                            printf("Falha na conexão com o cliente %s <%s>\n", client_ip, cli->nome);
                            fflush(stdout);// Isso força a mensagem a aparecer no terminal imediatamente.
                            remove_client(client_ip);
                            //exit(EXIT_FAILURE);
                        }
                        sprintf(envio, "Você já esta conectado com o nome: %s", cli->nome);
                        if(send(client_sock7, envio, strlen(envio) + 1, 0) == -1) {// Retorna MSG pra avisar q ta na lista e o nome 
                            printf("Falha na conexão com o cliente %s <%s>\n", client_ip, cli->nome);
                            fflush(stdout);// Isso força a mensagem a aparecer no terminal imediatamente.
                            remove_client(client_ip);
                            //exit(EXIT_FAILURE);
                        }
                    }
                }                
                else if(strcmp(cod, "/sai") == 0) {// SAI -----------------------------
                    // Criar função para deixar cliente suspenso;
                    //printf("---------------> Suspensao de cliente!\n");
                    send(client_sock7, "2", 1, 0);// Código de controle 2 para avisar q sabe q o cliente saiu 
                    printf("Cliente %s desconectado\n", client_ip);
                    fflush(stdout);// Isso força a mensagem a aparecer no terminal imediatamente.
                    remove_client(client_ip);
                }
                else if(strcmp(cod, "/out") == 0) {// OUT -----------------------------
                    send(client_sock7, "4", 1, 0);// Código de controle para avisar q sabe q o cliente deslogou 
                    printf("Cliente IP %s nome <%s> desconectado\n",client_ip, cli->nome);
                    fflush(stdout);// Isso força a mensagem a aparecer no terminal imediatamente.
                    remove_client(client_ip);
                }
                else {// Mensagem inválida -------------------------------------------
                    if(send(client_sock7, "0", 1, 0) == -1) {// Código de controle 0 para avisar que o cliente mandou algo incongruente
                        printf("Falha na conexão com o cliente %s <%s>\n", client_ip, cli->nome);
                        fflush(stdout);// Isso força a mensagem a aparecer no terminal imediatamente.
                        remove_client(client_ip);
                    }
                }
            }
        }
        if(FD_ISSET(client_sock8, &readfds)) {// Troca de mensagens na porta 50008 /msg/mpv/arq
            len = recv(client_sock8, recebeporta8, sizeof(recebeporta8), 0);// (cliente -> 8)
            // printf("Recebido na porta 8: %s\n", recebeporta8);
            // printf("Mensagem recebida de %s, com tamanho %d\n", client_ip, len);
            if(len <= 0) {
                perror("recv client_sock8");
                printf("Falha na conexão com o cliente %s <%s>\n", client_ip, cli->nome);
                fflush(stdout);// Isso força a mensagem a aparecer no terminal imediatamente.
                remove_client(client_ip);
            }
            else{
            if(cli->nome != NULL) {// So permite que o ciente envie mensagem se tiver nome
                printf("Recebido na porta 8: %s\n", recebeporta8);
                printf("Mensagem recebida de %s, com tamanho %d\n", client_ip, len);
                segmenta_recebeporta(recebeporta8, cod, nome, mensagem, &tam, ext, nome_arq);
                //printf("%s %ld\n", cod, strlen(mensagem));// Debug
                if(strcmp(cod, "/msg") == 0) {// MSG -----------------------------------
                    //printf("<%s>: %s\n", nome, mensagem);// Debug
                    sprintf(envio, "<%s>: %s", cli->nome, mensagem);
                    //printf("%s", envio);// Debug
                    send_to_all_clients(envio, strlen(envio));// Manda broadcas para clientes logados
                }
                if(strcmp(cod, "/mpv") == 0) {// MPV -----------------------------------
                    ClientNode* temp= find_client_by_nome(nome);// Retorna o sock da conexão desse cliente  find_client_by_nome(nome)
                    if(temp !=NULL) {// Função pra retorna o sock da conexão desse cliente  find_client_by_nome(nome)
                        sprintf(envio, "MPV <%s>: %s", cli->nome, mensagem);
                        if(send(temp->client_sock8, envio, strlen(envio) + 1, 0) == -1) {// Envia a mensagem privada (cliente deseJádo <- 8)
                            printf("Falha na conexão com o cliente %s <%s>\n", client_ip, cli->nome);
                            fflush(stdout);// Isso força a mensagem a aparecer no terminal imediatamente.
                            remove_client(temp->ip_address);
                        }
                        if(send(client_sock7, "2", 1, 0) == -1) {// Código 2 de controle avisar q enviou 
                            printf("Falha na conexão com o cliente %s <%s>\n", client_ip, cli->nome);
                            fflush(stdout);// Isso força a mensagem a aparecer no terminal imediatamente.
                            remove_client(client_ip);
                        }
                    }
                    else{
                        if(send(client_sock7, "3", 1, 0) == -1) {// Código 3 de controle avisar q nao enviou (cliente <- 8)
                            printf("Falha na conexão com o cliente %s <%s>\n", client_ip, cli->nome);
                            fflush(stdout);// Isso força a mensagem a aparecer no terminal imediatamente.
                            remove_client(client_ip);
                        }
                    }
                }
                if((strcmp(cod, "/arq")== 0)) {// ARQ -----------------------------------
                    if(send(client_sock7, "2", 1, 0) == -1) {// Código 2 de controle para o cliente comecar a enviar (cliente <- 8)
                        printf("Falha na conexão com o cliente %s <%s>\n", client_ip, cli->nome);
                        fflush(stdout);// Isso força a mensagem a aparecer no terminal imediatamente.
                        remove_client(client_ip);
                    }
                    else{
                        memset(arquivo, 0, sizeof(arquivo));
                        bytes = recv(client_sock8, arquivo, sizeof(arquivo), 0);// Recebe Arquivo (cliente -> 8)
                        arquivo[bytes]='\0';
                        printf("%s\n", arquivo);// Debug
                        //-------------- Envia Arquivo -------------------------------------
                        send_to_all_clients("!",  2);// Avisa que vai transmitir um arquivo (Todos os clientes <- 8)
                        dessegmenta(envio, nome_arq, ext, tam);// Formata para enviar em "envio"
                        printf("%s\n", envio);// Debug
                        send_to_all_clients(envio, (strlen(envio) + 1));// Informar tam, nome_arq e ext aos clientes (Todos os clientes <- 8) 
                        usleep(3);
                        send_to_all_clients(arquivo, (strlen(arquivo) + 1));// Enviar arquivo (Todos os clientes <- 8) 
                        //-------------- Final de envia Arquivo ---------------------------
                    }
                }
            }
        }}
        // if((cli->nome == NULL) && (current_time_in_seconds() >= end_time)){// Se o cliente nao se logar em X segundos é automaticamente desconectado
        //     printf("Time out do Cliente_ip: %s\nCliente %s desconectado", client_ip, client_ip);
        //     fflush(stdout);// Isso força a mensagem a aparecer no terminal imediatamente.
        //     remove_client(client_ip);
        // }
    }
}

void clear_client_list() {// Garante a lista n ter elementos
    ClientNode* current = head;
    ClientNode* next_node;
    while(current != NULL) {
        next_node = current->next;// Salva o próximo nó 
        close(current->client_sock8);
        close(current->client_sock7);
        pthread_cancel(current->thread);
        free(current);// Libera a memória do nó atual
        current = next_node;// Move para o próximo nó
    }
    head = NULL;// Define a cabeça como NULL, tornando a lista vazia
    print_conexoes();
}
ClientNode* is_client_in_list(const char *ip_address) {
    if(ip_address == NULL) return NULL;
    ClientNode* temp = head;// Inicia o ponteiro temporário na cabeça da lista
    while(temp != NULL) {// Loop até o final da lista
        if(strcmp(temp->ip_address, ip_address) == 0)// Compara o endereço IP
            return temp;// Retorna verdadeiro se o IP for encontrado
        temp = temp->next;// Move para o próximo nó
    }
    return NULL;  // Retorna falso se o IP não for encontrado
}
ClientNode* find_client_by_nome(const char *nome) {// Função pra retorna o sock da conexão desse cliente
    if(nome == NULL) return NULL;
    ClientNode* temp = head;
    while(temp != NULL) {
        if(strcmp(temp->nome, nome) == 0) return temp;// Compara nome dado com nome do ClientNode
        temp = temp->next;}
    return NULL;
}
bool name_not_in_list(const char *nome) {// Procura se um nome jJá esta em uso
    ClientNode* temp = head;// Inicia o ponteiro temporário na cabeça da lista
    while(temp != NULL) {// Ve se o nome Já ta na lista
        if((strcmp(temp->nome, nome))==0) return false;// Compara nome
        temp = temp->next;// Move para o próximo nó
    }
    return true;// Retorna verdadeiro se o nome não estiver em uso
}
int list_size() {
    int count = 0;
    ClientNode* temp = head;
    while(temp != NULL) {
        count++;
        temp = temp->next;
    }
    return count;
}
int login_list_size() {
    int count = 0;
    ClientNode* temp = head;
    while(temp != NULL) {
        if (temp->nome[0] != '\0') count++;
        temp = temp->next;
    }
    return count;
}

// double current_time_in_seconds() {// Função para depois exclui clientes sem nome, dado um período
//     struct timespec now;
//     clock_gettime(CLOCK_REALTIME, &now);
//     return now.tv_sec + now.tv_nsec / 1e9;
// }
// Segmenta de acordo com a func----------------------------------------------------------------------
bool segmenta_recebeporta(char *recebeporta, char *cod, char *nome, char *mensagem, int *tam, char *ext, char *nome_arq) {
    if(strncmp(recebeporta, "/lin", 4) == 0) {// LIN ------------------------------------------------
		if(sscanf(recebeporta, "%4s %9s", cod, nome)==2) return true;
    }  
    if(strncmp(recebeporta, "/out", 4) == 0) {// OUT ------------------------------------------------
        strcpy(cod, "/out"); return true;
    }  
    if(strncmp(recebeporta, "/sai", 4) == 0) {// SAI ------------------------------------------------
        strcpy(cod, "/sai"); return true;
    }
    if(strncmp(recebeporta, "/msg", 4) == 0) {// MSG ------------------------------------------------
		if(sscanf(recebeporta, "%4s %111s", cod, mensagem)==2) return true;
    }  
    if(strncmp(recebeporta, "/mpv", 4) == 0) {// MPV -----------------------------------------------
        if(sscanf(recebeporta, "%4s %9s %111s", cod, nome, mensagem)==3) return true;
    }  
    if(strncmp(recebeporta, "/arq", 4) == 0) {// ARQ -----------------------------------------------
        if(sscanf(recebeporta, "%4s %19s %9s %d", cod, nome_arq, ext, tam) == 4) return true;
    }
    return false;}
// Dessegmenta de acordo com a func(Para o envio de arq)----------------------------------------------
bool dessegmenta(char *envio, const char *nome_arq, const char *ext, int tam) {
    if(sprintf(envio, "%s %s %d", nome_arq, ext, tam) > 0) return true;
    return false;
}
