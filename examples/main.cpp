#include "../diagctx.h"

/* This example will read an ASCII string and returns the number of uppercase letters per line.
 * An error is triggered if a non-ASCII character is found. */


#include <sstream>
#include <string_view>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <type_traits>

using namespace std;


/************ types and functions related to diagctx ************/

struct Message {
    ostringstream out;
};

template<typename...Args>
unsigned debug_ctx(Args...args) {
    unsigned id;
    Message* msg = (Message*) diagctx_push(&id);
    if (msg != NULL) {
        new (msg) Message;
        (msg->out << ... << args);
    }
    return id;
}


void debug_handler(void* indent_level, void* message) {
    int* indent_lvl = (int*) indent_level;
    int i, imax;
    for (i = 0, imax = *indent_lvl; i < imax; ++i)
        std::cerr << "  ";
        
    if (message == NULL)
        std::cerr << "??? (no memory available)";
    else
        std::cerr << static_cast<Message*>(message)->out.str();
    std::cerr << '\n';
    ++*indent_lvl;
}



/******************** ACTUAL PROGRAM *********************/


int count_uppercase_ascii(string_view str) {
    int count = 0;
    int i;
    unsigned msg_id = debug_ctx("count_uppercase_ascii(\"", str, "\")");
    for (int i = 0, length = str.size(); i < length; ++i) {
        unsigned char c = str[i];
        if (c >= 128) {
            /* non-ascii detected */
            unsigned msg_id_2 = debug_ctx("error: found '\\x",  " at position ", i);
            throw std::invalid_argument("Non-ASCII char");
            /* diagctx_pop(diagmsg_id_2); not needed because throw will exit the scope. */
        }
        count += (c >= 'A' && c <= 'Z');
    }
    diagctx_pop(msg_id);
    return count;
}

void for_each_line(string_view str) {
    unsigned msg_id = debug_ctx("for_each_line()");
    
    int line_number = 0;
    while (1) {
        ++line_number;
        int line_size = min(str.find('\n'), str.size());
        
        try {
            unsigned msg_id_2 = debug_ctx("line ", line_number);
            
            int nb_upper = count_uppercase_ascii(str.substr(0, line_size));
            cout << "Line " << line_number << ": " << nb_upper << " upper characters.\n";
            
            diagctx_pop(msg_id_2);
        } catch (exception const& e) {
            // error handling
            cerr << "ERROR! " << e.what() << "\n";
            int indent_level = 1;
            diagctx_get(msg_id, debug_handler, &indent_level);
        }
        
        str.remove_prefix(line_size);
        if (str.empty()) break;
        str.remove_prefix(1); // skipping '\n'
    }
    diagctx_pop(msg_id);
}


int main() {
    std::aligned_union<0, Message[10]> messages_buffer;

    diagctx_init(sizeof(Message), &messages_buffer, 10,
        [] (void* msg) { static_cast<Message*>(msg)->~Message(); });
    
    unsigned msg_id = debug_ctx("main()");
    
    for_each_line("Hello World!\n"
                  "ABC def GHI jlk\n"
                  "Hello \x86 World!\n"
                  "\x97 test\n"
                  "\x80\x81\x82\n"
                  "THE END!");
    
    diagctx_pop(msg_id);
    return 0;
}
