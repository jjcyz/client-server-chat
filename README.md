# Chat Application

This repository contains a simple chat application implemented in C++ using sockets for communication between a server and multiple clients. The application supports basic functionalities such as broadcasting messages, listing active users, viewing chat history, and sending private messages.

## Features

- **Broadcast Messages**: Clients can send messages to all connected users.
- **Private Messaging**: Send private messages to specific users using commands.
- **User Management**: Join and leave notifications for users.
- **Chat History**: Clients can request the chat history.
- **List Active Users**: Clients can list all active users.

## Getting Started

#### Prerequisites

- A C++ compiler (e.g., g++)
- POSIX-compliant operating system (Linux, macOS)

#### Building the Application
1. Clone the repository:
   ```sh
	git clone https://github.com/yourusername/chat-application.git
	cd chat-application
2. Compile the server and client programs:
   ```sh
	 g++ -std=c++11 -o server server.cpp
	 g++ -std=c++11 -o client client.cpp
#### Running the Application
1. Running the Server:
   ```sh
	 ./server
2. Running the client:
   ```sh
	 ./client
3. Enter your username when prompted.
4. Start chatting!

### Commands
The client supports the following commands:
- **Send a message**: Just type your message and press Enter.
- **Private message**: `/msg <username> <message>` - Sends a private message to the specified user.
- **List active users**: `/list` - Lists all active users.
- **View chat history**: `/history` - Displays the chat history.
- **Exit**: Type `exit` to leave the chat.

### Code Overview
#### Server (`server.cpp`)
- **Main function**: Initializes the server, binds to the specified port, listens for incoming connections, and spawns a new thread for each connected client.
- **`handle_client` function**: Manages communication with a connected client, handles commands, and broadcasts messages.
- **`broadcast` function**: Sends messages to all connected clients.
- **`send_join_message` and `send_leave_message` functions**: Notify all users when someone joins or leaves.
- **`handle_command` function**: Processes special commands like `/msg`, `/list`, and `/history`.

#### Client (`client.cpp`)

- **Main function**: Connects to the server, handles user input, and sends messages.
- **`receive_messages` function**: Runs in a separate thread to continuously receive messages from the server and display them to the user.

### What's Next
- **Error Handling**:
Error Messages: Provide informative error messages to users in case of failures or unexpected scenarios. Clearly communicate the nature of the error and any steps the user can take to resolve it. Log detailed error messages on the server side for debugging purposes. Exception Handling: Use exception handling mechanisms (try-catch blocks in C++) to gracefully handle runtime errors and exceptional conditions. Catch and handle exceptions at appropriate levels of abstraction to prevent crashes and maintain application stability.
- **User Interface (UI) Enhancement**:
Interactive Elements: Incorporate interactive elements such as buttons, input fields, and menus to streamline user interactions and make the application more user-friendly. Provide visual feedback (e.g., progress indicators, tooltips) to enhance usability. Message Formatting: Support basic text formatting options (e.g., bold, italic, underline) to allow users to express themselves more creatively in chat messages. Implement rich text rendering capabilities to display formatted messages in the chat interface.
- **Security Considerations**:
- Authentication: Implement user authentication mechanisms to verify the identity of clients and prevent unauthorized access to the chat system. Use strong authentication methods such as username/password authentication, OAuth, or token-based authentication. Access Control: Enforce access control policies to restrict user privileges and permissions based on roles and permissions. Define user roles (e.g., admin, regular user) and limit access to sensitive functionalities (e.g., administrative commands, chat history) accordingly.


