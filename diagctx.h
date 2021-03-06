/*
This is the C header for the diagctx library, written by Julien Vernay ( jvernay.fr ) in 2021.
It contains both the API and the documentation.
diagctx is a library to log context (= arbitrary data) and retrieve it in case of errors.
The library is available under the Boost Software License 1.0, whose terms are below:

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#ifndef JVERNAY_DIAGCTX
#define JVERNAY_DIAGCTX

#ifdef __cplusplus
extern "C" {
#endif

/*
 * diagctx is meant for the programmer to store context for future error logging.
 * Such context is called a "message", which can be any arbitrary byte sequence.
 * Adding context is called "push" and removing context is called "pop".
 *
 * For example, you can push the message "While parsing file '/path/to/file.c':"
 * and then push "Parsing token '3.14159f'...".
 * Then, at any moment you can get the context with diagctx_get().
 * So, if an error occurs in a function which cannot handle it, it can
 * print the context and then call exit(EXIT_FAILURE).
 *
 * More importantly, it can be used with C longjmp and C++ exceptions.
 * For instance, in the following stack, with associated messages:
 *     main()
 *       parse_multiple_files() "Parsing /path/to/*.c"
 *         parse_file() "Parsing /path/to/main.c"
 *           parse_file() "Parsing /path/to/library.h"
 *             tokenize_line() "Line 58"
 *               tokenize() "Tokenization of '3.14159g'
 * parse_multiple_files() register an error handler (either C setjmp() or C++ catch-block).
 * tokenize() will generate an error due to invalid 'g' suffix.
 * Due to this error, the execution reenters parse_multiple_files().
 * Then parse_multiple_files() can call diagctx_get() to have access to all the pushed messages.
 * 
 * Another example is to push "__FILE__:__LINE__ in __func_name__".
 * Then, at exit you can log the stacktrace (using atexit()). The stacktrace will be empty
 * at the end of main(), or it will contain context in case of early exit.
 */




/* Initialize diagctx, must be called before any other function.
 * diagctx will use a buffer of 'message_size * capacity' bytes.
 * 'msg_destructor' is called when 'msg' is not used anymore. 'msg' will never be NULL.
 *  'msg_destructor' can be NULL if nothing needs to be done.
 * Example with dynamic allocation in C:
 *     diagctx_init(sizeof(struct MyMessage),
 *                  malloc(sizeof(struct MyMessage) * 20, 20, NULL);
 * Example without dynamic allocation in C:
 *     struct MyMessage messages[20];
 *     diagctx_init(sizeof(struct MyMessage), messages, 20, NULL);
 * Example without dynamic allocation in C++:
 *     std::aligned_storage<MyMessage[20]> messages;
 *     diagctx_init(sizeof(MyMessage), &messages, 20,
 *         [](void* msg) { static_cast<MyMessage*>(msg)->~MyMessage(); }
 *     );
 */
void diagctx_init(unsigned message_size,
                  void* buffer,
                  unsigned capacity,
                  void(*msg_destructor)(void* msg));

/* Push a message slot to provide context to a future error.
 * 'msg_id' will store the position of the message, which is needed for the other functions.
 * Returns a pointer to where the message can be put, or NULL if no space is available.
 * Example:
 *      unsigned diagmsg_id;
 *      struct MyMessage * diagmsg = diagctx_push(&diagmsg_id);
 *      if (diagmsg != NULL) *diagmsg = (struct MyMessage){ ... };
 *      ... operations ...
 *      diagctx_pop(diagmsg_id);
 */
void* diagctx_push(unsigned* msg_id);

/* Pop 'diagmsg'. Must be called when the context is obsolete.
 * Each diagctx_push() must have its diagctx_pop() counterpart. */
void diagctx_pop(unsigned msg_id);


/* Signature of the function needed for diagctx_get().
 * 'userdata' is for your use only, and is provided by you in the call to diagctx_get().
 * 'message' will contain the pointers returned by diagctx_push(), including NULL if no space was available.
 *
 * Example in C++:
 *     void my_handler(void* userdata, void* message) {
 *         MyMessage* msg = *(MyMessage*)(message);
 *         if (msg != nullptr)
 *             msg->print();
 *         else
 *             std::puts("??? (no memory available)");
 *     }
 */
typedef void diagctx_handler_t(void* userdata, void* message);

/* Iterate other the pushed messages, and call 'handler' for each message.
 * `msg_id' can be specified as -1, in which case it will iterate over all the messages.
 * After the iteration, destroy obsolete messages which have not been popped via diagctx_pop()
 * due to the distant jump (either C longjmp or C++ throw-catch).
 * For this reason, if you use distant error handling, you must call diagctx_get() at least
 * once at the end of your error handling logic, so that the messages are back
 * in a coherent state. 'handler' may be NULL to only do message popping.  */
void diagctx_get(unsigned msg_id, diagctx_handler_t* handler, void* userdata);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif
