#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include "server.h"
#include "message_queue.h"

// Processes a command message (dispatches to the correct handler)
void process_command(const Message& msg);
void handle_stats(const Message& msg);
void handle_list(const Message& msg);
void handle_msg(const Message& msg);
void handle_unknown(const Message& msg);
void handle_register(const Message& msg);
void handle_login(const Message& msg);
void handle_removeuser(const Message& msg);


#endif // COMMAND_PROCESSOR_H
