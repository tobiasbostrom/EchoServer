#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <csignal>

static int listener_socket;
static int client_socket;

#define BUFFER_SIZE 4096

void sig_handler(int signo)
{
    std::cerr << "Received signal, closing sockets." << "\n";

    close(listener_socket);
    close(client_socket);
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Wrong number of arguments." << "\n";
        return -1;
    }

    std::stringstream port_stream(argv[1]);
    in_port_t server_port = 0;
    port_stream >> server_port;

    if (port_stream.fail())
    {
        std::cerr << "Invalid port." << "\n";
        return -2;
    }

    // Creating socket.
    listener_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (listener_socket == -1)
    {
        std::cerr << "Socket creation failed with errno: " << std::strerror(errno) << "\n";
        return -3;
    }

    sockaddr_in server_address = { .sin_family = AF_INET,
                                   .sin_port = htons(server_port),
                                   .sin_addr = { .s_addr = INADDR_ANY } };

    // Binding to local address.
    if (bind(listener_socket,
             reinterpret_cast<sockaddr *>(&server_address),
             sizeof(server_address)) == -1)
    {
        std::cerr << "Bind failed with errno: " << std::strerror(errno) << "\n";
        return -4;
    }

    // Setting socket to listen for connections.
    if (listen(listener_socket, SOMAXCONN) == -1)
    {
        std::cerr << "Marking as listening socked failed with errno: " << std::strerror(errno) << "\n";
        return -5;
    }

    while(true)
    {
        sockaddr_in client_address = { };
        socklen_t client_addr_size = sizeof(client_address);
        client_socket = accept(listener_socket,
                               reinterpret_cast<sockaddr *>(&client_address),
                               &client_addr_size);
        if (client_socket == -1) {
            std::cerr << "Accepting connection from client failed with errno: " << std::strerror(errno) << "\n";
            return -6;
        }

        std::string message;

        while(true)
        {
            char message_buffer[BUFFER_SIZE] = { };

            ssize_t received_bytes = recv(client_socket,
                                          message_buffer,
                                          BUFFER_SIZE,
                                          0);

            if (received_bytes == -1)
            {
                std::cerr << "recv failed with errno: " << std::strerror(errno) << "\n";
                return -7;
            }
            else if (received_bytes == 0)
            {
                //Connection shut down by client.
                break;
            }
            else
            {
                for (int i = 0; i < received_bytes; i++)
                {
                    if (message_buffer[i] == '\n')
                    {
                        std::reverse(message.begin(),
                                     message.end());
                        ssize_t  result = send(client_socket,
                                               message.c_str(),
                                               message.size(),
                                               0);
                        if (result == -1)
                        {
                            std::cerr << "send failed with errno: " << std::strerror(errno) << "\n";
                            return -7;
                        }
                        message.clear();
                    }
                    else
                    {
                        // We have to add each character one at a time because
                        // there might be '\0' inside the received string.
                        message.push_back(message_buffer[i]);
                    }
                }
            }
        }

        close(client_socket);
    }
}
