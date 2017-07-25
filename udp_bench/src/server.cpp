#include <cstdlib>
#include <iostream>
#include <string>
#include <functional>
#include <asio.hpp>

using asio::ip::udp;

enum PacketType
{
    E_INFO = 1,
    E_HEARTBEAT,
    E_PACKET,
    E_BAD_PACKET,
    E_QUIT
};

class server
{
public:
    server(asio::io_service& io_service, short port)
        : io_service_(io_service),
          socket_(io_service, udp::endpoint(udp::v4(), port)),
          bytes(0), packets(0), bad_packets(0)
    {
        socket_.async_receive_from(asio::buffer(data_, max_length),
                                   sender_endpoint_,
                                   std::bind(&server::handle_receive_from, this,
                                               std::placeholders::_1,
                                               std::placeholders::_2));
    }

    void handle_receive_from(const asio::error_code& error,
                             size_t bytes_recvd)
    {
        if (!error && bytes_recvd > 0)
        {
            PacketType packet_type = checkPacket(data_, bytes_recvd);
            switch (packet_type)
            {
                case E_QUIT:
                {
                    std::stringstream sstr;
                    sstr << "Packets " << packets << "\nBytes " << bytes << "\nBad packets " << bad_packets << std::ends;

                    size_t response_length = sstr.str().length();
                    if (response_length > max_length)
                    {
                        response_length = max_length;
                    }
                    strncpy(data_, sstr.str().c_str(), response_length);

                    /* Echo back the quit message to terminate the session */
                    socket_.async_send_to(asio::buffer(data_, response_length),
                                          sender_endpoint_,
                                          std::bind(&server::handle_send_to, this,
                                                      std::placeholders::_1,
                                                      std::placeholders::_2));
                    break;
                }
                default:
                {
                    socket_.async_receive_from(asio::buffer(data_, max_length),
                                               sender_endpoint_,
                                               std::bind(&server::handle_receive_from, this,
                                                           std::placeholders::_1,
                                                           std::placeholders::_2));
                    break;
                }
            }
        }
        else
        {
            if (error)
            {
                std::cout << "Error code " << error << " bytes received " << bytes_recvd << std::endl;
                ++bad_packets;
            }

            socket_.async_receive_from(asio::buffer(data_, max_length),
                            sender_endpoint_,
                            std::bind(&server::handle_receive_from, this,
                                        std::placeholders::_1,
                                        std::placeholders::_2));
        }
        return;
    }

    void handle_send_to(const asio::error_code& /*error*/, size_t /*bytes_sent*/)
    {
        std::cout << "Quit message sent\n";
        std::cout << "Packets " << packets << "\nBytes " << bytes << "\nBad packets " << bad_packets << std::endl;
        return;	// io_service will complete with one worker
    }

private:
    bool validatePacket(const char *data, size_t length)
    {
        struct packet_header
        {
            uint32_t version_mark;  // 'quit' or version pattern
            uint32_t packet_length;
        } checkHeader;

        if (length < sizeof(packet_header))
        {
            return false;   
        }

        /* can't cast due to possible alignment issues so copy minimum amount */
        memcpy(&checkHeader, data, sizeof(packet_header));

        if (checkHeader.packet_length != length)
        {
            return false;
        }
        return true;
    }

    PacketType checkPacket(const char *data, size_t length) 
    {
        bytes += length;
        ++packets;

        if (length >= 4 && strncmp(data, "quit",4) == 0)
        {
            std::cout << "detected quit request\n";
            return E_QUIT;
        }

        if (!validatePacket(data, length))
        {
            ++bad_packets;
            return E_BAD_PACKET;
        }

        return E_PACKET;
    }

    asio::io_service& io_service_;
    udp::socket socket_;
    udp::endpoint sender_endpoint_;
    enum { max_length = 1468 };
    char data_[max_length];
    size_t bytes;
    size_t packets;
    size_t bad_packets;

};

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 2)
        {
            std::cerr << "Usage: async_udp_server <port>\n";
            return 1;
        }

        asio::io_service io_service;

        using namespace std; // For atoi.
        server s(io_service, atoi(argv[1]));

        io_service.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}