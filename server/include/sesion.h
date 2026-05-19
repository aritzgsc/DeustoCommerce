#ifndef SESION_H_
#define SESION_H_

#include <string>
#include <winsock2.h>
#include <openssl/ssl.h>

class Sesion {
private:

	SSL* ssl;
	bool autenticado;
	std::string correo;
	std::string nombre;
	std::string apellido;

public:

	Sesion(SSL* ssl);
	~Sesion();

	// Getters

	SSL* getSSL() const;
	bool isAutenticado() const;
	const std::string& getCorreo() const;
	const std::string& getNombre() const;
	const std::string& getApellido() const;

	// Settes y lógica

	void login(const std::string& correo, const std::string& nombre, const std::string& apellido);
	void logout();

	// Enviar datos al cliente
	void enviar(const std::string& mensaje) const;

};

#endif /* SESION_H_ */
