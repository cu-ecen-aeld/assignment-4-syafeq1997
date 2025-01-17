#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>

#define FILE_PATH "/var/tmp/aesdsocketdata"
#define PORT 9000
#define BUFFER_SIZE 1024

static int server_fd, new_socket;
static FILE *file;
pthread_mutex_t file_mutex;
pthread_mutex_t send_mutex;
volatile sig_atomic_t exit_flag = 0;

typedef struct ThreadNode {
    pthread_t thread_id;
    struct ThreadNode *next;
} ThreadNode;

ThreadNode *thread_list = NULL;

pthread_mutex_t thread_list_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_thread(pthread_t thread_id) 
{
    pthread_mutex_lock(&thread_list_mutex);
    
    ThreadNode *new_node = malloc(sizeof(ThreadNode));
    
    if (!new_node) 
    {
        perror("Failed to allocate memory for thread node");
        pthread_mutex_unlock(&thread_list_mutex);
        return;
    }
    
    new_node->thread_id = thread_id;
    new_node->next = thread_list;
    
    thread_list = new_node;
    pthread_mutex_unlock(&thread_list_mutex);
}

void join_and_remove_threads() 
{
    pthread_mutex_lock(&thread_list_mutex);
    ThreadNode *current = thread_list;
    
    while (current) {
        pthread_join(current->thread_id, NULL);
        ThreadNode *temp = current;
        current = current->next;
        free(temp);
    
    }
    thread_list = NULL;
    pthread_mutex_unlock(&thread_list_mutex);
}


void become_daemon()
{
    //1st fork
    switch(fork())
    {
        case -1: perror("Fork failed"); exit(EXIT_FAILURE);
	case 0: break;
	default: exit(EXIT_SUCCESS); //terminate parent
    }

    //become season leader
    if(setsid() == -1)
    {
        exit(-1);
    }

    //2nd fork
    switch(fork())
    {
        case -1: perror("Fork failed"); exit(EXIT_FAILURE);
        case 0: break;
        default: exit(EXIT_SUCCESS);
    }

    umask(0);
    //chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_RDWR);
}

char *readBuff(int server_fd)
{
    char buffer[BUFFER_SIZE] = {};
    char *result = NULL;
    size_t totalSize = 0;

    while(1){
        memset(buffer, 0, sizeof(buffer));
        size_t bytesNum = recv(server_fd, buffer, BUFFER_SIZE, 0);
        if (bytesNum < 0){
            perror("Read error");
            free(result);
            return NULL;
        }else if(bytesNum == 0){
            // Client was close
            break;
        }

        // Expand buffer(result)
        char *temp = realloc(result, totalSize + bytesNum + 1); // +1 for '\0'
        if (temp == NULL) {
            perror("Memory allocation error");
            free(result);
            return NULL;
        }
        result = temp;

        // Copy to buffer(result)
        memcpy(result + totalSize, buffer, bytesNum);
        totalSize += bytesNum;
        if(buffer[bytesNum-1] == '\n'){
            result[totalSize] = '\0';
            break;
        }
    }
    return result;
}

void *handle_client(void *args)
{
    int client_socket = *(int *)args;
    free(args);
    char *result = readBuff(client_socket);
    if (!result) {
        syslog(LOG_ERR, "Failed to read buffer");
        close(client_socket);
        return NULL;
    }  	
    
    pthread_mutex_lock(&file_mutex);

    file = fopen(FILE_PATH, "a");
    if (file == NULL) {
        syslog(LOG_ERR, "Failed to open file: %s", FILE_PATH);
	free(result);
	pthread_mutex_unlock(&file_mutex);
	close(client_socket);
        return NULL;
    }
	
    fprintf(file, "%s", result);
    fclose(file);
    pthread_mutex_unlock(&file_mutex);
	
    pthread_mutex_lock(&send_mutex);
    file = fopen(FILE_PATH, "r");
    if (file == NULL) {
        syslog(LOG_ERR, "Failed to open file: %s", FILE_PATH);
	pthread_mutex_unlock(&send_mutex);
	close(client_socket);
        return NULL;
    }

    char buffer[BUFFER_SIZE] = {};
    while(fgets(buffer, sizeof(buffer), file) != NULL){
        if (send(client_socket, buffer, strlen(buffer), 0) == -1){
            perror("Send failed");
            break;
        }
    }
    fclose(file);
    pthread_mutex_unlock(&send_mutex);

    close(client_socket);
    return NULL;
}


void openSocket()
{
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed");
        exit(-1);
    }

    if (setsockopt(server_fd, SOL_SOCKET,SO_REUSEADDR | SO_REUSEPORT, &opt,sizeof(opt)))
    {
        perror("setsockopt failed");
     	exit(-1);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address,sizeof(address))< 0)
    {
        perror("bind failed");
        exit(-1);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("listen failed");
        exit(-1);
    }

    char ip4[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(address.sin_addr), ip4, INET_ADDRSTRLEN);
    syslog(LOG_INFO, "Accepted connection from: %s", ip4);

    while(!exit_flag){
        printf("waiting new connection\n");
        
	if ((new_socket = accept(server_fd, (struct sockaddr*)&address,&addrlen))< 0)
	{
            perror("accept failed");
	    continue;
        }

	char ip4[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(address.sin_addr), ip4, INET_ADDRSTRLEN);
        syslog(LOG_INFO, "Accepted connection from %s", ip4);
	
	int *client_socket_ptr = malloc(sizeof(int));
	if(!client_socket_ptr){
	    perror("memory allocation failed");
	    close(new_socket);
	    continue;
	}

	*client_socket_ptr = new_socket;
	pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, client_socket_ptr) != 0) {
            perror("Thread creation failed");
            close(new_socket);
            free(client_socket_ptr);
        }else{
	    add_thread(thread_id);
	}
    }
    
}

void handle_sig()
{
    syslog(LOG_INFO, "Caught signal, exiting");
    exit_flag = 1;
    
    //delete /var/tmp/aesdsocketdata
    if (remove(FILE_PATH) == 0) {
        printf("File deleted successfully.\n");
    } else {
        printf("Error: Unable to delete the file.\n");
    }

    close(server_fd);

    pthread_mutex_destroy(&file_mutex);
    pthread_mutex_destroy(&send_mutex);

    closelog();

    exit(0);
}

void *append_timestamp(void *arg) 
{
    while (!exit_flag) {
        char timestamp[BUFFER_SIZE];
        time_t now = time(NULL);
        struct tm *timeinfo = localtime(&now);

        if (strftime(timestamp, sizeof(timestamp), "timestamp:%a, %d %b %Y %H:%M:%S %z\n", timeinfo) == 0) {
            syslog(LOG_ERR, "Failed to format timestamp");
            continue;
        }

        pthread_mutex_lock(&file_mutex);
        FILE *file = fopen(FILE_PATH, "a");
        if (file == NULL) {
            syslog(LOG_ERR, "Failed to open file: %s", FILE_PATH);
            pthread_mutex_unlock(&file_mutex);
            continue;
        }

        fprintf(file, "%s", timestamp);
        fclose(file);
        pthread_mutex_unlock(&file_mutex);

        sleep(10); // Sleep for 10 seconds
    }

    return NULL;
}

int main(int argc, char* argv[])
{
    if (access(FILE_PATH, F_OK) == 0) {
        // File exists, attempt to delete it
        if (remove(FILE_PATH) == 0) {
            printf("File %s deleted successfully.\n", FILE_PATH);
        }
    }

    //register signal handler
    signal(SIGINT, handle_sig);
    signal(SIGTERM, handle_sig);

    //deamonize if argument passed 
    if (argc == 2 && strcmp(argv[1], "-d") == 0){
        become_daemon();
    }

    //open syslog
    openlog("App_aesdsocket", LOG_PID | LOG_CONS, LOG_USER);

    pthread_mutex_init(&file_mutex, NULL);
    pthread_mutex_init(&send_mutex, NULL);

    pthread_t timestamp_thread;
    if (pthread_create(&timestamp_thread, NULL, append_timestamp, NULL) != 0) {
        perror("Failed to create timestamp thread");
        exit(EXIT_FAILURE);
    }

    //Open socket and listen
    openSocket();

    join_and_remove_threads();

    pthread_join(timestamp_thread, NULL);    
    
    close(server_fd);
    
    pthread_mutex_destroy(&file_mutex);
    pthread_mutex_destroy(&send_mutex);
    closelog();
    
    return 0;
}
