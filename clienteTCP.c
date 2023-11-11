#include <arpa/inet.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <stdio.h>
//#define SERV_IP  "10.0.0.2.15"// Qual o IP?
//#define SERV_IP  "198.168.56.104"// Qual o IP?
#define SERV_IP  "192.168.56.104"// Qual o IP?
//#define SERV_IP  "172.17.0.1"// Qual o IP?
//#define SERV_IP  "10.0.2.15"// Qual o IP?
#define CAMINHO_IN "/home/brunobavarescozaffari/Desktop/"// Onde você quer salvar os arquivos recebidos?
//#define SERV_IP "172.20.10.12"// Qual o IP?
// ----------------------------------- Arquivo Recebe ------------------------------------
bool segmenta_arq_recebimento(const char *recebe, char *nome_arq, char *ext, char *tam) {
    if(sscanf(recebe, "%s %s %s", nome_arq, ext, tam) == 3) return true;
    return false;
}
void salva_arquivo(const char *conteudo, const char *nome_arq, const char *caminho, const char *ext) {// Define a função que salvará o arquivo
    char caminho_completo[128] = {0};// Cria o caminho completo para o arquivo
    size_t len;
    sprintf(caminho_completo, "%s/%s.%s", caminho, nome_arq, ext);
    FILE *fp=fopen(caminho_completo, "wb");// Abre o arquivo para gravação
    if(fp == NULL) {
        perror("Erro ao abrir o arquivo para gravação");
        printf("Falha ao salvar o arquivo.\n");
        return ;
    }
    len = strlen(conteudo);// Assume que o conteúdo é uma string terminada em nulo
    if(fwrite(conteudo, 1, len, fp) != len) {// Grava o conteúdo no arquivo
        perror("Erro ao escrever no arquivo");
        printf("Falha ao salvar o arquivo.\n");
        fclose(fp);
        return;
    }
    fclose(fp);// Fecha o arquivo
    printf("Arquivo salvo com sucesso!\n");
    return;
}
// ----------------------------------- Arquivo Envio -----------------------------------
bool segmnta_arq(const char *txt, char *cod, char *caminho) {
    if(strncmp(txt, "/arq", 4) == 0) {
        strcpy(cod, "/arq");// Copia "/arq" para 'cod'
        const char *resto = txt + 4;
        if(!isspace(*resto)) return false;// Se o próximo caractere não for um espaço, retorne falso
        while(isspace(*resto)) resto++;// Ignorar espaços  
        if(*resto == '\0') return false;// Se o resto da string for vazio, retorne falso
        strcpy(caminho, resto);// Copia o resto da string, ignorando espaços iniciais
        return true;
    }
    return false;
}
bool le_arq(const char *caminho, char *tam, char *ext, char *arquivo, char *nome_arq) {
    char x[128];
    strcpy(x, caminho);
    FILE *fp = fopen(x, "r");
    if(fp == NULL) {
        perror("Erro ao abrir arquivo");
        return false;
    }
    fseek(fp, 0, SEEK_END);
    long tamanho = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (tamanho >= 1501) {
        printf("O arquivo é muito grande para ser transmitido.\n");
        fclose(fp);
        return false;
    }
    snprintf(tam, 5, "%ld", tamanho);
    char *nome = strrchr(caminho, '/');
    if(nome != NULL) nome++;
    else nome=(char *) caminho;
    char *extensao=strrchr(nome, '.');
    if(extensao != NULL) {
        strcpy(ext, extensao + 1);
        strncpy(nome_arq, nome, extensao - nome);// Copia o nome do arquivo sem a extensão
        nome_arq[extensao - nome] = '\0';// Adicionar o null-terminator manualmente
    }
    else {
        strcpy(ext, "");
        strcpy(nome_arq, nome);
    }
    fread(arquivo, 1, tamanho, fp);
    arquivo[tamanho] = '\0';
    fclose(fp);
    return true;
}
// ---------------------------------- Mensagens gerais----------------------------------------
bool segmenta_geral(char *envio, char *txt, char *cod, char *nome, char *mensagem) {
    if(strncmp(txt, "/lin ", 5) == 0) strcpy(cod, "/lin");
    else if((strncmp(txt, "/sai ", 5) == 0) || (strncmp(txt, "/sai", 5) == 0)) {
        strcpy(cod, "/sai");
        sprintf(envio, "%s", cod);
        return true;
    }
    else if((strncmp(txt, "/out ", 5) == 0) || (strncmp(txt, "/out", 5) == 0)) {
        strcpy(cod, "/out");
        sprintf(envio, "%s", cod);
        return true;
    }
    else if(strncmp(txt, "/msg ", 5) == 0) strcpy(cod, "/msg");
    else if(strncmp(txt, "/mpv ", 5) == 0) strcpy(cod, "/mpv");
    else if((strncmp(txt, "/hlp ", 5) == 0) || (strncmp(txt, "/hlp", 5) == 0)) {
        strcpy(cod, "/hlp");
        sprintf(envio, "%s", cod);
        return true;
    }
    else return false;// Mensagem invalida
    cod[4] = '\0';
    char *resto = txt + 5;
    if(strcmp(cod, "/lin") == 0) {
        sscanf(resto, "%9s", nome);
        if(strlen(nome) > 9) {
            perror("Nome grande demais\n");
            return false;
        }
        sprintf(envio, "%s %s", cod, nome);
        return true;
    }
    else if(strcmp(cod, "/msg") == 0) {
        strncpy(mensagem, resto, 111);
        mensagem[127] = '\0';
        sprintf(envio, "%s %s", cod, mensagem);
        return true;
    }
    else if(strcmp(cod, "/mpv") == 0) {
        sscanf(resto, "%9s", nome);
        if(strlen(nome) > 9) {
            perror("Nome grande demais\n");
            return false;
        }
        resto = strchr(resto, ' ');
        if(resto != NULL) {
            strncpy(mensagem, resto + 1, 127);
            mensagem[127] = '\0';
        }
        sprintf(envio, "%s %s %s", cod, nome, mensagem);
        return true;
    }
    return false;
}
int main(int argc, char *argv[]) {
    //------------------------------------------------- Váriaveis Uso geral-------------------------
    struct sockaddr_in server_address7, server_address8;
    int controle=0, controle2=0, controle3=0, conectado=0;
    int status_receive, status_sender;
    char envioporta8[1501];
    char envioporta7[128];
    char mensagem[128];
    char recebe[1501];// Recebimento de mensagens
    char recebe_char;// Recebimento de códigos
    pid_t receiver;
    char txt[143];
    char nome[10]; 
    socklen_t len;
    ssize_t x, y;
    pid_t sender;
    char cod[5];
    int logado = 0;
    //-------------------------------------------------Váriaveis para Arquivo----------------------
    char arquivo_in[1501];
    char arquivo[1501];
    char caminho[128];
    char nome_arq[20];
    char ext[10];
    char tam[5];    
    //----------------------------------------------------- Criação de sockets -----------------
     if(controle==0) {
            if((sender=fork())!=0) kill(sender, SIGTERM);// Se for igual esta em Sender
            if((receiver=fork())!=0) kill(receiver, SIGTERM);// Se for igual esta em Reciver
            controle = 1;
            //printf("opaaa %d %d\n", (int)sender, (int)receiver);// debug
        }
    int sock7 = socket(AF_INET, SOCK_STREAM, 0);// Criar socket 7 ------------------------------
    if(sock7 < 0) {
        perror("Erro ao criar socket");
        exit(1);
    }
    
    server_address7.sin_family = AF_INET;
    server_address7.sin_port = htons(50007);// Configurar endereço do servidor para porta 50007
    
    if(inet_pton(AF_INET, SERV_IP, &server_address7.sin_addr)<=0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    
    int sock8 = socket(AF_INET, SOCK_STREAM, 0);// Criar socket 8 ------------------------------
    if(sock8 < 0) {
        perror("Erro ao criar socket");
        exit(1);  
    }
    
    server_address8.sin_family = AF_INET;
    server_address8.sin_port = htons(50008);// Configurar endereço do servidor para porta 50008
    
    if(inet_pton(AF_INET, SERV_IP, &server_address8.sin_addr)<=0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    // --------------------------------------------------- Fim da Criação de sockets -----------
    //-------------------- Conectar ---------------------
    y = connect(sock7, (struct sockaddr *) &server_address7, sizeof(server_address7));
    //printf("%d\n",(int)y);// Debug
    //printf("%d\n",(int)sock7);// Debug
    if(y < 0) {
        perror("Erro ao conectar com socket 7");
        exit(1);  
    }
    usleep(9);
    y = connect(sock8, (struct sockaddr *) &server_address8, sizeof(server_address8));
    //printf("%d\n",(int)y);// Debug
    //printf("%d\n",(int)sock8);// Debug
    if(y < 0) {
        perror("Erro ao conectar com socket 8");
        exit(1);  
    }
   
    x = recv(sock8, &recebe_char, 1, 0);// Se já esta logado recebe confirmacao 
    if(x > 0 && recebe_char== '1') conectado = 1;
    printf("%c\n", recebe_char);
    //}// ------------------------------------------------- Fim Conectar ------------------------
    // ----------------- Vê se já esta logado -------------------------------------------------
    recebe_char = -1;// "Zera" variável recebe_char
    memset(recebe, 0, sizeof(recebe));// Zera variável recebe

    if(conectado ==1){
        if(recv(sock7, &recebe_char, 1, 0) > 0) {
            //printf("%c\n", recebe_char);// Debug
            if(recebe_char== '1') {// Vê se já esta Logado
                x = recv(sock7, &recebe, sizeof(recebe) - 1, 0);// Aguarda receber o nome cliente <- 7
                recebe[x] = '\0';
                printf("Voce já esta logado e seu nick é: %s\n", nome);
                logado = 1;// Para criar os processos filhos: sender e recive                             
            }
            else if(recebe_char== '0') logado = 0;
            else if(recebe_char !='0'|| recebe_char !='1') {
                logado = 0;
                printf("Código recebido do servidor incoerente\n");
            }
        }
        else {// Servidor perdeu a conexão
            printf("Erro inesperado. Desconexão com o servidor\n");
            close(sock7);
            close(sock8);
            //return main();
            //execv(argv[0], argv);// Substitui o processo atual por uma nova instância dele mesmo
            exit(0);
        }
    
    while(1) {// ------------------------------------ while ----------------------------------
        if((receiver != 0) && (sender != 0) && (logado==0)) {// Executa Pai ---------------------------------
            while(logado == 0) {
                if(controle3 ==0) {
                    printf("Bem-vindo ao chat! Aqui estão as opções que você pode usar:\n");
                    printf("--------------------------------------------------------\n");
                    printf("/lin <nikname[9]> : Loga e salva seu nome. Se já estiver logado, informará que você já está logado e não poderá trocar o nome.\n");
                    printf("/sai : Sai e encerra o programa.\n");
                    printf("--------------------------------------------------------\n");
                    controle3 = 1;}
                // Zera variaveis para garantir-----------------------------------------------
                memset(envioporta7, 0, sizeof(envioporta7));
                memset(recebe, 0, sizeof(recebe));
                memset(nome, 0, sizeof(nome));
                memset(txt, 0, sizeof(txt));
                memset(ext, 0, sizeof(ext));
                memset(tam, 0, sizeof(tam));
                memset(cod, 0, sizeof(cod));
                recebe_char = 0;
                x = -2;
                // - Lê entrada ---------------//|
                /*|*/fgets(txt, 143, stdin);   //|
                /*|*/txt[strlen(txt) - 1]='\0';//|
                //|//--------------------------//|
                printf("\n"); 
                if(segmenta_geral(envioporta7, txt, cod, nome, mensagem)) {// /lin /out /sai---
                    if(strcmp(cod, "/lin") == 0) {// LIN --------------------------------------
                        y = send(sock7, envioporta7, sizeof(envioporta7), 0);
                        //printf("%d\n", (int)y);     
                        if( y > 0) {                      
                            if(recv(sock7, &recebe_char, 1, 0) > 0) {
                                if(recebe_char== '2') {// Talvez tirar daqui?
                                    printf("Agora, voce esta logado e seu nick es: %s\n", nome);
                                    logado = 1;// Para criar os processos filhos: sender e recive                             
                                    x = recv(sock7, recebe, sizeof(recebe)-1, 0);
                                    recebe[x]='\0';
                                    printf("%s", recebe);
                                }
                                else if(recebe_char == '3') {
                                    printf("Servidor cheio, tente novamente mais tarde!\n");
                                    logado = 0;// Para sair dos processos filhos: sender e recive
                                }
                                else if(recebe_char == '9') {
                                    printf("Nome ja em uso, tente outro nome\n");
                                    logado = 0;// Para sair dos processos filhos: sender e recive
                                }
                                else if(recebe_char !='9'|| recebe_char !='3'|| recebe_char !='2')  
                                    printf("Código recebido do servidor incoerente, digite novamente\n");
                            }
                            else {// Servidor perdeu a conexão
                            printf("Erro, inesperado. Desconexão com o servidor\n");
                            close(sock7);// Fecha socket sock7
                            close(sock8);// Fecha socket sock8
                            //return main();
                            execv(argv[0], argv);// Substitui o processo atual por uma nova instância dele mesmo
                        }
                        }
                        else {// Servidor perdeu a conexão
                            printf("Erro, inesperado. Desconexão com o servidor\n");
                            close(sock7);// Fecha socket sock7
                            close(sock8);// Fecha socket sock8
                            //return main();
                            execv(argv[0], argv);// Substitui o processo atual por uma nova instância dele mesmo
                        }
                    }
                    else if(strcmp(cod, "/sai") == 0) {// SAI ---------------------------------
                            if (send(sock7, envioporta7, sizeof(envioporta7), 0) > 0) {
                                x = -1;
                                x = recv(sock7, &recebe_char, 1, 0);
                                logado = 0;
                                if (x > 0 && recebe_char == '2') printf("Você esta saindo do programa\n"); // Controle
                                else printf("erro\n");
                                close(sock7);// Fecha socket sock7
                                close(sock8);// Fecha socket sock8
                                exit(0);// Encerra programa
                            }
                            else printf("Erro, ao enviar\n");
                            close(sock7);// Fecha socket sock7
                            close(sock8);// Fecha socket sock8
                            exit(0);// Encerra programa
                        }
                }
            }
            if(logado == 1 && controle2 ==0) {  
                printf("------------------------------------------------------------------------------------------------------------------------------------\n");
                printf("/lin <nikname[9]> : Loga e salva seu nome. Se já estiver logado, informará que você já está logado e não poderá trocar o nome.\n");
                printf("/out : Sai do chat.\n");
                printf("/msg <mensagem[128]> : Envia uma mensagem com tamanho de até 127 caracteres.\n");
                printf("/mpv <nikname[9]> <mensagem[127]> : Envia uma mensagem privada com tamanho de até 127 caracteres para o usuário especificado. Se o usuário não existir, você será informado.\n");
                printf("/arq <caminho do arquivo> Envia um arquivo de até 1500 bytes.\n");
                printf("/hlp : Ajuda/Printa opcoes\n");
                printf("-----------------------------------------------------------------------------------------------------------------------------------\n");
                controle2=1;
            }}// ---------------------------------- Termino do código Pai --------------
        else if(receiver == 0) {// -------------- Código do receiver(filho) ------------
            while(1) {
                //printf("receiver\n");// Debug
                x = -2;
                memset(recebe, 0, sizeof(recebe));
                x = recv(sock8, recebe, sizeof(recebe) - 1, 0);
                if(x>0) {// Aguarda receber o nome (cliente <-8)
                    if(recebe[0]=='!') {// ----------- Recebimento de Arquivo------------
                        memset(ext, 0, sizeof(ext));// Garante que strings estao vazias
                        memset(tam, 0, sizeof(tam));// Garante que strings estao vazias
                        memset(recebe, 0, sizeof(recebe));// Garante que strings estao vazias
                        memset(nome_arq, 0, sizeof(nome_arq));// Garante que strings estao vazias
                        x=-2;
                        x=recv(sock8, recebe, sizeof(recebe) - 1, 0);
                        if(x ==-1 || x==0) {perror("Erro ao receber dados"); exit(0);}
                        else {
                            recebe[x] = '\0';
                            printf("Recebido do servidor: %s\n", recebe);// Debug
                            if((segmenta_arq_recebimento(recebe, nome_arq, ext, tam))) {// Reescreve variaveis
                                x = -2;
                                //printf("o\n");
                                memset(arquivo_in, 0, sizeof(arquivo_in));// Garante que strings estao vazias
                                x = recv(sock8, arquivo_in, sizeof(arquivo_in) - 1, 0);
                                if(x >0) {
                                    arquivo_in[x] = '\0';
                                    strcat(nome_arq, "(2)");
                                    //printf("%s\n", arquivo_in);// Debug
                                    salva_arquivo(arquivo_in, nome_arq, CAMINHO_IN, ext);}
                                else{
                                    perror("Erro ao receber dados"); exit(0);
                                }
                            }
                            else printf("Erro\n");
                        }}
                        // ------------------------- Fim Recebimento de Arquivo---------
                    else printf("%s\n", recebe);// Mensagem comum ------------------------
                }
                else {// Servidor perdeu a conexão
                    printf("Erro inesperado. Desconexão com o servidor\n");
                    exit(1);// Mata receiver
                }
            
        }}// ------------------------------------- Fim do Reciver (filho) -----------------
        else if(sender == 0) {// ---------------- Código do sender -----------------------
            while(1) {
                //printf("Sender\n");// Debug
                // Zera variaveis para garantir ----------------------------------------------
                memset(envioporta8, 0, sizeof(envioporta8));
                memset(arquivo_in, 0, sizeof(arquivo_in));
                memset(mensagem, 0, sizeof(mensagem));
                memset(nome_arq, 0, sizeof(nome_arq));
                memset(caminho, 0, sizeof(caminho));
                memset(arquivo, 0, sizeof(arquivo));
                memset(recebe, 0, sizeof(recebe));
                memset(nome, 0, sizeof(nome));
                memset(txt, 0, sizeof(txt));
                memset(ext, 0, sizeof(ext));
                memset(tam, 0, sizeof(tam));
                memset(cod, 0, sizeof(cod));
                recebe_char = 0;
                x = -2;
                y = -2;
                // - Lê entrada ---------------//|
                /*|*/fgets(txt, 143, stdin);   //|
                /*|*/txt[strlen(txt) - 1]='\0';//|
                //|//--------------------------//|
                printf("\n"); 
                if(segmenta_geral(envioporta8, txt, cod, nome, mensagem)) {// /lin /out /sai /msg /mpv MENSAGENS GERAIS-
                    if(strcmp(cod, "/lin") == 0) {// LIN ------------------------
                        if(send(sock7, envioporta8, strlen(envioporta8) + 1, 0) > 0) {
                            if(recv(sock7, &recebe_char, 1, 0) > 0) {
                                if (recebe_char == '4') {// Se já esta logado vai receber o nome q esta salvo no servidor
                                    x = recv(sock7, recebe, sizeof(recebe) - 1, 0);// Aguarda receber o nome cliente <-7
                                    recebe[x] = '\0';
                                    printf("%s\n", recebe);
                                }
                                else printf("Código recebido do servidor incoerente, digite novamente\n");
                            }
                        }
                        else {// Servidor perdeu a conexão
                            printf("Erro inesperado. Desconexão com o servidor\n");
                            exit(0);// Mata sender
                        }
                    }
                    else if(strcmp(cod, "/out") == 0) {// OUT ------------------------
                        if(send(sock7, envioporta8, strlen(envioporta8) + 1, 0) > 0) {
                            x = recv(sock7, &recebe_char, 1, 0);
                            if(x > 0 && recebe_char == '4') {
                                printf("Você foi deslogado do chat.\n");// Controle
                                exit(0);// Mata sender
                            }
                            else if(x > 0 && (recebe_char != '1' || recebe_char != '4')) 
                                    printf("Código recebido do servidor incoerente, digite novamente\n");
                        }
                        else {// Servidor perdeu a conexão
                            printf("Erro inesperado. Desconexão com o servidor\n");
                            exit(0);// Mata sender
                        }
                    }
                    else if(strcmp(cod, "/msg") == 0) {// MSG ------------------------
                        if(send(sock8, envioporta8, strlen(envioporta8) + 1, 0)==-1) {
                            printf("Erro inesperado. Desconexão com o servidor\n");
                            exit(0);// Mata sender
                        }
                    }
                    else if(strcmp(cod, "/mpv") == 0) {// MPV -------------------------------------------------
                        if(send(sock8, envioporta8, strlen(envioporta8) + 1, 0) > 0) {
                            if (recv(sock7, &recebe_char, 1, 0) > 0) {// Controle cliente <-7
                                if (recebe_char == '2') printf("Enviado.\n");
                                else if (recebe_char == '3') printf("%s Nao foi encontrado no chat\n", nome);
                                else if (recebe_char != '2' || recebe_char != '3')
                                    printf("Código recebido do servidor incoerente, digite novamente\n");}
                        }
                        else {// Servidor perdeu a conexão
                            printf("Erro inesperado. Desconexão com o servidor\n");
                            exit(0);// Mata sender
                        }
                    }
                    else if(strcmp(cod, "/hlp") == 0){// HLP=Ajuda ------------------------------------------
                        printf("--------------------------------------------------------\n");
                        printf("/lin <nikname[9]> : Loga e salva seu nome. Se já estiver logado, informará que você já está logado e não poderá trocar o nome.\n");
                        printf("/out : Sai do chat.\n");
                        printf("/msg <mensagem[128]> : Envia uma mensagem com tamanho de até 127 caracteres.\n");
                        printf("/mpv <nikname[9]> <mensagem[127]> : Envia uma mensagem privada com tamanho de até 127 caracteres para o usuário especificado. Se o usuário não existir, você será informado.\n");
                        printf("/arq <caminho do arquivo> Envia um arquivo de até 1500 bytes.\n");
                        printf("/hlp : Ajuda/Printa opcoes\n");
                        printf("--------------------------------------------------------\n");
                    } 
                }
                else if(segmnta_arq(txt, cod, caminho)) {// ARQ ---------------------------------------------
                    if(le_arq(caminho, tam, ext, arquivo, nome_arq)) {
                        // printf("%s %s %s %s", caminho, tam, ext, nome_arq);// Debug
                        sprintf(envioporta8, "%s %s %s %s", cod, nome_arq, ext, tam);
                        if(send(sock8, envioporta8, strlen(envioporta8) + 1, 0) > 0) {
                            if(recv(sock7, &recebe_char, 1, 0) > 0){
                                if(recebe_char == '2') send(sock8, arquivo, sizeof(arquivo), 0); // Permissão para mandar arquivo
                                else printf("Código recebido do servidor incoerente, digite novamente\n");
                            }
                        }
                        else {// Servidor perdeu a conexão
                            printf("Erro inesperado. Desconexão com o servidor\n");
                            exit(0);// Mata sender
                        }
                    }
                }    
                else printf("Falha ao processar o texto.\n");// Texto nao contem código valido
            } 
        }// ------------------------------------- Fim Sender -----------------------------------------------
        // ----------------------------------- Parte do Pai ------------------------------------------------  
        if(logado==1 && sender!= 0 && receiver!=0) {// Pai cria Sender e Reciver
            sender = fork(); 
            //printf("sender %d\n", (int)sender);// Debug
            if(sender!=0) {
                receiver = fork(); 
                //printf("receiver %d\n", (int)receiver);// Debug
                }// Pai cria Receiver
            }
        //if(logado==1 && sender>0 && receiver>0) printf("opaaa 2 %d %d\n", (int)sender, (int)receiver); // Debug
        if(logado==1 && sender>0 && receiver>0) {
            if(waitpid(sender, &status_sender, WNOHANG) >0) {// Verifica se o processo sender terminou
                printf("Sender terminou, matando processo Receiver\n");
                kill(receiver, SIGKILL);
                logado=0;
                controle=0;
                controle2=0;
                conectado=0;
                controle3 = 0;
                close(sock7);
                close(sock8);
                printf("Desconexao com o servidor\n");
                execv(argv[0], argv);
                //return 0;
                //exit(0);
            }
            else if(waitpid(receiver, &status_receive, WNOHANG) >0) {// Verifica se o processo receiver terminou
                printf("Receiver terminou, matando processo Sender\n");
                kill(sender, SIGKILL);
                logado=0;
                controle=0;
                controle2=0;
                conectado=0;
                controle3 = 0;
                close(sock7);
                close(sock8);
                printf("Desconexao com o servidor\n");
                execv(argv[0], argv);
                //return 0;
                //exit(0);
            }
            waitpid(sender, &status_sender, 0);
            waitpid(receiver, &status_receive, 0);
            if(WIFEXITED(status_receive) && WIFEXITED(status_sender)) {
                logado=0;
                controle=0;
                controle2=0;
                conectado=0;
                controle3 = 0;
                close(sock7);
                close(sock8);
                printf("Desconexao com o servidor\n");
                execv(argv[0], argv);// Substitui o processo atual por uma nova instância dele mesmo
                //return 0;
                //exit(0);
            }
        }// ----------------------------- Fim da parte do Pai ---------------------------------------------
    }// ---------------------------------- Final do while ------------------------------------------------
}
    return 0;
}