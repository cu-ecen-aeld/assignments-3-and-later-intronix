#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>

void log_message(const char *message, int priority) {
    openlog("writer", LOG_PID | LOG_CONS, LOG_USER);
    syslog(priority, "%s", message);
    closelog();
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <file> <string>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *file_path = argv[1];
    const char *text = argv[2];
    FILE *file = fopen(file_path, "w");

    if (file == NULL) {
        char error_message[256];
        snprintf(error_message, sizeof(error_message), "Failed to open file %s: %s", file_path, strerror(errno));
        log_message(error_message, LOG_ERR);
        fprintf(stderr, "Error: %s\n", error_message);
        exit(EXIT_FAILURE);
    }

    if (fprintf(file, "%s\n", text) < 0) {
        char error_message[256];
        snprintf(error_message, sizeof(error_message), "Failed to write to file %s: %s", file_path, strerror(errno));
        log_message(error_message, LOG_ERR);
        fclose(file);
        fprintf(stderr, "Error: %s\n", error_message);
        exit(EXIT_FAILURE);
    }

    log_message("Writing to file succeeded", LOG_DEBUG);
    fclose(file);

    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Writing \"%s\" to %s", text, file_path);
    log_message(log_msg, LOG_DEBUG);

    return EXIT_SUCCESS;
}
