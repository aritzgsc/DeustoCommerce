#include "sesion.h"

using namespace std;

Sesion::Sesion(SSL* ssl) {

    this->ssl = ssl;
    this->autenticado = false;
    this->correo = "";
    this->nombre = "";
    this->apellido = "";

}

Sesion::~Sesion() { logout(); }

// Getters

SSL* Sesion::getSSL() const { return ssl; }
bool Sesion::isAutenticado() const { return autenticado; }
const std::string& Sesion::getCorreo() const { return correo; }
const std::string& Sesion::getNombre() const { return nombre; }
const std::string& Sesion::getApellido() const { return apellido; }

// Setters y lógica

void Sesion::login(const string& correo, const string& nombre, const string& apellido) {

    autenticado = true;
    this->correo = correo;
    this->nombre = nombre;
    this->apellido = apellido;

}

void Sesion::logout() {

    autenticado = false;
    correo = "";
    nombre = "";
    apellido = "";

}
