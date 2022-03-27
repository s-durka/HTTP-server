#ifndef BUFFERHTTP_H
#define BUFFERHTTP_H


#include <iosfwd>
#include <cstdio>
#include <sstream>
#include "server_errors.h"
#include "definitions.h"
#include "SocketTCP.h"

class BufferHTTP {
public:
    BufferHTTP() : http_buffer_ss(std::string()) {}

    void clear_buffer() {
        http_buffer_ss.str(std::string());
        http_buffer_ss.clear();
    }

    // funkcja pozwalająca pobierać komunikaty wygodnie na poziomie HTTP, a nie TCP
    // @throws msg_too_long, std::system_error z soket_read()
    std::string get_line(SocketTCP & msg_socket, char* tcp_buffer) {
        std::stringstream prefix;
        prefix.str("");
        std::string read = std::string(); // jest nadpisywany kolejnymi wynikami getline()
        http_buffer_ss.clear();

        getline(http_buffer_ss, read, '\n');
        prefix << read; // dodajemy do prefiksu wynik ostatniego getline, który nie skończył się na "\n"
        while (!http_buffer_ss.good()) {
            http_buffer_ss.clear();
            http_buffer_ss.str(""); // czyścimy bo już wszystko z niego przeczytaliśmy

            ssize_t len = msg_socket.socket_read(tcp_buffer, BUFFER_SIZE); // BUFFER_SIZE jest o jeden mniejsze niż faktycznyu rozmiar bufora TCP
            tcp_buffer[len] = '\0';
            http_buffer_ss << tcp_buffer;
            getline(http_buffer_ss, read, '\n');
            prefix << read; // dodajemy do prefiksu wynik ostatniego getline, który nie skończył się na "\n"
            prefix.seekg(0, std::ios::end);
            long size = prefix.tellg();
            if (size > MAX_STR_LEN)
                throw error400();
        }

        // jeśli ostatni getline() skończył się '\n' to wyjdzie z pętli
        return prefix.str();
    }
private:
    std::stringstream http_buffer_ss;
};


#endif //BUFFERHTTP_H
