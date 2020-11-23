#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sstream>

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Wrong number of arguments.";
        return -1;
    }

    std::stringstream port_stream(argv[1]);
    in_port_t server_port = 0;
    port_stream >> server_port;

    if (port_stream.fail())
    {
        std::cerr << "Invalid port.";
        return -2;
    }

    auto listener_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (listener_socket == -1)
    {
        std::cerr << "Socket creation failed with errno: " << errno;
        return -3;
    }

    sockaddr_in server_address = { .sin_family = AF_INET,
                                   .sin_port = server_port,
                                   .sin_addr = { .s_addr = INADDR_ANY } };

    if (bind(listener_socket,
             reinterpret_cast<sockaddr *>(&server_address),
             sizeof(server_address)) == -1)
    {
        std::cerr << "Bind failed with errno: " << errno;
        return -4;
    }

    return 0;
}
