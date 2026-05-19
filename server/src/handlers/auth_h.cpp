#include "auth_h.h"
#include "protocolo.h"
#include "mail_utils.h"
#include <iostream>
#include <iomanip>

extern "C" {
	#include "sodium.h"
	#include "auth_db.h"
	#include "log.h"
}

using namespace std;

// AUXILIARES

string generarToken() {

	unsigned char token[32];
	randombytes_buf(token, sizeof(token));

	stringstream ss;
	for (int i = 0 ; i < 32 ; i++) ss << hex << setw(2) << setfill('0') << (int)token[i];

	return ss.str();

}

// AUTH HANDLERS

void handleAutoLogin(SSL* ssl, sqlite3* db, string args[], Sesion& sesion) {

	string tokenRecibido = args[1];
	Usuario u;

	int encontrado = getUsuarioPorToken(db, tokenRecibido.c_str(), &u);

	if (encontrado == 1) {

		string nuevoToken = generarToken();

		if (actualizarToken(db, u.correo, nuevoToken.c_str())) {

			string res = "OK|" + string(u.correo) + "|" + string(u.nombre) + "|" + string(u.apellido) + "|" + nuevoToken;
			responder(ssl, res);

			sesion.login(string(u.correo), string(u.nombre), string(u.apellido));
			LOG_INFO("AutoLogin exitoso para usuario: %s", u.correo);

		} else {

			string err = "ERR|Error al renovar sesión en el servidor.";
			responder(ssl, err);
			LOG_ERROR("Fallo al actualizar token en BD para %s", u.correo);

		}

	} else {

		string err;

		if (encontrado == 2) {
            err = "ERR|Sesión expirada.";
            LOG_WARN("AutoLogin fallido por sesión expirada");
        } else {
            err = "ERR|Sesión inválida.";
            LOG_WARN("AutoLogin fallido por token inválido");
        }

		responder(ssl, err);

	}

}

void handleLogin(SSL* ssl, sqlite3* db, string args[], Sesion& sesion) {

	string correo = args[1];
	string passPlana = args[2];
	Usuario u;

	if (getUsuarioPorCorreo(db, correo.c_str(), &u)) {

		if (crypto_pwhash_str_verify(u.contrasenaHash, passPlana.c_str(), passPlana.length()) == 0) {

			string token = generarToken();

			if (actualizarToken(db, correo.c_str(), token.c_str())) {

				string res = "OK|" + correo + "|" + string(u.nombre) + "|" + string(u.apellido) + "|" + token;
				responder(ssl, res);

				sesion.login(correo, string(u.nombre), string(u.apellido));
				LOG_INFO("Login exitoso para usuario: %s", correo.c_str());

			} else {

				string err = "ERR|Error de BD al asignar token.";
				responder(ssl, err);
				LOG_ERROR("Error de BD al guardar token para %s", correo.c_str());

			}

		} else {

			string err = "ERR|Credenciales incorrectas.";
			responder(ssl, err);
			LOG_WARN("Credenciales incorrectas para %s", correo.c_str());

		}

	} else {

		string err = "ERR|El correo no está registrado.";
		responder(ssl, err);
		LOG_WARN("Intento de acceso a cuenta no registrada (%s)", correo.c_str());

	}

}

void handleRegistro(SSL* ssl, sqlite3* db, string args[], Sesion& sesion) {

	string correo = args[1];
    string nombre = args[2];
    string apellido = args[3];
    string passPlana = args[4];

    if (existeUsuario(db, correo.c_str())) {

        string err = "ERR|El correo ya esta en uso.";
        responder(ssl, err);
        LOG_WARN("Intento de registro con correo ya existente (%s)", correo.c_str());
        return;

    }

    uint32_t codigo = randombytes_uniform(1000000);

    char codigoGen[7];
    snprintf(codigoGen, sizeof(codigoGen), "%06u", codigo);

    LOG_INFO("Enviando correo de verificacion a %s (Codigo: %s)", correo.c_str(), codigoGen);

    enviarMailVerificacion(correo.c_str(), codigoGen);

    responder(ssl, "WAIT");

    char buffer[256] = {0};
    int bytesLeidos = SSL_read(ssl, buffer, sizeof(buffer) - 1);

    if (bytesLeidos <= 0) {
        LOG_WARN("El cliente se desconecto esperando la verificacion de registro (%s)", correo.c_str());
        return;
    }

    string codigoIngresado(buffer);

    codigoIngresado.erase(codigoIngresado.find_last_not_of(" \n\r\t") + 1);

    if (codigoIngresado != string(codigoGen)) {

        string err = "ERR|Codigo de verificacion incorrecto.";
        responder(ssl, err);
        LOG_WARN("Codigo de verificación incorrecto para %s. Esperado: %s, Recibido: %s", correo.c_str(), codigoGen, codigoIngresado.c_str());
        return;

    }

    char hash[crypto_pwhash_STRBYTES];

    if (crypto_pwhash_str(hash, passPlana.c_str(), passPlana.length(), crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {

    	string err = "ERR|Error interno al procesar seguridad.";
    	responder(ssl, err);
    	LOG_ERROR("Fallo de libsodium al generar hash para %s", correo.c_str());
    	return;

    }

    if (crearUsuario(db, correo.c_str(), nombre.c_str(), apellido.c_str(), hash)) {

    	LOG_INFO("Nuevo usuario registrado con éxito: %s", correo.c_str());
        string argsLogin[3] = {"LOGIN", correo, passPlana};
        handleLogin(ssl, db, argsLogin, sesion);

    } else {

    	string err = "ERR|No se pudo completar el registro en la BD.";
    	responder(ssl, err);
    	LOG_ERROR("Error de BD al insertar usuario %s", correo.c_str());
    	return;

    }

}

void handleLogout(SSL* ssl, sqlite3* db, Sesion& sesion) {

	if (invalidarToken(db, sesion.getCorreo().c_str())) {

		string res = "OK";
		responder(ssl, res);
		LOG_INFO("Logout exitoso para usuario: %s", sesion.getCorreo().c_str());
		sesion.logout();

	} else {

		string err = "ERR|Error al invalidar token en servidor.";
		responder(ssl, err);
		LOG_ERROR("Error de BD al invalidar token para %s", sesion.getCorreo().c_str());

	}

}
