//C Server

#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>


#define MSG_BUFFER_SIZE 1024 
#define MAX_CLIENTS 1000
        

int main() {
    printf("C Server!\n");

    int server_socket;
    int protocol;
    struct protoent *proto_entry;

    struct sockaddr_in listening_address;

    const char *address = "127.0.0.1";
    in_port_t port = 49500; 

    //getprotobyname searches for the protocol number
    const char *proto_name = "tcp";
    proto_entry = getprotobyname(proto_name);    

    if(proto_entry == NULL) {
        perror("Failed to get TCP protocol");
        exit(1);
    } else {
        printf("Protocol successfully defined\n");
    }

    protocol = proto_entry->p_proto;

    // Creates the socket: AF_INET, SOCK_STREAM, protocol
    server_socket = socket(AF_INET, SOCK_STREAM, protocol);

    if(server_socket < 0) {
        fprintf(stderr, "Socket() failed: %s\n", strerror(errno));
        exit(1);
    } else {
        printf("Socket %d has been opened!\n", server_socket);
    }
    
    //Defining the listening address
    memset(&listening_address, 0, sizeof(listening_address));
    listening_address.sin_family = AF_INET;
    listening_address.sin_port = htons(port);
    listening_address.sin_addr.s_addr = inet_addr(address);

    // Reuse Address
    int enable_reuse = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable_reuse, sizeof(enable_reuse)); 

    //Binding
    int bound = bind(server_socket, (struct sockaddr *)&listening_address, sizeof(listening_address)) ;
    if (bound < 0) {
        fprintf(stderr, "bind failed: %s\n", strerror(errno));
        close(server_socket);
        exit(1);
    } else {
        printf("Bind successful\n");
    }

    //Listening
    int is_listening = listen(server_socket, SOMAXCONN);
    if (is_listening < 0) {
        fprintf(stderr, "Cannot listen: %s\n", strerror(errno));
        close(server_socket);
        exit(1);
    } else {
        printf("Listen succeeded\n"); 
    }

    //Creating the array of structs pollfd for poll()
    struct pollfd fds[MAX_CLIENTS + 1];

    //Defining the elements of the struct pollfd
    fds[0].fd = server_socket;
    fds[0].events = POLLIN;
    fds[0].revents = 0; 

    for(int i = 1; i < MAX_CLIENTS + 1; i++) {
        fds[i].fd = -1;
        fds[i].events = POLLIN;
        fds[i].revents = 0; 
    }    

    //Specify the number of itens in the array fds
    nfds_t nfds = 1;

    //Defining the timeout in milliseconds. Example: 1000ms = 1s
    int timeout = 1000;

    //Client ID
    static int client_id = 0;

    //Client personal space in memory
    typedef struct {
        int id;
        int socket;
        char ip[INET_ADDRSTRLEN];
        uint16_t port;
        char msg_buffer[MSG_BUFFER_SIZE];
        size_t msg_len; 
        char recv_buffer[1024];
        ssize_t recv_bytes;
    } Client;

    static Client clients[MAX_CLIENTS];

    //Main loop
    while(1) {
        //printf("Main Loop! Waiting for new connections...\n");

        //Accepting connections
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);
        int new_connection_socket;

        char client_ip[INET_ADDRSTRLEN];
        uint16_t client_port;

        int ret_poll = poll(fds, nfds, timeout);
        if(ret_poll < 0) {
            printf("poll() failed - errno: %d(%s)\n", errno, strerror(errno));
            break;
        }
        if(ret_poll == 0) {
            //Timeout with no events
            //printf("poll() timed out with no events within %d ms\n", timeout);
            continue;
        }
        if(fds[0].revents & POLLIN) {      

            new_connection_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_len);
           
            if(new_connection_socket < 0) {
                fprintf(stderr, "Failed to accept(): %s\n", strerror(errno));
                continue;
            } else {
                client_id++;
                printf("Client id: %d connected with socket %d\n", client_id, new_connection_socket); 
            }

            fds[client_id].fd = new_connection_socket;
            nfds++;

            //Get peer name
            struct sockaddr_in peer_address;
            socklen_t peer_len = sizeof(peer_address);
            int got_peer_name = getpeername(new_connection_socket, (struct sockaddr *)&peer_address, &peer_len);

            
            if(got_peer_name < 0) {
                fprintf(stderr, "Failed to getpeername(): %s\n", strerror(errno));
            } else {
                strncpy(client_ip, inet_ntoa(peer_address.sin_addr), sizeof(client_ip)); 
                client_ip[sizeof(client_ip) -1] = '\0';

                client_port = ntohs(peer_address.sin_port);
                printf("Connected client has IP: %s, and Port: %u\n", client_ip, client_port);
            }

            //Sending a message to the client
            const char *message = "Hello from the C server!\n\r";
            ssize_t sent_message = send(new_connection_socket, message, strlen(message), 0);
            if(sent_message < 0) {
            fprintf(stderr, "Failed to send message to the client: %s\n", strerror(errno)); 
            }
            
            //Filling the client fields
            clients[client_id - 1].id = client_id;
            clients[client_id - 1].socket = new_connection_socket;
            strncpy(clients[client_id - 1].ip, client_ip, sizeof(clients[0].ip));
            clients[client_id - 1].port = client_port;

            //Listing all connected clients
            int connected_count = 0;
            
            for(int j = 0; j < MAX_CLIENTS; j++) {
                if(clients[j].id != 0) { //Valid client 
                    connected_count++;                    
                }    
            }
            printf("We have now %d connected clients:\n", connected_count);

            for(size_t i = 0; i < (size_t)client_id; i++) {
                if(clients[i].id != 0) { 
                    printf("CLIENT %d CONNECTED WITH IP: %s AND PORT: %u\n",
                        clients[i].id, clients[i].ip, clients[i].port);
                }    
            }

            //Sending a message to the connected clients through the clients array
            //const char *client_set_message = "You have now a personal space\r\n";
            char client_set_message[128];
            snprintf(client_set_message, sizeof(client_set_message), 
            "You have now a personal space with ID: %d\r\n", client_id);

            if(send(clients[client_id - 1].socket, client_set_message, 
                strlen(client_set_message), 0) == -1) {

                fprintf(stderr, "Failed to send client_set_message to the client: %s\n", 
                    strerror(errno));    
            }        
               
        }
        //Back to Main Loop           
        
        //User Session                   
        
        //Receiving messages            
        
        for(size_t i = 0; i < (size_t)client_id; i++) {
            memset(clients[i].recv_buffer, 0, sizeof(clients[i].recv_buffer));
            //creating an index for fds
            int fds_index = i + 1;
            if(fds[fds_index].fd == -1) {
                continue;
            }

            if(fds[fds_index].revents & POLLIN) {
                clients[i].recv_bytes = recv(clients[i].socket, clients[i].recv_buffer, sizeof(clients[i].recv_buffer) -1, 0);
                           
                
                //Detecting recv() errors
                if(clients[i].recv_bytes < 0) {
                    fprintf(stderr, "Failed to recv(): %s\n", strerror(errno));
                    continue;
                } 

                //Detecting client disconnection
                if(clients[i].recv_bytes == 0) { 
                    printf("Client ID: %d - %s:%u disconnected (Close FD: %d)\n", clients[i].id, clients[i].ip, clients[i].port, clients[i].socket);
                    //break;
                    if(fds[fds_index].fd > 0) {
                        close(fds[fds_index].fd);
                    }

                    
                    //Sending a disconnection message                    
                    char disconnection_message[128];
                    snprintf(disconnection_message, sizeof(disconnection_message), 
                        "Server said: client %d left\r\n", clients[i].id);
                   
                    for(int j = 0; j < MAX_CLIENTS; j++) {
                        if(clients[j].id != 0 && j != i) {
                            //Verify the status of the socket before sending the message
                            struct sockaddr_in client_socket_status;
                            socklen_t css_len = sizeof(client_socket_status);
                            int peer_status = getpeername(clients[j].socket, (struct sockaddr *)&client_socket_status, &css_len);
                            if(peer_status == -1) {
                                continue;
                            }

                            ssize_t bytes_sent = send(clients[j].socket, disconnection_message, 
                                strlen(disconnection_message), 0);
                        
                            if(bytes_sent < 0) {
                                fprintf(stderr, "Failed to send disconnection message to the client %d: %s\n", 
                                clients[j].id, strerror(errno));
                            }    
                            else {
                                printf("Disconnection message was sent to client %d\n", clients[j].id); 
                            }

                        }   

                    }


                    fds[fds_index].fd = -1;
                    fds[fds_index].revents = 0;
                    memset(&clients[i], 0, sizeof(Client));   

                    //Verifying clients which are still connected
                    int connected_count = 0;
                    printf("AFTER CLIENT DISCONNECTION WE STILL HAVE THE FOLLOWING CLIENTS CONNECTED\n");

                    for(int j = 0; j < MAX_CLIENTS; j++) {
                        if(clients[j].id != 0) { //Valid client 
                            connected_count++;
                            printf("CLIENT %d CONNECTED WITH IP: %s AND PORT: %u\n",
                                clients[j].id, clients[j].ip, clients[j].port);
                        }    
                    }

                    //No connected client
                    if(connected_count == 0) {
                        printf("NO CLIENTS REMAINING - SERVER WAITING FOR NEW CONNECTIONS\n");         
                    } else {
                        printf("TOTAL REMAINING CLIENTS: %d\n", connected_count);
                    }

                    printf("------------------------------\n");
                    continue;

                }

                clients[i].recv_buffer[clients[i].recv_bytes] = '\0';
                //printf("Received from client %d: %s\n", clients[i].id, clients[i].recv_buffer);  
                

                // Fix the overflow
                if(clients[i].msg_len + clients[i].recv_bytes > MSG_BUFFER_SIZE ) {
                    printf("Buffer Overflow! Cleaning the buffer. Your message will be RESET!\n"); 
                    clients[i].msg_len = 0; 
                }

                memcpy(clients[i].msg_buffer + clients[i].msg_len, clients[i].recv_buffer, clients[i].recv_bytes);
                clients[i].msg_len += clients[i].recv_bytes; 
                clients[i].msg_buffer[clients[i].msg_len] = '\0';

                //Temp Message
                //printf("Temp message from client %d: %s\n", clients[i].id, clients[i].msg_buffer);            
                
                //Accumulate the data received in the buffer of each client
                char *newline_ptr;
                while(newline_ptr = strchr(clients[i].msg_buffer, '\n')) {

                    //printf("Message ended with ENTER\n");
                                
                    size_t complete_len = (size_t)(newline_ptr - clients[i].msg_buffer) + 1;
                    char complete_message[MSG_BUFFER_SIZE + 1];
                    memcpy(complete_message, clients[i].msg_buffer, complete_len);
                    complete_message[complete_len] = '\0';

                    printf("Message from client %d - %s:%d: %s\n", clients[i].id, clients[i].ip, clients[i].port, complete_message);
                    
                    /*
                    //Converting to upper case
                    for(size_t index = 0; index < complete_len; index++) {
                        if(complete_message[index] >= 'a' && complete_message[index] <= 'z') {
                            complete_message[index] = complete_message[index] - 'a' + 'A';
                        }
                    }    
                    
                    const char *message_header = "Server sent: ";
                    send(clients[i].socket, message_header, strlen(message_header), 0);

                    //send the message back to the client
                    ssize_t bytes_sent = send(clients[i].socket, complete_message, complete_len, 0);
                    if(bytes_sent < 0) {
                        fprintf(stderr, "Failed to reflect the message back to the client: %s\n", strerror(errno)); 
                    }   
                    */
                   
                    
                    //Header to the client's message
                    const char *client_message_header = "Client %d said: %s";
                    size_t cmh_size = strlen(client_message_header);
                    char reflection_message[MSG_BUFFER_SIZE + cmh_size];
                    snprintf(reflection_message, sizeof(reflection_message), 
                        client_message_header, clients[i].id, complete_message);

                    //Send the received message to all other connected clients
                    for(int j = 0; j < MAX_CLIENTS; j++) {
                        if(clients[j].id != 0 && j != i) {
                            ssize_t bytes_sent = send(clients[j].socket, reflection_message, 
                                strlen(reflection_message), 0);
                        
                            if(bytes_sent < 0) {
                                fprintf(stderr, "Failed to send message to the client %d: %s\n", 
                                clients[j].id, strerror(errno));
                            }    
                            else {
                                printf("Reflection was sent to client %d\n", clients[j].id); 
                            }

                        }   

                    }
                    
                    // renewing the buffer 

                    // How many characters remain in the msg_buffer after \n
                    size_t remaining = clients[i].msg_len - complete_len;
                    //printf("Number of characters remaining in the buffer after newline: %zu\n", remaining);
                    
                    //Move byte per byte everything thats stored after \n
                    //to the beginning of the buffer
                    //for(size_t j = 0; j < remaining; j++) {
                    //    clients[i].msg_buffer[j] = clients[i].msg_buffer[complete_len + j];
                    //}
                    memmove(clients[i].msg_buffer, clients[i].msg_buffer + complete_len, remaining);
                    clients[i].msg_len = remaining;
                    clients[i].msg_buffer[clients[i].msg_len] = '\0';                
                
                }
                
            }
        
        }
            
    }
        
	
    //try to always close the socket, if possible
    close(server_socket);

    return 0;
  
}