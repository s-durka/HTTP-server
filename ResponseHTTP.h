#ifndef SIK_ZAL1_RESPONSEHTTP_H
#define SIK_ZAL1_RESPONSEHTTP_H


#include <string>
#include <sstream>
#include <vector>
#include <filesystem>
#include <fstream>
#include "SocketTCP.h"
#include "server_errors.h"

class ResponseHTTP {
public:
    ResponseHTTP() :
        status_code(0), reason_phrase(""),
        connection_close(false),
        headers(std::vector<std::string>()),
        send_file_flag(false)
        {};

    std::string create_head() {
        std::stringstream head;
        head << "HTTP/1.1 " << status_code << " " << reason_phrase << "\r\n";
        for (const auto& h : headers) {
            head << h << "\r\n";
        }
        head << "\r\n";
        return head.str();
    }

    void add_header(const std::string& field_name, const std::string& field_value) {
        headers.push_back(field_name + ": " + field_value);
    }

    void send_file_content(const std::filesystem::path& path, char* send_buffer, int buffer_size, SocketTCP& socket) {
        std::fstream file(path);
        if (!file.is_open()) {
            throw error500();
        }
        while(!file.eof()) {
            file.read(send_buffer, buffer_size);
            socket.socket_write(send_buffer, file.gcount());
        }
    }

    bool send_response(SocketTCP& socket, char* send_buffer, int buffer_size) {
        std::string head = create_head();
        char* head_arr = head.data();
        socket.socket_write(head_arr, (ssize_t) head.length());
        if (send_file_flag) {
            send_file_content(this->file_to_send, send_buffer, buffer_size, socket);
        }
        if(connection_close)
            return true;
        return false;
    }

    void setConnectionClose(bool connectionClose) {
        connection_close = connectionClose;
    }
    void setStatusCode(int statusCode) {
        status_code = statusCode;
    }
    void setReasonPhrase(const std::string &reasonPhrase) {
        reason_phrase = reasonPhrase;
    }
    void setSendFileFlag(bool sendFile) {
        send_file_flag = sendFile;
    }
    void setFileToSend(const std::filesystem::path &fileToSend) {
        file_to_send = fileToSend;
    }
private:
    int status_code;
    std::string reason_phrase;
    bool connection_close;
    std::vector<std::string> headers;
    bool send_file_flag;
    std::filesystem::path file_to_send;
};


#endif //SIK_ZAL1_RESPONSEHTTP_H
