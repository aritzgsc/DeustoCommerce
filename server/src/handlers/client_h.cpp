#include "server.h"
#include "client_h.h"
#include "protocolo.h"
#include "mail_utils.h"
#include <string>
#include <sstream>
#include <cstring>
#include <cstdio>

extern "C" {
    #include "usuario_db.h"
    #include "utils_ui.h"
    #include "logistica.h"
    #include "finanzas.h"
    #include "config.h"
    #include "log.h"
	#include "api.h"
}

using namespace std;

// AUXILIARES INTERNAS

// Construye el string de precio unitario con descuento aplicado si lo hay
static string buildPrecioUnitStr(double precio, double descuento) {
    char buf[32];
    if (descuento > 0)
        snprintf(buf, sizeof(buf), "%.2f€ -%d%%", precio * (1.0 - descuento), (int)(descuento * 100));
    else
        snprintf(buf, sizeof(buf), "%.2f€", precio);
    return string(buf);
}

// Colorea el estado del pedido según su valor
static string buildEstadoStr(const char* estado) {
    if (!estado) return string(C_GRIS) + "—" + RESET;
    if (strcmp(estado, "ENTREGADO") == 0) return string(ESTILO_EXITO) + estado + RESET;
    if (strcmp(estado, "EN CAMINO") == 0) return string(ESTILO_WARN) + estado + RESET;
    if (strcmp(estado, "EN CAMINO") == 0) return string(ESTILO_ERROR) + estado + RESET;
    return string(C_GRIS) + estado + RESET;   // EN PROCESO u otro
}

// CLIENT HANDLERS

void handleGetEstadoCliente(SSL* ssl, sqlite3* db, Sesion& sesion) {

    const char* correo = sesion.getCorreo().c_str();

    int nCarrito = getItemsCarrito(db, correo);
    if (nCarrito < 0) {
        responder(ssl, "ERR|Error al obtener items del carrito.");
        LOG_ERROR("GetItemsCarrito falló para %s", correo);
        return;
    }

    int nFavs = getItemsFavoritos(db, correo);
    if (nFavs < 0) {
        responder(ssl, "ERR|Error al obtener favoritos.");
        LOG_ERROR("GetItemsFavoritos falló para %s", correo);
        return;
    }

    int nCurso = getPedidosCurso(db, correo);
    if (nCurso < 0) {
        responder(ssl, "ERR|Error al obtener pedidos en curso.");
        LOG_ERROR("GetPedidosCurso falló para %s", correo);
        return;
    }

    string res = "OK|" + to_string(nCarrito) + "|" + to_string(nFavs) + "|" + to_string(nCurso);
    responder(ssl, res);
    LOG_INFO("%s → carrito=%d favs=%d curso=%d", correo, nCarrito, nFavs, nCurso);

}

void handleGetDirecciones(SSL* ssl, sqlite3* db, Sesion& sesion) {

    const char* correo = sesion.getCorreo().c_str();

    int nUbs = 0;
    Ubicacion* ubs = getUbicaciones(db, correo, &nUbs);

    if (!ubs && nUbs == 0) {

        responder(ssl, "OK|0|");
        LOG_INFO("%s no tiene direcciones guardadas", correo);
        return;

    }

    if (!ubs) {

        responder(ssl, "ERR|Error de conexión con BD.");
        LOG_ERROR("GetDirecciones devolvió NULL para %s", correo);
        return;

    }

    string texto = "";
    for (int i = 0; i < nUbs; i++) {

        Ubicacion* u = &ubs[i];
        char linea[512];

        // Cabecera de la dirección: Índice + País, Ciudad
        snprintf(linea, sizeof(linea), "  " ESTILO_ID "%d" RESET "  " C_BLANCO "%s" RESET" (" C_BLANCO "%s" RESET ", " C_GRIS "%s" RESET ")\n", i + 1, u->direccion, u->ciudad.nombre, u->ciudad.pais.nombre);
        texto += linea;

    }

    texto += "\n";

    liberarUbicaciones(ubs, nUbs);

    string res = "OK|" + to_string(nUbs) + "|" + texto;
    responder(ssl, res);
    LOG_INFO("%s → %d direcciones", correo, nUbs);

}

static Columna colsCarrito[] = {

    { (char*)"                   PRODUCTO                   ", 46,  0 },
    { (char*)"     VARIANTE     ",                  18,  0 },
    { (char*)" CANT ", 6,  1 },
    { (char*)"  PRECIO/U  ",                       12,  1 },
    { (char*)"   SUBTOTAL   ",                      14,  1 }

};

#define N_COLS_CARRITO 5

void handleGetCarrito(SSL* ssl, sqlite3* db, Sesion& sesion) {

    const char* correo = sesion.getCorreo().c_str();

    int nItems = 0;
    ItemCarrito* items = getCarrito(db, correo, &nItems);

    if (nItems == 0) {

        if (items) liberarCarrito(items, 0);
        responder(ssl, "OK|0|");
        LOG_INFO("Carrito vacío para %s", correo);
        return;

    }

    if (!items) {

        responder(ssl, "ERR|Error de conexión con BD.");
        LOG_ERROR("GetCarrito devolvió NULL para %s", correo);
        return;

    }

    string tabla = "";
    char* cab = getCabeceraTabla(colsCarrito, N_COLS_CARRITO);
    if (cab) { tabla += cab; free(cab); }

    double totalGeneral = 0.0;

    for (int i = 0; i < nItems; i++) {

        ItemCarrito* item = &items[i];

        double precioFinal = item->precioUnitario * (1.0 - item->descuento);
        double subtotal = precioFinal * item->cantidad;
        totalGeneral += subtotal;

        char nombre[47]; strncpy(nombre, item->nombreProducto, 46); nombre[46] = '\0';
        char var[19]; strncpy(var, item->variante, 18); var[18] = '\0';
        char cantStr[8], subtotalStr[16];
        snprintf(cantStr, sizeof(cantStr), "%d", item->cantidad);
        snprintf(subtotalStr, sizeof(subtotalStr), "%.2f€", subtotal);

        string precioUStr = buildPrecioUnitStr(item->precioUnitario, item->descuento);

        char* fila[]  = { nombre, var, cantStr, (char*)precioUStr.c_str(), subtotalStr };
        char* filaStr = getFilaTabla(fila, colsCarrito, N_COLS_CARRITO, i % 2);
        if (filaStr) { tabla += filaStr; free(filaStr); }

    }

    char* pie = getPieTabla(colsCarrito, N_COLS_CARRITO);
    if (pie) { tabla += pie; free(pie); }

    // Total general bajo la tabla, alineado a la derecha del ancho de la tabla
    char totalLine[128];
    snprintf(totalLine, sizeof(totalLine), "\n  " ESTILO_SUBTITULO "TOTAL: " RESET ESTILO_PRECIO "%.2f€\n" RESET, totalGeneral);
    tabla += totalLine;

    liberarCarrito(items, nItems);

    if (tabla.empty()) {

        responder(ssl, "ERR|Error al crear la tabla formateada.");
        LOG_ERROR("Tabla vacía para %s", correo);
        return;

    }

    string res = "OK|" + to_string(nItems) + "|" + tabla;
    responder(ssl, res);
    LOG_INFO("%s → %d items total=%.2f€", correo, nItems, totalGeneral);

}

void handleVaciarCarrito(SSL* ssl, sqlite3* db, Sesion& sesion) {

    const char* correo = sesion.getCorreo().c_str();

    if (vaciarCarrito(db, correo) == 0) {

        responder(ssl, "OK");
        LOG_INFO("Carrito vaciado para %s", correo);

    } else {

        responder(ssl, "ERR|Error de conexión con BD.");
        LOG_ERROR("Error al vaciar carrito de %s", correo);

    }

}

void handlePagarCarrito(SSL* ssl, sqlite3* db, string args[], Sesion& sesion) {

    const char* correo = sesion.getCorreo().c_str();

    int nItems = getItemsCarrito(db, correo);

    if (nItems <= 0) {

        responder(ssl, nItems == 0 ? "ERR|El carrito está vacío." : "ERR|Error de conexión con BD.");
        LOG_WARN("%s intentó pagar con carrito vacío (n=%d)", correo, nItems);
        return;

    }

    string tipoDireccion = args[1];
    int idxUb = -1;
    int idUbicacion = -1;
    bool existe = false;

    if (tipoDireccion == "GUARDADA") {

        idxUb = stoi(args[2]);

        int nUbs = 0;
        Ubicacion* ubs = getUbicaciones(db, correo, &nUbs);

        if (ubs && idxUb > 0 && idxUb <= nUbs) {

        	idUbicacion = ubs[idxUb - 1].id;
            existe = true;
            liberarUbicaciones(ubs, nUbs);

        } else if (ubs) liberarUbicaciones(ubs, nUbs);

        if (!existe) {

            responder(ssl, "ERR|Dirección guardada no encontrada.");
            LOG_WARN("%s pidió una ubicación que no existe", correo);
            return;

        }


    } else if (tipoDireccion == "NUEVA") {

        string pais = args[2];
        string ciudad = args[3];
        string direccion = args[4];

        Ubicacion ub;

        if (completarUbicacion(&ub, pais.c_str(), ciudad.c_str(), direccion.c_str()) == 0) {

			idUbicacion = crearUbicacion(db, correo, pais.c_str(), ciudad.c_str(), direccion.c_str(), ub.latitud, ub.longitud);
			liberarUbicacionApi(&ub);

			if (idUbicacion < 0) {

				responder(ssl, "ERR|Error al crear la ubicación.");
				LOG_ERROR("Error al crear la ubicación para %s [%s] (%s, %s)", correo, direccion.c_str(), ciudad.c_str(), pais.c_str());
				return;

			}

			LOG_INFO("Nueva ubicación #%d creada para %s", idUbicacion, correo);

        } else {

        	responder(ssl, "ERR|Ubicación no reconocida.");
        	LOG_WARN("No se ha reconocido la ubicación [%s] (%s, %s)", direccion.c_str(), ciudad.c_str(), pais.c_str());
        	return;

        }

    } else {

        responder(ssl, "ERR|Tipo de dirección no reconocido.");
        LOG_WARN("Tipo de dirección desconocido '%s' de %s", tipoDireccion.c_str(), correo);
        return;

    }

    double costeEnvio = 0;
    time_t duracion = 0;
    calcularCosteEnvio(db, correo, idUbicacion, &costeEnvio, &duracion);

    if (costeEnvio < 0) {
        responder(ssl, "ERR|No hay stock suficiente para completar el pedido.");
        LOG_WARN("Falta de stock general para pedido de %s", correo);
        return;
    }

    double totalIngreso = 0;
    int cantItems = 0;
    ItemCarrito* carrito = getCarrito(db, correo, &cantItems);

    if (carrito) {

    	for (int i = 0 ; i < cantItems ; i++) {

    		totalIngreso += carrito[i].precioUnitario * (1.0 - carrito[i].descuento) * carrito[i].cantidad;

    	}

		time_t timestampEjecucion = time(NULL) + duracion;

		int idPedido = crearPedido(db, correo, sesion.getNombre().c_str(), sesion.getApellido().c_str(), idUbicacion, timestampEjecucion);

		if (idPedido > 0) {

			char rutaRegFinanciero[512];
			configGet(CONFIG_PATH, (char*)"REG_FINANCIERO_PATH", rutaRegFinanciero, sizeof(rutaRegFinanciero));

			char conceptoVenta[128];
			snprintf(conceptoVenta, sizeof(conceptoVenta), "Pedido de %d articulo(s) para %s", cantItems, correo);
			registrarTransaccion(rutaRegFinanciero, (char*)"INGRESO", (char*)"VENTA_PEDIDO", conceptoVenta, totalIngreso);

			char conceptoEnvio[128];
			snprintf(conceptoEnvio, sizeof(conceptoEnvio), "Envio a ubicacion #%d", idUbicacion);
			registrarTransaccion(rutaRegFinanciero, (char*)"GASTO", (char*)"ENVIO_PAQUETERIA", conceptoEnvio, costeEnvio);

			Ubicacion* ub = getUbicacionPorId(db, idUbicacion);

			if (enviarMailPedidoEnProceso(correo, sesion.getNombre().c_str(), idPedido, timestampEjecucion, carrito, cantItems, *ub)) {

				LOG_INFO("Correo de confirmación de pedido #%d enviado a %s", idPedido, correo);

			} else {

				LOG_ERROR("Error al enviar el correo de confirmación de pedido #%d a %s", idPedido, correo);

			}

			responder(ssl, "OK");
			LOG_INFO("Pedido creado para %s a ubicación #%d", correo, idUbicacion);

		} else {

			responder(ssl, "ERR|Error de conexión con BD.");
			LOG_ERROR("Error al crear pedido para %s a ubicación #%d", correo, idUbicacion);

		}

		liberarCarrito(carrito, cantItems);

	}

}

static Columna colsPedidos[] = {

    { (char*)"  FECHA Y HORA  ",    18,  2 },
    { (char*)"        ESTADO        ",         22,  2 },
    { (char*)"    PRECIO    ",         14,  2 },
    { (char*)"                  DIRECCIÓN                  ", 45, 2 }

};

#define N_COLS_PEDIDOS 4

void handleGetPedidos(SSL* ssl, sqlite3* db, string args[], Sesion& sesion) {

    const char* correo = sesion.getCorreo().c_str();
    int pagina = stoi(args[1]);

    int total = 0;
    Pedido* pedidos = getPedidos(db, correo, pagina, &total);

    if (total == 0) {

        responder(ssl, "OK|1|");
        LOG_INFO("%s no tiene pedidos", correo);
        return;

    }

    if (!pedidos) {

        responder(ssl, "ERR|Error de conexión con BD.");
        LOG_ERROR("GetPedidos devolvió NULL para %s (total=%d)", correo, total);
        return;

    }

    int totalPags = (total + PEDIDOS_POR_PAGINA - 1) / PEDIDOS_POR_PAGINA;
    int enEstaPag = total - (pagina - 1) * PEDIDOS_POR_PAGINA;
    if (enEstaPag > PEDIDOS_POR_PAGINA) enEstaPag = PEDIDOS_POR_PAGINA;
    if (enEstaPag < 0) enEstaPag = 0;

    // Construir tabla
    string tabla = "";
    char* cab = getCabeceraTabla(colsPedidos, N_COLS_PEDIDOS);
    if (cab) { tabla += cab; free(cab); }

    for (int i = 0; i < enEstaPag; i++) {

        Pedido* ped = &pedidos[i];

        char totalStr[14];
        snprintf(totalStr, sizeof(totalStr), "%.2f€",  ped->total);

        char fecha[17]; strncpy(fecha, ped->fecha, 16); fecha[16] = '\0';
        char resumen[46]; strncpy(resumen, ped->resumenDir, 45); resumen[45] = '\0';

        string estadoStr = buildEstadoStr(ped->estado);

        char* fila[] = { fecha, (char*)estadoStr.c_str(), totalStr, resumen };
        char* filaStr = getFilaTabla(fila, colsPedidos, N_COLS_PEDIDOS, i % 2);
        if (filaStr) { tabla += filaStr; free(filaStr); }

    }

    char* pie = getPieTabla(colsPedidos, N_COLS_PEDIDOS);
    if (pie) { tabla += pie; free(pie); }

    tabla += getPaginacion(pagina, totalPags, total);

    liberarPedidos(pedidos, enEstaPag);

    if (tabla.empty()) {

        responder(ssl, "ERR|Error al crear la tabla formateada.");
        LOG_ERROR("Tabla vacía para %s pag=%d", correo, pagina);
        return;

    }

    string res = "OK|" + to_string(totalPags) + "|" + tabla;
    responder(ssl, res);
    LOG_INFO("Resultado de histórico de pedidos (%s): pag=%d/%d total=%d", correo, pagina, totalPags, total);

}
