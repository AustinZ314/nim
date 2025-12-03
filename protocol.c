#include <ctype.h>

#define MAX_LEN 104

typedef struct {
    char type[5];
    char body[MAX_LEN + 1];
    char *fields[3];
    int field_count;
} Message;

// read in an exact number of bytes for the next field
int read_field(int fd, char *buf, int num_bytes, int expected_bars) {
    int bytes_read = 0;
    int bars = 0;
    while(bytes_read < num_bytes) {
        int bytes = read(fd, buf + bytes_read, num_bytes - bytes_read);
        if(bytes <= 0) {
            return -2; // error
        }
        bytes_read += bytes;

        for(int i = bytes_read - bytes; i < bytes_read; i++) {
            if(buf[i] == '|') bars++;
        }

        if(bars > expected_bars || (bars >= expected_bars && bytes_read < num_bytes)) return -1; // 10 Invalid error
    }

    return 0;
}

// check the individual fields and return 0 for success, 1 for error, 2 for disconnected
int parse_message(int fd, Message *msg) {
    // read the protocol version and check if its valid
    char version[2];
    if(read_field(fd, version, 2, 1) < 0) return -2; // client disconnected
    if(version[0] != '0' || version[1] != '|') return -1; // 10 Invalid error

    // read the message length and check if its valid
    char length[3];
    if(read_field(fd, length, 3, 1) < 0) return -2; // client disconnected
    if(!isdigit(length[0]) || !isdigit(length[1]) || length[2] != '|') return -1; // 10 Invalid error
    
    char convert[3] = {length[0], length[1], '\0'};
    int real_length = atoi(convert) - 5;
    if(real_length < 0 || real_length > (MAX_LEN - 10)) return -1; // 10 Invalid error

    // read the message type and check if its valid
    if(read_field(fd, msg->type, 5, 1) < 0) return -2; // client disconnected
    if(msg->type[4] != '|') return -1; // 10 Invalid error
    msg->type[4] = '\0';

    int expected_fields = 0;
    if (strncmp(msg->type, "WAIT", 4) == 0) expected_fields = 0;
    else if(strncmp(msg->type, "OPEN", 4) == 0 || strncmp(msg->type, "FAIL", 4) == 0) expected_fields = 1;
    else if(strncmp(msg->type, "NAME", 4) == 0 || strncmp(msg->type, "PLAY", 4) == 0 || strncmp(msg->type, "MOVE", 4) == 0) expected_fields = 2;
    else if(strncmp(msg->type, "OVER", 4) == 0) expected_fields = 3;
    else return -1; // 10 Invalid error

    // read the body of the message and check if its valid
    if(read_field(fd, msg->body, real_length, expected_fields) < 0) return -2; // client disconnected
    msg->body[real_length] = '\0';

    if(msg->body[real_length - 1] != '|') return -1; // 10 Invalid error

    msg->field_count = 0;
    char *cur = msg->body;
    for(int i = 0; i < real_length; i++) {
        if(msg->body[i] == '|') {
            if(msg->field_count >= expected_fields) return -1; // 10 Invalid error
            msg->body[i] = '\0';
            msg->fields[msg->field_count] = cur;
            msg->field_count++;
            cur = msg->body + i + 1;
        }
    }
    if(msg->field_count != expected_fields) return -1; // 10 Invalid error

    return 0;
}