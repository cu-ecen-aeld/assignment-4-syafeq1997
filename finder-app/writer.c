#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>


int main(int argc, char *argv[]){
    openlog("writer program", LOG_PID | LOG_CONS, LOG_USER);
    if (argc != 3){
        syslog(LOG_ERR, "no arguments supplied");
	closelog();
	return 1;
    }

    const char *writefile = argv[1];
    const char *writestr = argv[2];

    //write filie
    FILE* fileptr;
    fileptr = fopen(writefile, "w");
    if (fileptr == NULL ){
        syslog(LOG_ERR, "failed to open file: %s", writefile);
	closelog();
        return 1;
    } else {
        syslog(LOG_DEBUG, "file open: %s", writefile);
    }

    if (fputs(writestr, fileptr) == EOF) {
        syslog(LOG_ERR, "failed to write to file: %s", writefile);
        fclose(fileptr);
        closelog();
        return 1;
    } else {
        syslog(LOG_DEBUG, "Writing '%s' to %s", writestr, writefile);
    }
    syslog(LOG_DEBUG, "Writing '%s' to %s", writestr, writefile);
    printf("Writing '%s' to %s", writestr, writefile);

    fclose(fileptr);
    closelog();
    return 0;
}
