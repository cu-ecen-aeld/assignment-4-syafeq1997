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

#define FILE_PATH "/var/tmp/aesdsocketdata"
#define PORT 9000
#define BUFFER_SIZE 1024

static int server_fd, new_socket;
static FILE *file;


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

char *readBuff(int server_fd){
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


void openSocket()
{
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];

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

    while(1){
        printf("waiting new connection\n");
        
	if ((new_socket = accept(server_fd, (struct sockaddr*)&address,&addrlen))< 0)
	{
            perror("accept failed");
            exit(-1);
        }

	char ip4[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(address.sin_addr), ip4, INET_ADDRSTRLEN);
        syslog(LOG_INFO, "Accepted connection from %s", ip4);

	char *result = readBuff(new_socket);
	fprintf(file, "%s", result);
	
	memset(buffer, 0, sizeof(buffer));
	rewind(file);
	while(fgets(buffer, sizeof(buffer), file) != NULL){
	    if (send(new_socket, buffer, strlen(buffer), 0) == -1)
	    {
                perror("Send failed");
                break;
            }

            // Clear the buffer before next use
            memset(buffer, 0, sizeof(buffer));
	}
	syslog(LOG_INFO, "Closed connection from %s", ip4);
    }
    
}

void handle_sig()
{
    syslog(LOG_INFO, "Caught signal, exiting");
    
    //delete /var/tmp/aesdsocketdata
    if (remove(FILE_PATH) == 0) {
        printf("File deleted successfully.\n");
    } else {
        printf("Error: Unable to delete the file.\n");
    }

    //close socket
    close(new_socket);
    close(server_fd);

    //close file
    fclose(file);

    //close syslog
    closelog();

    exit(0);
}

int main(int argc, char* argv[])
{

    //register signal handler
    signal(SIGINT, handle_sig);
    signal(SIGTERM, handle_sig);

    //deamonize if argument passed 
    if (argc == 2 && strcmp(argv[1], "-d") == 0){
        become_daemon();
    }

    //open syslog
    openlog("App_aesdsocket", LOG_PID | LOG_CONS, LOG_USER);

    //Open file
    file = fopen(FILE_PATH, "w+");
    if (file == NULL) {
        syslog(LOG_ERR, "Failed to open file: %s", FILE_PATH);
        exit(-1);
    }

    //Open socket and listen
    openSocket();
    
    //close socket
    close(new_socket);
    close(server_fd);

    //close file
    fclose(file);

    //close syslog
    closelog();
    
    return 0;
}
