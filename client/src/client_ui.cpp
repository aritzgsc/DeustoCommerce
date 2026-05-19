#include <auth_ui.h>
#include <catalogo_ui.h>
#include <client_ui.h>
#include <iostream>
#include <iomanip>
#include <string>

extern "C" {
	#include <utils_ui.h>
	#include "log.h"
}

using namespace std;

bool salir = false;

// AUXILIARES

static void imprimirEstadoCliente(Client& cliente) {

    imprimirSeccion((char*)"RESUMEN DE ACTIVIDAD");

    time_t ahora = time(nullptr);
    struct tm* tm = localtime(&ahora);
    char fechaStr[32];
    strftime(fechaStr, sizeof(fechaStr), "%d/%m/%Y  %H:%M", tm);

    cout << "  " << C_GRIS << left << setw(24) << "Fecha y hora:" << RESET ESTILO_HINT << "  " << fechaStr << "\n" << RESET;

    if (cliente.isAutenticado()) {

        cliente.enviar("GET_ESTADO_CLIENTE");
        string respuesta = cliente.recibir();

        int itemsCarrito = 0;
        int itemsFavoritos = 0;
        int pedidosCurso = 0;

        if (respuesta.find("OK|") == 0) {

            sscanf(respuesta.c_str(), "OK|%d|%d|%d", &itemsCarrito, &itemsFavoritos, &pedidosCurso);

            // Usuario actual
            cout << "  " << C_GRIS << left << setw(24) << "Conectado como:" << RESET C_CYAN << "  " << cliente.getNombre() << " " << cliente.getApellido() << "\n" << RESET;

            // Carrito
            cout << "  " << C_GRIS << left << setw(24) << "En carrito:" << RESET;
            if (itemsCarrito > 0) cout << C_BLANCO << "  " << itemsCarrito << (itemsCarrito == 1? " artículo\n" : " artículos\n") << RESET;
            else cout << C_GRIS << "  Vacío\n" << RESET;

            // Favoritos
            cout << "  " << C_GRIS << left << setw(24) << "Favoritos guardados:" << RESET C_BLANCO << "  " << itemsFavoritos << " productos\n" << RESET;

            // Pedidos pendientes
            cout << "  " << C_GRIS << left << setw(24) << "Pedidos en curso:" << RESET;
            if (pedidosCurso > 0) cout << ESTILO_WARN << "  " << pedidosCurso << " en preparación\n" << RESET;
            else cout << ESTILO_EXITO << "  Sin pedidos pendientes\n" << RESET;

        } else {

        	string mensajeError = "Error al conseguir información del cliente.";
        	if (respuesta.find("ERR|") == 0) mensajeError = respuesta.substr(4);

        	imprimirError((char*) mensajeError.c_str());
        	pausar();

        	LOG_WARN("No se pudo obtener el estado del cliente %s: %s", cliente.getCorreo().c_str(), mensajeError.c_str());

        }

    } else {

        // Estado para invitados
        cout << "  " << C_GRIS << left << setw(24) << "Estado de cuenta:" << RESET  C_AMARILLO << "  Invitado\n" << RESET;
        cout << "  " << C_GRIS << left << setw(24) << "Aviso:" << RESET C_GRIS << "  Inicia sesión para ver tu carrito y guardar favoritos.\n" << RESET;

    }

}

// SELECCIONAR DIRECCIÓN

int seleccionarDireccion(Client& cliente) {

    cliente.enviar("GET_DIRECCIONES");
    string respuesta = cliente.recibir();

    if (respuesta.find("OK|") == 0) {

        size_t posPipe = respuesta.find('|', 3);

        if (posPipe != string::npos) {

            int nDirs = 0;

            try {

                nDirs = stoi(respuesta.substr(3, posPipe - 3));

            } catch (...) {}

            if (nDirs == 0) {

                imprimirWarn((char*)"No tienes direcciones guardadas.");
                pausar();
                return -1;

            }

            int selDir;

            if (nDirs == 1) selDir = 1;
            else {

				string textoFormateado = respuesta.substr(posPipe + 1);
				imprimirSeccion((char*)"TUS DIRECCIONES");
				cout << textoFormateado;

				selDir = leerEntero((char*)"Numero de direccion:", 1, nDirs);

            }

            LOG_INFO("Usuario %s seleccionó dirección guardada con índice: %d", cliente.getCorreo().c_str(), selDir);
            return selDir;

        }

    } else {

    	string mensajeError = "No se pudieron cargar las direcciones.";
    	if (respuesta.find("ERR|") == 0) mensajeError = respuesta.substr(4);

    	imprimirError((char*) mensajeError.c_str());
    	pausar();

    	LOG_ERROR("Error al obtener direcciones de %s: %s", cliente.getCorreo().c_str(), mensajeError.c_str());

    }

    return -1;
}

// HOME

static char* cmdsHomeNoLogueado[] = {

		(char*)"LOGIN", (char*)"REGISTRO", (char*)"CATALOGO", (char*)"BUSCAR", (char*)"EXIT"

};

#define N_CMDS_HOME_NO_LOGUEADO 5

static char* cmdsHomeLogueado[] = {

		(char*)"CATALOGO", (char*)"BUSCAR", (char*)"CARRITO", (char*)"FAVORITOS", (char*)"PEDIDOS", (char*)"LOGOUT", (char*)"EXIT"

};

#define N_CMDS_HOME_LOGUEADO 7

void pantallaHome(Client& cliente) {

    limpiarPantalla();

    // Logo completo en el HOME

    printf("\n");
    printf(ESTILO_TITULO
           "  ╔══════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n"
           "  ║                                                                                                              ║\n"
           "  ║" RESET C_BLANCO NEGRITA
              "                                         DEUSTO" RESET C_CYAN NEGRITA
			                                                 "COMMERCE" RESET C_BLANCO NEGRITA
                                                                     "  —  Tu tienda                                         " RESET ESTILO_TITULO
								                                                                                            "║\n"
           "  ║                                                                                                              ║\n"
           "  ╚══════════════════════════════════════════════════════════════════════════════════════════════════════════════╝\n"
           RESET);

    imprimirEstadoCliente(cliente);

    imprimirSeccion((char*)"COMANDOS DISPONIBLES");

    if (!cliente.isAutenticado()) {

        cout << "  " << ESTILO_CMD << "LOGIN     " << RESET << "  Acceder con tu correo y contraseña\n";
        cout << "  " << ESTILO_CMD << "REGISTRO  " << RESET << "  Crear una cuenta nueva de cliente\n";
        cout << "  " << ESTILO_CMD << "CATALOGO  " << RESET << "  Explorar productos (Modo invitado)\n";
        cout << "  " << ESTILO_CMD << "BUSCAR    " << RESET << "  Búsqueda filtrada de productos\n";
        cout << "  " << ESTILO_CMD << "EXIT      " << RESET << "  Salir de la aplicación\n\n";

    } else {

        cout << "  " << ESTILO_CMD << "CATALOGO  " << RESET << "  Explorar todos los productos disponibles\n";
        cout << "  " << ESTILO_CMD << "BUSCAR    " << RESET << "  Búsqueda filtrada de productos\n";
        cout << "  " << ESTILO_CMD << "CARRITO   " << RESET << "  Ver y gestionar tu cesta de la compra\n";
        cout << "  " << ESTILO_CMD << "FAVORITOS " << RESET << "  Acceder a tu lista de deseos\n";
        cout << "  " << ESTILO_CMD << "PEDIDOS   " << RESET << "  Consultar el historial y estado de pedidos\n";
        cout << "  " << ESTILO_CMD << "LOGOUT    " << RESET << "  Cerrar la sesión actual de forma segura\n";
        cout << "  " << ESTILO_CMD << "EXIT      " << RESET << "  Salir de la aplicación\n\n";

    }

}

// CARRITO

static char* cmdsCarrito[] = {

    (char*)"PAGAR", (char*)"VACIAR", (char*)"HOME", (char*)"EXIT"

};

#define N_CMDS_CARRITO 4

void pantallaCarrito(Client& cliente) {

    LOG_INFO("Usuario %s entro en la pantalla del carrito", cliente.isAutenticado() ? cliente.getCorreo().c_str() : "Invitado");

    while (!salir) {

        cliente.enviar("GET_CARRITO");
        string respuesta = cliente.recibir();

        string tablaFormateada = "";
        bool carritoVacio = true;

        if (respuesta.find("OK|") == 0) {

            size_t posPipe = respuesta.find('|', 3);

            if (posPipe != string::npos) {

                try {

                    int numItems = stoi(respuesta.substr(3, posPipe - 3));
                    carritoVacio = (numItems == 0);

                } catch (...) { }

                tablaFormateada = respuesta.substr(posPipe + 1);

            }

        } else {

            string error = "Error al obtener el carrito.";
            if (respuesta.find("ERR|") == 0) error = respuesta.substr(4);
            tablaFormateada = "  " + string(C_AMARILLO) + error + string(RESET) + "\n";
            LOG_ERROR("Fallo al obtener carrito de %s: %s", cliente.getCorreo().c_str(), error.c_str());

        }

        imprimirCabecera((char*)"TU CARRITO DE LA COMPRA", (char*)"Revisa tus productos antes de pagar");

        if (carritoVacio) {

        	cout << "\033[A";
            imprimirWarn((char*)"Tu carrito esta vacio. !Añade algun producto!");
            pausar();
            return;

        } else {

            cout << tablaFormateada;

        }

        imprimirSeccion((char*)"COMANDOS");

        cout << "  " << ESTILO_CMD << "PAGAR                  " << RESET << "Tramitar el pedido\n";
        cout << "  " << ESTILO_CMD << "VACIAR                 " << RESET << "Eliminar todos los productos\n";
        cout << "  " << ESTILO_CMD << "HOME                   " << RESET << "Volver al menu\n\n";

        Entrada e = leerComando(cmdsCarrito, N_CMDS_CARRITO, (char*)">");

        if (strcmp(e.comando, "PAGAR") == 0 && !carritoVacio) {

            if (pantallaPagarCarrito(cliente)) return; // Si el pago fue un éxito, salimos al HOME

        } else if (strcmp(e.comando, "VACIAR") == 0 && !carritoVacio) {

            LOG_INFO("Usuario %s solicito vaciar el carrito", cliente.getCorreo().c_str());
            cliente.enviar((char*)"VACIAR_CARRITO");
            string res = cliente.recibir();

            if (res.find("OK") == 0) {

                imprimirExito((char*)"Carrito vaciado.");
                LOG_INFO("Carrito de %s vaciado correctamente", cliente.getCorreo().c_str());

            } else {

            	string mensajeError = "No se pudo vaciar el carrito.";
            	if (res.find("ERR|") == 0) mensajeError = res.substr(4);

            	imprimirError((char*) mensajeError.c_str());
            	LOG_ERROR("Error al vaciar carrito de %s: %s", cliente.getCorreo().c_str(), mensajeError.c_str());
            	pausar();

            }

            pausar();

        } else if (strcmp(e.comando, "HOME") == 0) {

            return;

        } else if (strcmp(e.comando, "EXIT") == 0) {

            salir = 1;
            return;

        } else {

            imprimirError((char*)"Comando no reconocido.");
            pausar();

        }

    }

}

// PAGAR CARRITO

static char* cmdsPago[] = {

    (char*)"DIR_GUARDADA", (char*)"DIR_NUEVA", (char*)"CANCELAR"

};

#define N_CMDS_PAGO 3

bool pantallaPagarCarrito(Client& cliente) {

    string datosDir = "";

    LOG_INFO("Usuario %s inicio proceso de pago", cliente.getCorreo().c_str());

    while (!salir) {

        imprimirSeccion((char*)"DIRECCION DE ENVIO");
        cout << "  " << ESTILO_CMD << "DIR_GUARDADA           " << RESET << "Elegir de mis direcciones\n";
        cout << "  " << ESTILO_CMD << "DIR_NUEVA              " << RESET << "Introducir una nueva\n";
        cout << "  " << ESTILO_CMD << "CANCELAR               " << RESET << "Volver al carrito\n\n";

        Entrada eDir = leerComando(cmdsPago, N_CMDS_PAGO, (char*)"Elige opcion de envio:");

        if (strcmp(eDir.comando, "DIR_GUARDADA") == 0) {

            int idxUb = seleccionarDireccion(cliente);
            if (idxUb != -1) {
                datosDir = "GUARDADA|" + to_string(idxUb);
                LOG_INFO("%s seleccionó una direccion guardada", cliente.getCorreo().c_str());
                break;
            }

        } else if (strcmp(eDir.comando, "DIR_NUEVA") == 0) {

            char bufPais[64] = {0};
            char bufCiudad[64] = {0};
            char bufDir[128] = {0};

            leerTexto((char*)"Pais:", bufPais, sizeof(bufPais));
            leerTexto((char*)"Ciudad:", bufCiudad, sizeof(bufCiudad));
            leerTexto((char*)"Direccion:", bufDir, sizeof(bufDir));

            datosDir = "NUEVA|" + string(bufPais) + "|" + string(bufCiudad) + "|" + string(bufDir);
            LOG_INFO("Pago %s: introducida nueva direccion: %s, %s", cliente.getCorreo().c_str(), bufCiudad, bufPais);
            break; // Tenemos la dirección, salimos del bucle para confirmar el pago

        } else if (strcmp(eDir.comando, "CANCELAR") == 0) {

            LOG_INFO("Usuario %s cancelo el proceso de pago", cliente.getCorreo().c_str());
            return false;

        } else {

            imprimirError((char*)"Comando no reconocido.");
            pausar();

        }

    }

    // Si hemos salido del bucle y tenemos datos de dirección, procedemos al pago
    if (!datosDir.empty() && confirmar((char*)"¿Confirmar compra?")) {

        string peticion = "PAGAR_CARRITO|" + datosDir;
        cliente.enviar(peticion);
        string res = cliente.recibir();

        if (res.find("OK") == 0) {

            imprimirExito((char*)"¡Pedido tramitado con exito!");
            LOG_INFO("Pedido tramitado con exito para el usuario: %s", cliente.getCorreo().c_str());
            pausar();
            return true; // Pagado correctamente

        } else {

            string err = "Error al procesar el pago.";
            if (res.find("ERR|") == 0) err = res.substr(4);
            imprimirError((char*)err.c_str());
            LOG_ERROR("Fallo en el pago de %s: %s", cliente.getCorreo().c_str(), err.c_str());
            pausar();
            return false;

        }

    }

    return false;

}

// PEDIDOS

static char* cmdsPedidos[] = {

    (char*)"ANTERIOR", (char*)"SIGUIENTE", (char*)"HOME", (char*)"EXIT"

};

#define N_CMDS_PEDIDOS 4

void pantallaPedidos(Client& cliente) {

    int pagina = 1;
    int totalPags = 1;

    LOG_INFO("Usuario %s consultando historial de pedidos", cliente.getCorreo().c_str());

    while (!salir) {

        string peticion = "GET_PEDIDOS|" + to_string(pagina);
        cliente.enviar(peticion);
        string respuesta = cliente.recibir();

        string tablaFormateada = "";

        if (respuesta.find("OK|") == 0) {

            size_t posPipe = respuesta.find('|', 3);

            if (posPipe != string::npos) {

                try {

                    totalPags = stoi(respuesta.substr(3, posPipe - 3));

                } catch (...) {

                    totalPags = 1;

                }

                tablaFormateada = respuesta.substr(posPipe + 1);

            }

        } else {

            string error = "Error al obtener los pedidos.";
            if (respuesta.find("ERR|") == 0) error = respuesta.substr(4);
            tablaFormateada = "  " + string(C_AMARILLO) + error + string(RESET) + "\n";
            LOG_ERROR("Error al obtener pedidos de %s: %s", cliente.getCorreo().c_str(), error.c_str());

        }

        imprimirCabecera((char*)"HISTORIAL DE PEDIDOS", (char*)"Tus compras anteriores");

        if (tablaFormateada.empty()) {

        	cout << "\033[A";
            imprimirWarn((char*)"Aun no has realizado ningun pedido.");
            pausar();
            return;

        } else {

            cout << tablaFormateada;

        }

        imprimirSeccion((char*)"COMANDOS");
        if (totalPags > 1) cout << "  " << ESTILO_CMD << "ANTERIOR / SIGUIENTE   " << RESET << "Navegar paginas\n";
        cout << "  " << ESTILO_CMD << "HOME                   " << RESET << "Volver al menu principal\n\n";

        Entrada e = leerComando(cmdsPedidos, N_CMDS_PEDIDOS, (char*)">");

        if (strcmp(e.comando, "SIGUIENTE") == 0) {

            if (pagina < totalPags) pagina++;
            else pagina = 1;

        } else if (strcmp(e.comando, "ANTERIOR") == 0) {

            if (pagina > 1) pagina--;
            else pagina = totalPags;

        } else if (strcmp(e.comando, "HOME") == 0) {

            return;

        } else if (strcmp(e.comando, "EXIT") == 0) {

            salir = 1;
            return;

        } else {

            imprimirError((char*)"Comando no reconocido.");
            pausar();

        }
    }
}

// BUCLE PRINCIPAL

void bucleCliente(Client& cliente) {

	activarColores();
	intentarAutoLogin(cliente);

	while (!salir) {

		pantallaHome(cliente);

		if (!cliente.isAutenticado()) {

			Entrada e = leerComando(cmdsHomeNoLogueado, N_CMDS_HOME_NO_LOGUEADO, (char*)">");

			if (strcmp(e.comando, "LOGIN") == 0) {

			    pantallaLogin(cliente);

			} else if (strcmp(e.comando, "REGISTRO") == 0) {

			    pantallaRegistro(cliente);

			} else if (strcmp(e.comando, "CATALOGO") == 0) {

			    pantallaCatalogo(cliente);

			} else if (strcmp(e.comando, "BUSCAR") == 0) {

			    pantallaBuscar(cliente);

			} else if (strcmp(e.comando, "EXIT") == 0) {

			    cliente.enviar("EXIT");
			    salir = true;
			    LOG_INFO("Cliente (Invitado) solicito salir");

			} else {

			    imprimirError((char*)"Comando no reconocido.");
			    pausar();

			}

		} else {

			Entrada e = leerComando(cmdsHomeLogueado, N_CMDS_HOME_LOGUEADO, (char*)">");

			if (strcmp(e.comando, "CATALOGO") == 0) {

				pantallaCatalogo(cliente);

			} else if (strcmp(e.comando, "BUSCAR") == 0) {

			    pantallaBuscar(cliente);

			} else if (strcmp(e.comando, "CARRITO") == 0) {

			    pantallaCarrito(cliente);

			} else if (strcmp(e.comando, "FAVORITOS") == 0) {

			    pantallaFavoritos(cliente);

			} else if (strcmp(e.comando, "PEDIDOS") == 0) {

			    pantallaPedidos(cliente);

			} else if (strcmp(e.comando, "LOGOUT") == 0) {

			    efectuarLogout(cliente);

			} else if (strcmp(e.comando, "EXIT") == 0) {

			    cliente.enviar("EXIT");
			    salir = true;
			    LOG_INFO("Cliente %s solicito salir", cliente.getCorreo().c_str());

			} else {

			    imprimirError((char*)"Comando no reconocido.");
			    pausar();

			}

		}

	}

    limpiarPantalla();
    cout << "\n  " << ESTILO_SUBTITULO << "Cerrando DeustoCommerce...\n\n" << RESET;

}
