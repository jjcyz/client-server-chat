#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include "message_queue.h"

void process_command(const Message& msg);
void message_worker();

#endif // COMMAND_PROCESSOR_H
