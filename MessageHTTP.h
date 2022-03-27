#ifndef MESSAGEHTTP_H
#define MESSAGEHTTP_H


#include <algorithm>
#include <regex>
#include <iostream>
#include "server_errors.h"
#include "definitions.h"


class MessageHTTP {
public:
    MessageHTTP() : connection_flag(false), content_length_flag(false), request_flag(false) {}

    void validate_header(const std::string& str) {
        if (str == "\r")
            throw crlf();
        std::smatch m;
        if (!std::regex_match(str, m, header_field_regex))
            throw error400();
        std::string field_name = m.str(1);
        std::string field_value = m.str(2);
        trim_spaces_suffix(field_value);

        if (strcmp_case_insensitive(field_name, _connection_str)) {
            this->connection_field_value = field_value;
            if (this->connection_flag == true) {
                throw error400();
            }
            this->connection_flag = true;
        }
        else if (strcmp_case_insensitive(field_name, _content_length_str)) {
            if (this->content_length_flag == true)
                throw error400();
            if (field_value != "0")
                throw error400();
            this->content_length_field_value = field_value;
            this->content_length_flag = true;
        }
    }

    void validate_request_line(const std::string& str) {
        if (str == "\r")
            throw crlf();
        std::smatch m;
        if (!std::regex_match(str, m, request_line_regex))
            throw error400();
        std::string method = m.str(1);
        std::string target = m.str(2);
        this->request_method = method;
        this->request_target = target;
        this->request_flag = true;
    }

    /*
     * @return 1 jeśli napotkał znak kończący wiadomość
     * @return 0 jeśli dalej będzie wczytywać wiadomości
     */
    int read_next_line(std::string& next_line) {
        if (request_flag == false) {
            try {
                validate_request_line(next_line);
            } catch (const crlf &e) {
                throw error400();
            }
        } else {
            try {
                validate_header(next_line);
            } catch (const crlf &e) { // koniec wiadomości
                return 1;
            }
        }
        return 0;
    }

    void reset_flags() {
        this->content_length_flag = false;
        this->connection_flag = false;
        this->request_flag = false;
    }

    const std::string &getRequestMethod() const {
        return request_method;
    }
    const std::string &getRequestTarget() const {
        return request_target;
    }
    const std::string &getConnectionFieldValue() const {
        return connection_field_value;
    }
    const std::string &getContentLengthFieldValue() const {
        return content_length_field_value;
    }
    bool getConnectionFlag() const {
        return connection_flag;
    }
    bool getContentLengthFlag() const {
        return content_length_flag;
    }
    bool getRequestFlag() const {
        return request_flag;
    }

    std::string toString() {
        std::string ret = std::string("-------HTTP MESSAGE--------\nrequest - method: " + request_method + ", target: " + request_target + "\n");
        if (connection_flag) {
            ret += "connection - value: " + connection_field_value + "\n";
        }
        if (content_length_flag) {
            ret += "content-length: - value: " + content_length_field_value + "\n";
        }
        return ret;
    }

    static bool strcmp_case_insensitive(const std::string &s1, const std::string& s2) {
        if (s1 == s2)
            return true;
        if (s1.length() != s2.length())
            return false;
        for (std::string::size_type i = 0; i < s1.length(); i++) {
            if (std::tolower(s1[i]) != std::tolower(s2[i]))
                return false;
        }
        return true;
    }

private:
    // usuwa ostatnie spacje ze stringa
    static void trim_spaces_suffix(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    }

    std::string request_method;
    std::string request_target;

    // jedyne obsługiwane nagłówki (reszta nagłówków jest ignorowana):
    std::string connection_field_value;
    std::string content_length_field_value;
    // flagi -- każdy z obsługiwanych nagłówków może być wysłany maksymalnie raz w jednej wiadomości:
    bool connection_flag;
    bool content_length_flag;
    bool request_flag;

    inline static const std::string _connection_str = std::string("Connection");
    inline static const std::string _content_length_str = std::string("Content-length");

    inline static const std::string _method_str = std::string("([!#$%&'\\*\\+\\-\\.\\^_`\\|~\\da-zA-Z]+)");
    //inline static const std::string _request_target_str = std::string("((?:/[a-zA-Z0-9\\.\\-]*)+)");
    inline static const std::string _request_target_str = std::string("(/[^ ]*)");
    inline static std::string _http_version_str = std::string("(HTTP/1.1)");

    inline static std::regex request_line_regex = std::regex(_method_str + " " + _request_target_str + " " + _http_version_str + "\r");

    inline static const std::string _field_name = std::string("([!#$%&'\\*\\+\\-\\.\\^_`\\|~\\da-zA-Z]+):");
    inline static const std::string _ows = std::string("[ ]*");
    inline static const std::string _field_value = std::string("(.*)");

    inline static std::regex header_field_regex = std::regex(_field_name + _ows + _field_value + _ows + "\r");
};



#endif //MESSAGEHTTP_H
