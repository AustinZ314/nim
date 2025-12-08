#ifndef PROTOCOL_H
#define PROTOCOL_H

#define MAX_LEN 104

typedef struct {
    char type[5];
    char body[MAX_LEN + 1];
    char *fields[3];
    int field_count;
} Message;

int read_field(int fd, char *buf, int num_bytes, int expected_bars);
int parse_message(int fd, Message *msg);

#endif