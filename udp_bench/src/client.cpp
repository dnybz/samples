// Based upon
// blocking_udp_echo_client.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>  // memcpy, rand, srand
#include <cstring>  // strlen, strncmp
#include <iostream>
#include <asio.hpp>
#include <thread>
#include <chrono>

using asio::ip::udp;
// using namespace boost::chrono;
// using namespace std; // For strlen etc

template< class Clock >
class timer
{
  typename Clock::time_point start;
public:
  timer() : start( Clock::now() ) {}
  typename Clock::duration elapsed() const
  {
    return Clock::now() - start;
  }
  double seconds() const
  {
    return elapsed().count() * ((double)Clock::period::num/Clock::period::den);
  }
};

enum { max_length = 1468 };

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 5)
        {
            std::cerr << "Usage: udp_client <host> <port> <packets> <rate>\n";
            return 1;
        }

        int pps_multi = 10;
    
        int packets = atoi(argv[3]);
        double rate = static_cast<double>(atoi(argv[4]));

        if (packets <= 0 || rate < pps_multi)
        {
            std::cerr << "Invalid parameters\nUsage: udp_client <host> <port> <packets> <rate>\n";
            return 2;
        }

        int pause_ms = static_cast<int>(0.5 + (1000.0 * pps_multi)/rate);

        if (pause_ms <= 0)
        {
            pps_multi *= 10;
            pause_ms = 1;
        }

        std::cout << "Packets per iteration " << packets << std::endl;

        srand(0);   // crude random number initialisation

        asio::io_service io_service;

        udp::socket s(io_service, udp::endpoint(udp::v4(), 0));

        udp::resolver resolver(io_service);
        udp::resolver::query query(udp::v4(), argv[1], argv[2]);
        udp::resolver::iterator iterator = resolver.resolve(query);

        char request[max_length];
        while (1)
        {
            std::cout << "Enter message: ";
            std::cin.getline(request, max_length);

            if (strncmp(request, "quit",4) == 0)
            {
                s.send_to(asio::buffer(request, 4), *iterator);
                break;
            }

            timer<std::chrono::high_resolution_clock> t;

            for (int i = 0; i < packets; ++i)
            {
                size_t request_length = 1200 + rand() % (max_length - 1200);
                memcpy(request + 4, &request_length, sizeof(uint32_t));
                s.send_to(asio::buffer(request, request_length), *iterator);

                if (i % pps_multi == 0)
                {
                    std::this_thread::sleep_for( std::chrono::milliseconds(pause_ms) );
                }
            }

            double timed = t.seconds();

            std::cout << "Iteration took " << timed << "sec at rate " << static_cast<double>(packets)/timed << "pps\n\n"; 
        }

        char reply[max_length];
        udp::endpoint sender_endpoint;
        size_t reply_length = s.receive_from(
            asio::buffer(reply, max_length), sender_endpoint);
        std::cout << "Reply is:\n";
        std::cout.write(reply, reply_length);

        std::cout << "\nPress a key to exit";
        std::cin.getline(request, max_length);
    }
    catch (std::exception& e)
    {
    std::cerr << "Exception: " << e.what() << "\n";
    }

  return 0;
}