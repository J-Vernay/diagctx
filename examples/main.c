#include "../diagctx.h"

/* This example will read an ASCII string and returns the number of uppercase letters per line.
 * An error is triggered if a non-ASCII character is found. */


#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>


/************ types and functions related to diagctx ************/

typedef struct {
    char* str; /* dynamically-allocated, needs to be free(). */
} Message;

void destroy_Message(void* msg) {
    free( ((Message*)msg)->str );
}

/* using a macro because va_copy does not exist in C89 */
#define DEBUG_CTX(id_name, ...) \
    unsigned id_name; \
    do { Message* msg = (Message*) diagctx_push(&id_name); \
         if (msg) { \
             size_t size = 1 + (int) snprintf(NULL, 0, __VA_ARGS__); \
             msg->str = malloc(size); \
             snprintf(msg->str, size, __VA_ARGS__); \
         } \
    } while(0)

void debug_handler(void* indent_level, void* message) {
    int* indent_lvl = (int*) indent_level;
    int i, imax;
    for (i = 0, imax = *indent_lvl; i < imax; ++i)
        fputs("  ", stderr);
        
    if (message == NULL)
        fputs("??? (no memory available)", stderr);
    else
        fputs(((Message*) message)->str, stderr);
    fputc('\n', stderr);
    ++*indent_lvl;
}

/******************** ACTUAL PROGRAM *********************/

jmp_buf error_handling_jmp;

int count_uppercase_ascii(char const* str, int length) {
    int count = 0;
    int i;
    DEBUG_CTX(msg_id, "count_uppercase_ascii(\"%.*s\", %i)", length, str, length);
    for (i = 0; i < length; ++i) {
        unsigned char c = str[i];
        if (c >= 128) {
            /* non-ascii detected */
            DEBUG_CTX(msg_id_2, "error: found '\\x%.2X' at position %d", c, i);
            longjmp(error_handling_jmp, 1);
            /* diagctx_pop(diagmsg_id_2); not needed because longjmp will exit the scope. */
        }
        count += (c >= 'A' && c <= 'Z');
    }
    diagctx_pop(msg_id);
    return count;
}

void for_each_line(char const* str) {
    DEBUG_CTX(msg_id, "for_each_line()");
    
    int line_number = 0;
    while (1) {
        ++line_number;
        int line_size = (int) strcspn(str, "\n");
        
        if (setjmp(error_handling_jmp) == 0) {
            DEBUG_CTX(msg_id_2, "line %d", line_number);
            
            int nb_upper = count_uppercase_ascii(str, line_size);
            printf("Line %d: %d upper characters\n", line_number, nb_upper);
            
            diagctx_pop(msg_id_2);
        } else {
            /* error handling */
            fputs("ERROR!\n", stderr);
            int indent_level = 1;
            diagctx_get(msg_id, debug_handler, &indent_level);
        }
        
        str += line_size;
        if (*str == '\0') break;
        ++str; /* skipping '\n' */
    }
    diagctx_pop(msg_id);
}

int main() {
    Message messages_buffer[10];
    diagctx_init(sizeof(Message), messages_buffer, 3, destroy_Message);
    
    DEBUG_CTX(msg_id, "main()");
   
    for_each_line("Hello World!\n"
                  "ABC def GHI jlk\n"
                  "Hello \x86 World!\n"
                  "\x97 test\n"
                  "\x80\x81\x82\n"
                  "THE END!");
    
    diagctx_pop(msg_id);
    return 0;
}


