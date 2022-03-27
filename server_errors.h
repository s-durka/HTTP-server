#ifndef SERVER_ERRORS_H
#define SERVER_ERRORS_H

struct error302 : public std::exception {
    [[nodiscard]] const char * what () const throw () {
        return "error 302 - zasób tymczasowo przeniesiony";
    }
};
struct error400 : public std::exception {
    [[nodiscard]] const char * what () const throw () {
        return "error 400 - niepoprawny format";
    }
};
struct error404 : public std::exception {
    [[nodiscard]] const char * what () const throw () {
        return "error 404 - zasób nie został odnaleziony";
    }
};
struct error500 : public std::exception {
    [[nodiscard]] const char * what () throw () {
        return "error 500 - błąd po stronie serwera";
    }
};
struct error501 : public std::exception {
    [[nodiscard]] const char * what () throw () {
        return "error 501 - żądanie poza zakresem możliwości serwera";
    }
};

struct crlf : public std::exception {
    [[nodiscard]] const char * what () const throw () {
        return "CR read";
    }
};

#endif //SERVER_ERRORS_H
