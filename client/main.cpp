#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iomanip>

#define BUFFER_SIZE 128

int testMessage(std::string& message, int socket);

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Wrong number of arguments." << "\n";
        return -1;
    }

    std::stringstream port_stream(argv[2]);
    in_port_t server_port = 0;
    port_stream >> server_port;

    if (port_stream.fail())
    {
        std::cerr << "Invalid port." << "\n";
        return -2;
    }

    // Creating socket.
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket == -1)
    {
        std::cerr << "Socket creation failed with errno: " << std::strerror(errno) << "\n";
        return -3;
    }

    sockaddr_in server_address = { .sin_family = AF_INET,
                                   .sin_port = htons(server_port) };
    if (inet_aton(argv[1], &server_address.sin_addr) == 0)
    {
        std::cerr << "Failed to parse address." << "\n";
        return -4;
    }

    // Connecting to server.
    if (connect(server_socket,
                reinterpret_cast<sockaddr *>(&server_address),
                sizeof(server_address)) == -1)
    {
        std::cerr << "Connect failed with errno: " << std::strerror(errno) << "\n";
        return -5;
    }

    std::vector<std::string> messages = {
            { '\x01', '\x02', '\x03', '\x00' },
            { '\xFF', '\xFE', '\x1A', '\x0B', '\x02', '\x01', '\x00', '\x30', '\x00' },
            { '\x01', '\x01', '\x03', '\x00', '\xAA', '\xAA', '\xBB', '\x00'},
            { '\x03', '\x00', '\x00', '\x00', '\x01', '\x00', '\x00'}
    };

    for (auto& message : messages)
    {
        int results = testMessage(message, server_socket);

        //testMessage function failed, close socket and exit.
        if (results < 0)
        {
            close(server_socket);
            return results;
        }
    }

    close(server_socket);
}

int testMessage(std::string& message, int socket)
{
    ssize_t result = send(socket,
                          message.data(),
                          message.size(),
                          0);

    if (result == -1)
    {
        std::cerr << "Send failed with errno: " << std::strerror(errno) << "\n";
        return -7;
    }

    size_t leftToReceive = message.size();

    std::string reversedMessage;
    while (leftToReceive > 0)
    {
        char message_buffer[BUFFER_SIZE] = {};

        ssize_t received_bytes = recv(socket,
                                      message_buffer,
                                      BUFFER_SIZE,
                                      0);

        if (received_bytes > leftToReceive)
        {
            std::cerr << "Received more bytes than expected.\n";
            return -8;
        }
        if (received_bytes == -1)
        {
            std::cerr << "recv failed with errno: " << std::strerror(errno) << "\n";
            return -9;
        }
        else if (received_bytes == 0)
        {
            std::cerr << "Socket shut down by server: "<< "\n";
            return -9;
        }
        else
        {
            leftToReceive -= received_bytes;
            reversedMessage.append(message_buffer, received_bytes);
        }

        std::cout << "TCP Stream Input (hex): ";
        for (auto& character : message)
        {
            std::cout << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << (0xFF & character) << " " << std::dec;
        }
        std::cout << "\n" << "TCP Stream Output (hex): ";
        for (auto& character : reversedMessage)
        {
            std::cout << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << (0xFF & character) << " " << std::dec;
        }
        std::cout << "\n\n";
    }

    return 0;
}
