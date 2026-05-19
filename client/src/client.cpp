#include "client.h"
#include <sstream>

extern "C" {
	#include "log.h"
}

using namespace std;

#define BUFFER_SIZE 8192

// Auxiliar

int split(const string& s, char delimiter, string args[], int maxArgs) {

	int count = 0;
    string token;
    istringstream tokenStream(s);

    while (getline(tokenStream, token, delimiter) && count < maxArgs) {
        args[count] = token;
        count++;
    }

    return count;

}

// Clase cliente

Client::Client(string ip, int puerto) : sock(INVALID_SOCKET), ctx(nullptr), ssl(nullptr), ip(ip), puerto(puerto) {

	autenticado = false;
	correo = "";
	nombre = "";
	apellido = "";

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SSL_library_init();
	SSL_load_error_strings();
	OpenSSL_add_all_algorithms();

	const SSL_METHOD* method = TLS_client_method();
	ctx = SSL_CTX_new(method);

}

Client::~Client() {

    desconectar();
    if (ctx) SSL_CTX_free(ctx);
    WSACleanup();

}

bool Client::conectar() {

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) return false;

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(puerto);
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(sock);
        return false;
    }

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);

    if (SSL_connect(ssl) <= 0) {
        LOG_FATAL("Error en el Handshake TLS");
        return false;
    }

    return true;
}

void Client::desconectar() {

    if (ssl) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl = nullptr;
    }

    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }

}

bool Client::enviar(string mensaje) {

    if (!ssl) return false;
    int res = SSL_write(ssl, mensaje.c_str(), (int)mensaje.length());
    return res > 0;

}

string Client::recibir() {

    if (!ssl) return "";
    char buffer[BUFFER_SIZE];
    ZeroMemory(buffer, BUFFER_SIZE);
    int bytes = SSL_read(ssl, buffer, sizeof(buffer) - 1);

    if (bytes > 0) return string(buffer, bytes);
    return "";

}

// Getters

bool Client::isAutenticado() const { return autenticado; }
string Client::getCorreo() const { return correo; }
string Client::getNombre() const { return nombre; }
string Client::getApellido() const { return apellido; }

// Setters lógicos

void Client::login(const string& correo, const string& nombre, const string& apellido) {

    autenticado = true;
    this->correo = correo;
    this->nombre = nombre;
    this->apellido = apellido;

}

void Client::logout() {

    autenticado = false;
    correo = "";
    nombre = "";
    apellido = "";

}
