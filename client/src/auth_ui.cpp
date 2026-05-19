#include <auth_ui.h>
#include "client.h"
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>

extern "C" {
	#include "log.h"
	#include "config.h"
	#include <utils_ui.h>
}

using namespace std;

// AUXILIARES TOKENS

static string getTokenFile() {

	char tokenFile[256];
	if (!configGet(CONFIG_PATH, "TOKEN_PATH", tokenFile, sizeof(tokenFile))) LOG_ERROR("No se pudo obtener TOKEN_PATH de la configuracion.");
	return string(tokenFile);

}

static void guardarTokenLocal(const string& token) {

	ofstream file(getTokenFile());
	if (file.is_open()) {
		file << token;
		file.close();
        LOG_INFO("Token de sesion guardado localmente.");
	} else {
        LOG_ERROR("No se pudo abrir el archivo de tokens para escribir.");
    }

}

static void borrarTokenLocal() {

    string path = getTokenFile();
	if (remove(path.c_str()) == 0) LOG_INFO("Token local eliminado correctamente.");

}

// AUTOLOGIN

#define AUTOLOGIN_RES_PARTES 5

void intentarAutoLogin(Client& cliente) {

	ifstream file(getTokenFile());
	string token;

	if (file.is_open() && getline(file, token) && !token.empty()) {

		file.close();
        LOG_INFO("Token encontrado. Intentando AutoLogin...");

		cliente.enviar("AUTOLOGIN|" + token);
		string respuesta = cliente.recibir();

		if (respuesta.find("OK|") == 0) {

			string partes[AUTOLOGIN_RES_PARTES];
            int numPartes = split(respuesta, '|', partes, AUTOLOGIN_RES_PARTES);

            if (numPartes >= AUTOLOGIN_RES_PARTES) {

            	string correo = partes[1];
            	string nombre = partes[2];
            	string apellido = partes[3];
            	string nuevoToken = partes[4];

                cliente.login(correo, nombre, apellido);

				if (!nuevoToken.empty() > 0)  guardarTokenLocal(nuevoToken.c_str());

				LOG_INFO("AutoLogin exitoso para: %s", correo.c_str());

            } else {

                LOG_ERROR("Error en el formato de respuesta del servidor.");
    			borrarTokenLocal();

    		}

		} else {

			string mensajeError = "El token expiró o es inválido.";
			if (respuesta.find("ERR|") == 0) mensajeError = respuesta.substr(4);

            LOG_WARN("AutoLogin rechazado por el servidor: %s", mensajeError.c_str());
			borrarTokenLocal();

		}

	} else {

        LOG_INFO("No se encontro token previo para AutoLogin.");

    }
}

// LOGIN

#define LOGIN_RES_PARTES 5

void pantallaLogin(Client& cliente) {

	imprimirCabecera((char*)"INICIO DE SESIÓN", (char*)"Introduce tus credenciales para acceder");

	char correo[256] = {0};
	char password[256] = {0};

	leerTexto((char*)"Correo electrónico:", correo, sizeof(correo));
	if (strlen(correo) == 0) {

		imprimirError((char*)"El correo no puede estar vacio.");
		pausar();
		return;

	}

	leerContrasena((char*)"Contraseña:", password, sizeof(password));
	if (strlen(password) == 0) {

		imprimirError((char*)"La contraseña no puede estar vacia.");
	    pausar();
	    return;

	}

    LOG_INFO("Intento de Login para: %s", correo);

	string comando = string("LOGIN|") + correo + "|" + password;
	cliente.enviar(comando);

	string respuesta = cliente.recibir();

	if (respuesta.find("OK|") == 0) {

		string partes[LOGIN_RES_PARTES];
		int numPartes = split(respuesta, '|', partes, LOGIN_RES_PARTES);

		if (numPartes >= LOGIN_RES_PARTES) {

			string nombre = partes[2];
			string apellido = partes[3];
			string token = partes[4];

		    cliente.login(correo, nombre, apellido);

			if (!token.empty())  guardarTokenLocal(token.c_str());

			char msg[128];
			snprintf(msg, sizeof(msg), "Bienvenid@ de nuevo, %s.", nombre.c_str());
			imprimirExito(msg);
			LOG_INFO("Login exitoso: %s %s (%s)", nombre.c_str(), apellido.c_str(), correo);
			pausar();

		} else {

			LOG_ERROR("Error en el formato de respuesta del servidor.");
			borrarTokenLocal();

		}

	} else {

		string mensajeError = "Credenciales incorrectas.";
		if (respuesta.find("ERR|") == 0) mensajeError = respuesta.substr(4);

		imprimirError((char*) mensajeError.c_str());
        LOG_WARN("Fallo de login manual para %s: %s", correo, mensajeError.c_str());
		pausar();

	}
}

// REGISTRO

#define REGISTRO_RES_PARTES 5

void pantallaRegistro(Client& cliente) {

    imprimirCabecera((char*)"REGISTRO DE CLIENTE", (char*)"Crea una cuenta en DeustoCommerce");

    char nombre[64] = {0};
    char apellido[64] = {0};
    char correo[256] = {0};
    char password[256] = {0};

    leerTexto((char*)"Nombre:", nombre, sizeof(nombre));
    if (strlen(nombre) == 0) return;

    leerTexto((char*)"Apellido:", apellido, sizeof(apellido));
    if (strlen(apellido) == 0) return;

    leerTexto((char*)"Correo electronico:", correo, sizeof(correo));
    if (strlen(correo) == 0) return;

    leerContrasena((char*)"Contraseña (min. 6 caracteres):", password, sizeof(password));
    if (strlen(password) < 6) {

        imprimirError((char*)"La contraseña es demasiado corta.");
        pausar();
        return;

    }

    imprimirSeccion((char*)"CONFIRMACIÓN DE DATOS");
    cout << "  " << ESTILO_SUBTITULO << "Correo:      " << RESET << C_BLANCO << correo << "\n" << RESET;
    cout << "  " << ESTILO_SUBTITULO << "Nombre:      " << RESET << C_BLANCO << nombre << " " << apellido << "\n" << RESET;
    cout << "\n";

    if (confirmar((char*)"¿Crear cuenta con estos datos?")) {

        LOG_INFO("Iniciando peticion de registro para: %s", correo);

        string comando = string("REGISTRO|") + correo + "|" + nombre + "|" + apellido + "|" + password;
        cliente.enviar(comando);

        string respuesta = cliente.recibir();

        if (respuesta == "WAIT") {

            char codigo[16] = {0};
            imprimirInfo((char*)"¡Te hemos enviado un correo de verificacion!");
            cout << "\n";

            leerTexto((char*)"Introduce el codigo:", codigo, sizeof(codigo));

            cliente.enviar(codigo);

            respuesta = cliente.recibir();

        }

        if (respuesta.find("OK|") == 0) {

        	string partes[REGISTRO_RES_PARTES];
        	int numPartes = split(respuesta, '|', partes, REGISTRO_RES_PARTES);

        	if (numPartes >= REGISTRO_RES_PARTES) {

        		string token = partes[4];

				cliente.login(correo, nombre, apellido);
				if (!token.empty()) guardarTokenLocal(token);

				imprimirExito((char*)"¡Cuenta creada con exito! Sesion iniciada.");
				LOG_INFO("Usuario registrado y logueado con exito: %s", correo);
				pausar();

        	}

        } else {

            string mensajeError = "No se pudo completar el registro.";
            if (respuesta.find("ERR|") == 0) mensajeError = respuesta.substr(4);

            imprimirError((char*) mensajeError.c_str());
            LOG_WARN("Fallo en registro para %s: %s", correo, mensajeError.c_str());
            pausar();

        }

    } else {

    	imprimirWarn((char*)"Registro cancelado.");
        LOG_INFO("Registro cancelado por el usuario: %s", correo);
        pausar();

    }
}

// LOGOUT

void efectuarLogout(Client& cliente) {

	if (confirmar((char*)"¿Seguro que quieres cerrar la sesion actual?")) {

        string usuarioLogout = cliente.getCorreo();
		cliente.enviar("LOGOUT");

		string respuesta = cliente.recibir();

		if (respuesta.find("OK") != string::npos) {

			cliente.logout();
			borrarTokenLocal();

			imprimirExito((char*)"Sesion cerrada correctamente. ¡Hasta pronto!");
            LOG_INFO("Logout completado para el usuario: %s", usuarioLogout.c_str());
			pausar();

		} else {

			string mensajeError = "Error al cerrar sesión.";
			if (respuesta.find("ERR|") == 0) mensajeError = respuesta.substr(4);

			imprimirError((char*) mensajeError.c_str());
            LOG_ERROR("Fallo en la peticion de logout para %s: %s", usuarioLogout.c_str(), mensajeError.c_str());
			pausar();

		}
	}
}
