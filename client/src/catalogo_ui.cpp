#include <catalogo_ui.h>
#include <client_ui.h>
#include <iostream>

extern "C" {
	#include <utils_ui.h>
    #include "log.h"
}

using namespace std;

// SELECCIONAR CATEGORIA

static char* cmdsCats[] = {

    (char*)"SELECCIONAR", (char*)"ANTERIOR", (char*)"SIGUIENTE", (char*)"BUSCAR", (char*)"VOLVER"

};

#define N_CMDS_CATS 5

int seleccionarCategoria(Client& cliente) {

    int pagina = 1;
    int totalPags = 1;
    string filtro = "";

    while (!salir) {

        string peticion = "GET_CATEGORIAS|" + to_string(pagina) + "|" + filtro;
        cliente.enviar(peticion);
        string respuesta = cliente.recibir();

        string tablaFormateada = "";

        if (respuesta.find("OK|") == 0) {

            size_t p1 = 2;
            size_t p2 = respuesta.find('|', p1 + 1);

            if (p2 != string::npos) {

            	try {
                    totalPags = stoi(respuesta.substr(p1 + 1, p2 - p1 - 1));
                } catch (...) {
                    LOG_ERROR("Error de parseo en totalPags de categorias.");
                    totalPags = 1;
                }

                tablaFormateada = respuesta.substr(p2 + 1);

            }

        } else {

            string error = "Error al obtener categorias.";
            if (respuesta.find("ERR|") == 0) error = respuesta.substr(4);
            tablaFormateada = "  " + string(C_AMARILLO) + error + string(RESET) + "\n";
            LOG_WARN("Servidor rechazo GET_CATEGORIAS: %s", error.c_str());

        }

        imprimirCabecera((char*)"SELECCIONAR CATEGORIA", (char*)"Elige la categoria del producto");

        if (!filtro.empty()) cout << "  " << ESTILO_HINT << "Filtro activo: " << RESET << C_CYAN << "\"" << filtro << "\"\n\n" << RESET;

        if (tablaFormateada.empty() || respuesta.find("ERR|") == 0) {
        	cout << "\033[A";
        	imprimirWarn((char*)"No se encontraron categorias.");
        }
        else cout << tablaFormateada;

        imprimirSeccion((char*)"COMANDOS");
        cout << "  " << ESTILO_CMD << "SELECCIONAR [ID]       " << RESET << "Elegir categoria\n";
        cout << "  " << ESTILO_CMD << "BUSCAR [texto]         " << RESET << "Filtrar por nombre\n";
        if (totalPags > 1) cout << "  " << ESTILO_CMD << "ANTERIOR / SIGUIENTE   " << RESET << "Navegar paginas\n";
        cout << "  " << ESTILO_CMD << "VOLVER                 " << RESET << "Cancelar\n\n";

        Entrada e = leerComando(cmdsCats, N_CMDS_CATS, (char*)">");

        if (strcmp(e.comando, "SELECCIONAR") == 0 && strlen(e.arg1) > 0) {

            int id = atoi(e.arg1);
            LOG_INFO("Categoria seleccionada: %d", id);
            return id;

        } else if (strcmp(e.comando, "BUSCAR") == 0) {

            if (strlen(e.arg1) > 0) filtro = e.arg1;
            else {

                char buf[64] = {0};
                leerTexto((char*)"Texto a buscar:", buf, sizeof(buf));
                filtro = buf;

            }

            LOG_INFO("Filtrando categorias por: '%s'", filtro.c_str());
            pagina = 1;

        } else if (strcmp(e.comando, "SIGUIENTE") == 0) {

            if (pagina < totalPags) pagina++; else pagina = 1;

        } else if (strcmp(e.comando, "ANTERIOR") == 0) {

            if (pagina > 1) pagina--; else pagina = totalPags;

        } else if (strcmp(e.comando, "VOLVER") == 0) {

            return -1;

        } else {

            imprimirError((char*)"Comando no reconocido.");
            pausar();

        }

    }

    return -1;

}

// SELECCIONAR VARIANTE

string seleccionarVariante(Client& cliente, int idProd) {

    cliente.enviar("GET_VARIANTES|" + to_string(idProd));
    string respuesta = cliente.recibir();

    if (respuesta.find("OK|") == 0) {

        size_t posPipe = respuesta.find('|', 3);

        if (posPipe != string::npos) {

        	int nVariantes = 1;

        	try {
                nVariantes = stoi(respuesta.substr(3, posPipe - 3));
            } catch (...) {
                nVariantes = 1;
            }

            if (nVariantes <= 1) return "UNICA";

            string variantes[nVariantes];

            imprimirSeccion((char*)"VARIANTES DISPONIBLES");

            size_t inicio = posPipe + 1;

            for (int i = 0 ; i < nVariantes ; i++) {

            	size_t fin = respuesta.find('|', inicio);
            	string varianteActual;

            	if (fin == string::npos) varianteActual = respuesta.substr(inicio);
            	else varianteActual = respuesta.substr(inicio, fin - inicio);

            	variantes[i] = varianteActual;

            	cout << "  " << ESTILO_ID << to_string(i + 1) << RESET << "  " << varianteActual << endl;

            	inicio = fin + 1;

            }

            cout << endl;

            int sel = leerEntero((char*)"Numero de variante:", 1, nVariantes);
            string varSel = variantes[sel - 1];
            LOG_INFO("Variante seleccionada para prod #%d: %s", idProd, varSel.c_str());
            return varSel;

        }

    }

    LOG_WARN("No se pudieron cargar variantes para prod #%d.", idProd);

    return "";

}

// CATALOGO

static char* cmdsCatalogo[] = {

		(char*)"VER_PROD", (char*)"BUSCAR", (char*)"ANTERIOR", (char*)"SIGUIENTE", (char*)"HOME", (char*)"EXIT"

};

#define N_CMDS_CATALOGO 6

void pantallaCatalogo(Client& cliente) {

	int pagina = 1;
	int totalPags = 1;

	while (!salir) {

		string peticion = "GET_CATALOGO|" + to_string(pagina);
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

			string error = "Error al obtener el catálogo.";
			if (respuesta.find("ERR|") == 0) error = respuesta.substr(4);
			tablaFormateada = "  " + string(C_AMARILLO) + error + string(RESET) + "\n";
            LOG_ERROR("Fallo al cargar catalogo: %s", error.c_str());

		}

        imprimirCabecera((char*)"CATÁLOGO DE PRODUCTOS", (char*)"Explora nuestras ofertas");

		if (tablaFormateada.empty()) {
			cout << "\033[A";
			imprimirWarn((char*)"No hay productos en el catalogo.");
		}
		else cout << tablaFormateada;

		imprimirSeccion((char*)"COMANDOS");
        cout << "  " << ESTILO_CMD << "VER_PROD [ID]          " << RESET << "Ver detalle de producto\n";
        cout << "  " << ESTILO_CMD << "BUSCAR                 " << RESET << "Filtrar productos\n";
        if (totalPags > 1) cout << "  " << ESTILO_CMD << "ANTERIOR / SIGUIENTE   " << RESET << "Navegar paginas\n";
        cout << "  " << ESTILO_CMD << "HOME                   " << RESET << "Volver al menu principal\n\n";

        Entrada e = leerComando(cmdsCatalogo, N_CMDS_CATALOGO, (char*)">");

        if (strcmp(e.comando, "VER_PROD") == 0 && strlen(e.arg1) > 0) {

        	if (pantallaVerProducto(cliente, atoi(e.arg1))) return;

        } else if (strcmp(e.comando, "BUSCAR") == 0) {

            LOG_INFO("Usuario accede a busqueda avanzada.");
            pantallaBuscar(cliente);
            return;

        } else if (strcmp(e.comando, "SIGUIENTE") == 0) {

            if (pagina < totalPags) pagina++; else pagina = 1;

        } else if (strcmp(e.comando, "ANTERIOR") == 0) {

            if (pagina > 1) pagina--; else pagina = totalPags;

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

// BUSCAR

static char* cmdsBuscar[] = {

		(char*)"NOMBRE", (char*)"CATEGORIA", (char*)"PRECIO", (char*)"POR_DEFINIR", (char*)"BORRAR", (char*)"VER_PROD", (char*)"ANTERIOR", (char*)"SIGUIENTE", (char*)"HOME", (char*)"EXIT"

};

#define N_CMDS_BUSCAR 10

void pantallaBuscar(Client& cliente, bool soloFavoritos) {

	string fNombre = "";
	int fIdCat = -1;
	double fPrecioMin = -1;
	double fPrecioMax = -1;

	int pagina = 1;
	int totalPags = 1;

	while (!salir) {

		string peticion = "BUSCAR|" + to_string(pagina) + "|" + fNombre + "|" + to_string(fIdCat) + "|" + to_string(fPrecioMin) + "|" + to_string(fPrecioMax) + "|" + (soloFavoritos? "1" : "0");
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

			string error = "Error al buscar productos.";
			if (respuesta.find("ERR|") == 0) error = respuesta.substr(4);
			tablaFormateada = "  " + string(C_AMARILLO) + error + string(RESET) + "\n";
            LOG_WARN("Error en busqueda avanzada: %s", error.c_str());

		}

		if (soloFavoritos) imprimirCabecera((char*)"MIS FAVORITOS", (char*)"Productos que has guardado");
		else imprimirCabecera((char*)"BUSCAR PRODUCTOS", (char*)"Añade filtros y ejecuta la busqueda");

		imprimirSeccion((char*)"FILTROS ACTIVOS");

		cout << "  " << ESTILO_SUBTITULO << "Nombre:    " << RESET << (fNombre.empty() ? string(C_GRIS) + "—" : C_BLANCO + fNombre) << "\n" << RESET;
		cout << "  " << ESTILO_SUBTITULO << "Categoria: " << RESET << (fIdCat == -1 ? string(C_GRIS) + "—" : C_BLANCO + to_string(fIdCat)) << "\n" << RESET;

		cout << "  " << ESTILO_SUBTITULO << "Precio:    " << RESET;
		if (fPrecioMin >= 0 || fPrecioMax >= 0) printf(C_BLANCO "%.2f — %.2f €\n" RESET, fPrecioMin >= 0 ? fPrecioMin : 0.0, fPrecioMax >= 0 ? fPrecioMax : 99999.0);
		else cout << C_GRIS << "—\n" << RESET;

		imprimirSeccion((char*)"RESULTADOS");
		if (tablaFormateada.empty()) {
			cout << "\033[A";
			imprimirWarn((char*)"No se encontraron productos con esos filtros.");
		}
		else cout << tablaFormateada;

		imprimirSeccion((char*)"COMANDOS");
		cout << "  " << ESTILO_CMD << "VER_PROD [ID]          " << RESET << "Ver detalle de producto\n";
		cout << "  " << ESTILO_CMD << "NOMBRE [texto]         " << RESET << "Filtrar por nombre\n";
		cout << "  " << ESTILO_CMD << "CATEGORIA [id]         " << RESET << "Filtrar por ID de categoria\n";
		cout << "  " << ESTILO_CMD << "PRECIO [min] [max]     " << RESET << "Filtrar por rango de precio\n";
		cout << "  " << ESTILO_CMD << (soloFavoritos? "NO_FAVORITOS           " : "FAVORITOS              ") << RESET << "Filtrar por favoritos\n";
		cout << "  " << ESTILO_CMD << "BORRAR                 " << RESET << "Limpiar filtros\n";
		if (totalPags > 1) cout << "  " << ESTILO_CMD << "ANTERIOR / SIGUIENTE   " << RESET << "Navegar paginas\n";
		cout << "  " << ESTILO_CMD << "HOME                   " << RESET << "Volver al menu principal\n\n";

		cmdsBuscar[3] = (char*)(soloFavoritos ? "NO_FAVORITOS" : "FAVORITOS");

		Entrada e = leerComando(cmdsBuscar, N_CMDS_BUSCAR, (char*)">");

		if (strcmp(e.comando, "NOMBRE") == 0) {

			if (strlen(e.arg1) > 0) fNombre = e.arg1;
			else {

		    	char buf[128] = {0};
		        leerTexto((char*)"Nombre a buscar:", buf, sizeof(buf));
		        fNombre = buf;

		    }

            LOG_INFO("Busqueda: filtro nombre -> '%s'", fNombre.c_str());
			pagina = 1;

		} else if (strcmp(e.comando, "CATEGORIA") == 0) {

		    if (strlen(e.arg1) > 0) fIdCat = atoi(e.arg1);
		    else {

				int idSel = seleccionarCategoria(cliente);
				if (idSel != -1) fIdCat = idSel;

		    }

            LOG_INFO("Busqueda: filtro categoria -> %d", fIdCat);
		    pagina = 1;

		 } else if (strcmp(e.comando, "PRECIO") == 0) {

		    if (strlen(e.arg1) > 0) fPrecioMin = atof(e.arg1);
		    if (strlen(e.arg2) > 0) fPrecioMax = atof(e.arg2);
		    if (strlen(e.arg1) == 0 && strlen(e.arg2) == 0) {

		        fPrecioMin = leerDouble((char*)"Precio minimo (0 = sin limite):", 0, 999999);
		        fPrecioMax = leerDouble((char*)"Precio maximo (0 = sin limite):", 0, 999999);
		        if (fPrecioMin == 0) fPrecioMin = -1;
		        if (fPrecioMax == 0) fPrecioMax = -1;

		    }

            LOG_INFO("Busqueda: filtro precio -> [%.2f - %.2f]", fPrecioMin, fPrecioMax);
		    pagina = 1;

		} else if (strcmp(e.comando, "FAVORITOS") == 0 || strcmp(e.comando, "NO_FAVORITOS") == 0) {

			soloFavoritos = !soloFavoritos;
            LOG_INFO("Busqueda: soloFavoritos -> %d", soloFavoritos);
			pagina = 1;

		} else if (strcmp(e.comando, "BORRAR") == 0) {

		    fNombre = ""; fIdCat = -1; fPrecioMin = -1.0; fPrecioMax = -1.0; pagina = 1;
		    imprimirExito((char*)"Filtros eliminados.");
            LOG_INFO("Filtros de busqueda reseteados.");

		} else if (strcmp(e.comando, "VER_PROD") == 0 && strlen(e.arg1) > 0) {

		    if(pantallaVerProducto(cliente, atoi(e.arg1))) return;

		} else if (strcmp(e.comando, "SIGUIENTE") == 0) {

		    if (pagina < totalPags) pagina++; else pagina = 1;

		} else if (strcmp(e.comando, "ANTERIOR") == 0) {

		    if (pagina > 1) pagina--; else pagina = totalPags;

		} else if (strcmp(e.comando, "HOME") == 0) {

		    return;

		} else if (strcmp(e.comando, "EXIT") == 0) {

			salir = 1; return;

		} else {

		    imprimirError((char*)"Comando no reconocido.");
		    pausar();

		}

	}

}

// FAVORITOS

void pantallaFavoritos(Client& cliente) {

    LOG_INFO("Accediendo a la pantalla de favoritos.");
    pantallaBuscar(cliente, true);

}

// VER PRODUCTO

static char* cmdsVerProd[] = {

		(char*)"COMPRAR", (char*)"FAVORITO", (char*)"RESENA", (char*)"VOLVER", (char*)"HOME", (char*)"EXIT"

};

#define N_CMDS_VER_PROD 6

int pantallaVerProducto(Client& cliente, int idProd) {

    LOG_INFO("Cargando detalle de producto #%d", idProd);

    while (!salir) {

        cliente.enviar("GET_PROD_DETALLE|" + to_string(idProd));
        string respuesta = cliente.recibir();
        string vistaFormateada = "";

        if (respuesta.find("OK|") == 0) {

            vistaFormateada = respuesta.substr(3);

        } else {

            string error = "Producto no encontrado.";
            if (respuesta.find("ERR|") == 0) error = respuesta.substr(4);
            imprimirError((char*)error.c_str());
            LOG_WARN("Error al cargar producto #%d: %s", idProd, error.c_str());
            pausar();
            break;

        }

        imprimirCabecera((char*)"DETALLE DE PRODUCTO", (char*)"Informacion detallada del articulo");
        cout << vistaFormateada;

        if (cliente.isAutenticado()) {

			imprimirSeccion((char*)"COMANDOS");
			cout << "  " << ESTILO_CMD << "COMPRAR [cant]   " << RESET << "Añadir al carrito\n";
			cout << "  " << ESTILO_CMD << "FAVORITO         " << RESET << "Añadir o quitar de favoritos\n";
			cout << "  " << ESTILO_CMD << "RESENA           " << RESET << "Ver o escribir reseñas\n";
			cout << "  " << ESTILO_CMD << "VOLVER           " << RESET << "Volver al listado anterior\n\n";

        } else {

        	imprimirWarn((char*)"Inicia sesión para actuar sobre el producto.");
        	pausar();
        	break;

        }

        Entrada e = leerComando(cmdsVerProd, N_CMDS_VER_PROD, (char*)">");

        if (strcmp(e.comando, "COMPRAR") == 0) {

            int cantidad = 1;
            if (strlen(e.arg1) > 0) {

                cantidad = atoi(e.arg1);
                if (cantidad <= 0) cantidad = 1;

            } else cantidad = leerEntero((char*)"Cantidad a comprar:", 1, 9999);

            string variante = seleccionarVariante(cliente, idProd);

            if (!variante.empty()) {

                LOG_INFO("Intentando añadir al carrito: Prod #%d, Cant: %d, Var %s", idProd, cantidad, variante.c_str());
                cliente.enviar("ADD_CARRITO|" + to_string(idProd) + "|" + to_string(cantidad) + "|" + variante);
                string respCarrito = cliente.recibir();

                if (respCarrito.find("OK") == 0) {

                    imprimirExito((char*)"Producto añadido al carrito correctamente.");
                    LOG_INFO("Exito al añadir al carrito.");

                } else {

                    string err = "No se pudo añadir al carrito.";
                    if (respCarrito.find("ERR|") == 0) err = respCarrito.substr(4);
                    imprimirError((char*)err.c_str());
                    LOG_WARN("Error al añadir al carrito: %s", err.c_str());

                }

                pausar();

            }

        } else if (strcmp(e.comando, "FAVORITO") == 0) {

            cliente.enviar("TOGGLE_FAVORITO|" + to_string(idProd));
            string respFav = cliente.recibir();

            if (respFav.find("OK|") == 0) {

                string msg = respFav.substr(3);
                char imp[128];
                snprintf(imp, sizeof(imp), "Producto #%d %s favoritos", idProd, stoi(msg) ? "añadido a" : "eliminado de");
                if (stoi(msg)) {
                	imprimirExito(imp);
                } else {
                	imprimirExito(imp);
                }
                LOG_INFO(imp);

            } else {

                string err = "Error al actualizar favoritos.";
                if (respFav.find("ERR|") == 0) err = respFav.substr(4);
                imprimirError((char*)err.c_str());
                LOG_WARN("Fallo en toggle favorito: %s", err.c_str());

            }

            pausar();

        } else if (strcmp(e.comando, "RESENA") == 0) {

            if (pantallaResenas(cliente, idProd)) return 1;

        } else if (strcmp(e.comando, "VOLVER") == 0) {

            break;

        } else if (strcmp(e.comando, "HOME") == 0) {

            return 1;

        } else if (strcmp(e.comando, "EXIT") == 0) {

            salir = 1; return 1;

        } else {

            imprimirError((char*)"Comando no reconocido.");
            pausar();

        }

    }

    return 0;

}

// RESEÑAS

static char* cmdsResenasSinResena[] = {

		(char*)"ESCRIBIR", (char*)"ANTERIOR", (char*)"SIGUIENTE", (char*)"VOLVER", (char*)"HOME", (char*)"EXIT"

};

#define N_CMDS_RESENAS_SR 6

static char* cmdsResenasConResena[] = {

		(char*)"EDITAR", (char*)"ELIMINAR", (char*)"ANTERIOR", (char*)"SIGUIENTE", (char*)"VOLVER", (char*)"HOME", (char*)"EXIT"

};

#define N_CMDS_RESENAS_CR 7

int pantallaResenas(Client& cliente, int idProd) {

    int pagina = 1;
    int totalPags = 1;

    while (!salir) {

        string peticion = "GET_RESENAS|" + to_string(idProd) + "|" + to_string(pagina);
        cliente.enviar(peticion);
        string respuesta = cliente.recibir();
        bool tieneRes = false;
        string tablaFormateada = "";

        if (respuesta.find("OK|") == 0) {

            size_t posPipe = respuesta.find('|', 3);

            if (posPipe != string::npos) {

                try {
                	totalPags = stoi(respuesta.substr(3, posPipe - 3));
                } catch (...) {
                	totalPags = 1;
                }

                tieneRes = (respuesta[posPipe + 1] == '1');

                if (posPipe + 3 < respuesta.length()) {

                    tablaFormateada = respuesta.substr(posPipe + 3);

                } else {

                    tablaFormateada = "";

                }

            }

        } else {

            string error = "Error al cargar las resenas.";
            if (respuesta.find("ERR|") == 0) error = respuesta.substr(4);
            tablaFormateada = "  " + string(C_AMARILLO) + error + string(RESET) + "\n";
            LOG_WARN("No se pudieron cargar resenas para prod #%d: %s", idProd, error.c_str());

        }

        imprimirCabecera((char*)"RESEÑAS DEL PRODUCTO", (char*)"Opiniones de otros usuarios");
        if (tablaFormateada.empty()) {
        	cout << "\033[A";
        	imprimirWarn((char*)"Aun no hay reseñas para este producto. ¡Se el primero!");
        }
        else cout << tablaFormateada;

        imprimirSeccion((char*)"COMANDOS");
        if (!tieneRes) cout << "  " << ESTILO_CMD << "ESCRIBIR               " << RESET << "Publicar una nueva resena\n";
        else {
        	cout << "  " << ESTILO_CMD << "EDITAR                 " << RESET << "Editar una antigua resena\n";
        	cout << "  " << ESTILO_CMD << "ELIMINAR               " << RESET << "Eliminar una antigua resena\n";
        }
        if (totalPags > 1) cout << "  " << ESTILO_CMD << "ANTERIOR / SIGUIENTE   " << RESET  << "Navegar paginas\n";
        cout << "  " << ESTILO_CMD << "VOLVER                 " << RESET << "Volver al detalle del producto\n";
        cout << "  " << ESTILO_CMD << "HOME                   " << RESET << "Volver al menu principal\n\n";

        Entrada e = leerComando(tieneRes? cmdsResenasConResena : cmdsResenasSinResena, tieneRes? N_CMDS_RESENAS_CR : N_CMDS_RESENAS_SR, (char*)">");

        if (strcmp(e.comando, "ESCRIBIR") == 0 || strcmp(e.comando, "EDITAR") == 0) {

            double puntuacion;
            if (!tieneRes) puntuacion = leerDouble((char*)"Puntuacion (0 a 5):", 0, 5);
            else puntuacion = leerDouble((char*)"Nueva puntuacion (0 a 5):", 0, 5);
            char bufferComentario[256] = {0};
            if (!tieneRes) leerTexto((char*)"Escribe tu comentario:", bufferComentario, sizeof(bufferComentario));
            else leerTexto((char*)"Edita tu comentario:", bufferComentario, sizeof(bufferComentario));
            string comentario = bufferComentario;

            LOG_INFO("Enviando nueva reseña para prod #%d (Punt: %d)", idProd, puntuacion);
            cliente.enviar("ADD_RESENA|" + to_string(idProd) + "|" + to_string(puntuacion) + "|" + comentario);
            string respNueva = cliente.recibir();

            if (respNueva.find("OK") == 0) {

                imprimirExito((char*)"¡Reseña publicada con exito!");
                LOG_INFO("Reseña publicada correctamente.");
                pagina = 1;

            } else {

                string err = "No se pudo publicar la reseña.";
                if (respNueva.find("ERR|") == 0) err = respNueva.substr(4);
                imprimirError((char*)err.c_str());
                LOG_WARN("Fallo al publicar la reseña: %s", err.c_str());

            }

            pausar();

        } else if (strcmp(e.comando, "ELIMINAR") == 0) {

        	if (confirmar((char*)"¿Seguro que quieres borrar tu reseña?")) {

        		cliente.enviar("ELIMINAR_RESENA|" + to_string(idProd));

        		string respNueva = cliente.recibir();

        		if (respNueva.find("OK") == 0) {

        			imprimirExito((char*)"Reseña eliminada correctamente.");
                    LOG_INFO("Reseña eliminada correctamente.");
                    pagina = 1;

        		} else {

        			string err = "No se pudo eliminar la reseña.";
                    if (respNueva.find("ERR|") == 0) err = respNueva.substr(4);
                    imprimirError((char*)err.c_str());
                    LOG_WARN("Fallo al eliminar la reseña: %s", err.c_str());

        		}

        		pausar();

        	}

        } else if (strcmp(e.comando, "SIGUIENTE") == 0) {

            if (pagina < totalPags) pagina++;
            else pagina = 1;

        } else if (strcmp(e.comando, "ANTERIOR") == 0) {

            if (pagina > 1) pagina--;
            else pagina = totalPags;

        } else if (strcmp(e.comando, "VOLVER") == 0) {

            break;

        } else if (strcmp(e.comando, "HOME") == 0) {

            return 1;

        } else if (strcmp(e.comando, "EXIT") == 0) {

            salir = 1; return 1;

        } else {

            imprimirError((char*)"Comando no reconocido.");
            pausar();

        }

    }

    return 0;

}
