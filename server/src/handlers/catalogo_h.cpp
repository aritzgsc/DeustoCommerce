#include "catalogo_h.h"
#include "protocolo.h"
extern "C" {
    #include "catalogo_db.h"
    #include "usuario_db.h"
    #include "estructuras.h"
    #include <utils_ui.h>
    #include "log.h"
}
#include <string>
#include <sstream>
#include <cstring>
#include <cmath>

using namespace std;

// AUXILIARES INTERNAS

// Construye el string de precio con descuento aplicado, si lo hay (Sin Color)
static string buildPrecioStrSC(double precio, double descuento) {

    char buf[32];
    if (descuento > 0) snprintf(buf, sizeof(buf), "%.2f€ -%d%%", precio * (1.0 - descuento), (int)(descuento * 100));
    else snprintf(buf, sizeof(buf), "%.2f€", precio);
    return string(buf);

}

// Construye el string de precio con descuento aplicado, si lo hay (Con Color)
static string buildPrecioStrCC(double precio, double descuento) {

    char buf[32];
    if (descuento > 0) snprintf(buf, sizeof(buf), ESTILO_PRECIO "%.2f€" RESET C_GRIS "(-%d%%)", precio * (1.0 - descuento), (int)(descuento * 100));
    else snprintf(buf, sizeof(buf), "%.2f€", precio);
    return string(buf);

}

// Construye el string de valoración con estrellas Unicode: ★★★★☆ 4.2
static string buildEstrellasStr(double val) {

    if (val < 0) return string(C_GRIS) + "Sin valoracion" + RESET;
    int llenas = (int)round(val / 5.0 * 5);
    int vacias  = 5 - llenas;
    string s = C_AMARILLO;
    for (int i = 0; i < llenas; i++) s += "★";
    s += C_GRIS;
    for (int i = 0; i < vacias; i++) s += "☆";
    char num[16];
    snprintf(num, sizeof(num), " %.1f", val);
    return s + RESET + C_GRIS + num + RESET;

}

// Construye la tabla formateada de productos (catálogo/búsqueda)

static Columna colsCatalogo[] = {

    { (char*)"  ID  ",                                 6,  1 },
    { (char*)"                    NOMBRE                    ",  46,  0 },
    { (char*)"       CATEGORIA       ",               23,  0 },
    { (char*)"  PRECIO (€)  ",                        14,  1 },
    { (char*)" STOCK ",                                7,  1 }

};

#define N_COLS_CATALOGO 5

static string buildTablaCatalogo(sqlite3* db, Producto* prods, int n) {

	string tabla = "";

    char* cab = getCabeceraTabla(colsCatalogo, N_COLS_CATALOGO);
    if (!cab) return "";
    tabla += cab; free(cab);

    for (int i = 0; i < n; i++) {

        Producto* p = &prods[i];

        char idStr[8], stockStr[8];
        snprintf(idStr, sizeof(idStr), "%d", p->id);
        snprintf(stockStr, sizeof(stockStr), "%d", getStockProducto(db, p->id, -1));

        char nombre[47]; strncpy(nombre, p->nombre, 46); nombre[46] = '\0';
        char cat[24]; strncpy(cat, p->categoria.nombre, 23); cat[23]    = '\0';

        string ps = buildPrecioStrSC(p->precio, p->descuento);

        char* fila[]  = { idStr, nombre, cat, (char*)ps.c_str(), stockStr };
        char* filaStr = getFilaTabla(fila, colsCatalogo, N_COLS_CATALOGO, i % 2);
        if (filaStr) { tabla += filaStr; free(filaStr); }

    }

    char* pie = getPieTabla(colsCatalogo, N_COLS_CATALOGO);
    if (pie) { tabla += pie; free(pie); }

    return tabla;

}

// CATÁLOGO HANDLERS

static Columna colsCategorias[] = {

    { (char*)"  ID  ",                                 6,  1 },
    { (char*)"                  NOMBRE                  ",  42,  2 }

};

#define N_COLS_CAT 2

void handleGetCategorias(SSL* ssl, sqlite3* db, string args[]) {

    int pagina = stoi(args[1]);
    string filtro = args[2];

    int nCats = 0;
    Categoria* cats = getCategorias(db, &nCats);

    if (!cats) {

        responder(ssl, "ERR|Error de conexión con BD.");
        LOG_ERROR("Error de BD al obtener las categorías");
        return;

    }

    Categoria** filtradas = (Categoria**)malloc(sizeof(Categoria*) * nCats);

    if (!filtradas) {

        responder(ssl, "ERR|Error interno del servidor.");
        for (int i = 0; i < nCats; i++) free(cats[i].nombre);
        free(cats);
        return;

    }

    int nFiltradas = 0;
    for (int i = 0; i < nCats; i++) {
        if (filtro.empty() || _strnicmp(cats[i].nombre, filtro.c_str(), filtro.length()) == 0 || strstr(cats[i].nombre, filtro.c_str()) != NULL) {
            filtradas[nFiltradas++] = &cats[i];
        }
    }

    int totalPags = nFiltradas > 0 ? (nFiltradas + CATS_POR_PAGINA - 1) / CATS_POR_PAGINA : 1;
    if (pagina < 1) pagina = 1;
    if (pagina > totalPags) pagina = totalPags;

    // Construir tabla
    string tabla = "";
    char* cab = getCabeceraTabla(colsCategorias, N_COLS_CAT);
    if (cab) { tabla += cab; free(cab); }

    int inicio = (pagina - 1) * CATS_POR_PAGINA;
    int fin = inicio + CATS_POR_PAGINA < nFiltradas ? inicio + CATS_POR_PAGINA : nFiltradas;

    for (int i = inicio; i < fin; i++) {

        char idStr[8];
        snprintf(idStr, sizeof(idStr), "%d", filtradas[i]->id);
        char* fila[] = { idStr, filtradas[i]->nombre };
        char* filaStr = getFilaTabla(fila, colsCategorias, N_COLS_CAT, i % 2);
        if (filaStr) { tabla += filaStr; free(filaStr); }

    }

    char* pie = getPieTabla(colsCategorias, N_COLS_CAT);
    if (pie) { tabla += pie; free(pie); }

    tabla += getPaginacion(pagina, totalPags, nCats);

    // Liberar
    free(filtradas);
    for (int i = 0; i < nCats; i++) free(cats[i].nombre);
    free(cats);

    if (tabla.empty()) {
        responder(ssl, "ERR|Error al crear la tabla formateada.");
        LOG_ERROR("Tabla vacía al obtener categorías (pag=%d filtro='%s')", pagina, filtro.c_str());
        return;
    }

    string res = "OK|" + to_string(totalPags) + "|" + tabla;
    responder(ssl, res);
    LOG_INFO("Resultados de obtener categorías para: pag=%d filtro='%s' resultados=%d", pagina, filtro.c_str(), nFiltradas);

}

void handleGetVariantes(SSL* ssl, sqlite3* db, string args[]) {

    int idProd = stoi(args[1]);

    Producto* p = getProductoPorId(db, idProd);
    if (!p) {

        responder(ssl, "ERR|El producto no existe en la BD.");
        LOG_WARN("Producto #%d no encontrado", idProd);
        return;

    }

    int n = p->categoria.nVariantes;

    string res = "OK|" + to_string(n);

    for (int i = 0; i < n; i++) {

    	res += "|" + string(p->categoria.variantes[i]);

    }

    liberarProducto(p);
    free(p);

    responder(ssl, res);
    LOG_INFO("Producto #%d → %d variante(s)", idProd, n);

}

void handleGetCatalogo(SSL* ssl, sqlite3* db, string args[]) {

    int pagina = stoi(args[1]);
    int total  = 0;

    FiltrosProducto f = filtrosVacios();
    Producto* prods   = buscarProductos(db, f, pagina, &total);

    if (total == 0) {

        responder(ssl, "OK|1|");
        LOG_INFO("Catálogo vacío");
        return;

    }

    if (!prods) {

        responder(ssl, "ERR|Error de conexión con BD.");
        LOG_ERROR("Error de BD al buscar productos", total);
        return;

    }

    int totalPags = (total + ITEMS_POR_PAGINA - 1) / ITEMS_POR_PAGINA;
    int enEstaPag = total - (pagina - 1) * ITEMS_POR_PAGINA;
    if (enEstaPag > ITEMS_POR_PAGINA) enEstaPag = ITEMS_POR_PAGINA;
    if (enEstaPag < 0) enEstaPag = 0;

    string tabla = buildTablaCatalogo(db, prods, enEstaPag);

    tabla += getPaginacion(pagina, totalPags, total);

    for (int i = 0; i < enEstaPag; i++) liberarProducto(&prods[i]);
    free(prods);

    if (tabla.empty()) {
        responder(ssl, "ERR|Error al crear la tabla formateada.");
        LOG_ERROR("Tabla vacía (pag=%d)", pagina);
        return;
    }

    string res = "OK|" + to_string(totalPags) + "|" + tabla;
    responder(ssl, res);
    LOG_INFO("Resultados de búsqueda de productos pag=%d/%d total=%d", pagina, totalPags, total);

}

void handleBuscar(SSL* ssl, sqlite3* db, string args[], Sesion& sesion) {

    int pagina = stoi(args[1]);
    string fNombre = args[2];
    int fIdCat = stoi(args[3]);
    double fPrecioMin = stod(args[4]);
    double fPrecioMax = stod(args[5]);
    int soloFavs = stoi(args[6]);

    FiltrosProducto f = filtrosVacios();
    if (!fNombre.empty()) strncpy(f.nombre, fNombre.c_str(), sizeof(f.nombre) - 1);
    f.idCategoria = fIdCat;
    f.precioMin = fPrecioMin;
    f.precioMax = fPrecioMax;

    int total = 0;
    Producto* prods = soloFavs ? buscarProductosFavoritos(db, sesion.getCorreo().c_str(), f, pagina, &total) : buscarProductos(db, f, pagina, &total);

    if (total == 0) {

        responder(ssl, "OK|1|");
        LOG_INFO("Sin resultados para: %s ; fNom='%s' ; fCat=%d ; fAvs=%d", sesion.getCorreo().c_str(), fNombre.c_str(), fIdCat, soloFavs);
        return;

    }

    if (!prods) {

        responder(ssl, "ERR|Error de conexión con BD.");
        LOG_ERROR("Error de BD al buscar productos", total);
        return;

    }

    int totalPags = (total + ITEMS_POR_PAGINA - 1) / ITEMS_POR_PAGINA;
    int enEstaPag = total - (pagina - 1) * ITEMS_POR_PAGINA;
    if (enEstaPag > ITEMS_POR_PAGINA) enEstaPag = ITEMS_POR_PAGINA;
    if (enEstaPag < 0) enEstaPag = 0;

    string tabla = buildTablaCatalogo(db, prods, enEstaPag);

    tabla += getPaginacion(pagina, totalPags, total);

    for (int i = 0; i < enEstaPag; i++) liberarProducto(&prods[i]);
    free(prods);

    if (tabla.empty()) {

        responder(ssl, "ERR|Error al crear la tabla formateada.");
        LOG_ERROR("Tabla vacía (pag=%d)", pagina);
        return;

    }

    string res = "OK|" + to_string(totalPags) + "|" + tabla;
    responder(ssl, res);
    LOG_INFO("Resultados de búsqueda de productos: pag=%d/%d ; total=%d ; favs=%d ; correo=%s", pagina, totalPags, total, soloFavs, sesion.getCorreo().c_str());

}

void handleGetProdDetalle(SSL* ssl, sqlite3* db, string args[]) {

    int idProd = stoi(args[1]);

    Producto* p = getProductoPorId(db, idProd);
    if (!p) {
        responder(ssl, "ERR|El producto no existe en la BD.");
        LOG_WARN("Producto #%d no encontrado", idProd);
        return;
    }

    string vista = "";

    // ID y nombre
    char bufNom[512];
    char nombre[512];
    wordWrap(nombre, p->nombre, 6);
    snprintf(bufNom, sizeof(bufNom), "  " ESTILO_ID "#%d" RESET "  " NEGRITA C_BLANCO "%s\n\n" RESET, p->id, nombre);
    vista += bufNom;

    // Categoría
    char bufCat[256];
    snprintf(bufCat, sizeof(bufCat), "  " ESTILO_HINT "%s\n\n" RESET, p->categoria.nombre);
    vista += bufCat;

    // Descripción

    if (p->descripcion && strlen(p->descripcion) > 0) {
        char desc[1024] = "";
        wordWrap(desc, p->descripcion, 2);
        char buf[1100];
        snprintf(buf, sizeof(buf), "  %s\n\n", desc);
        vista += buf;
    }

    // Precio

    string ps = buildPrecioStrCC(p->precio, p->descuento);
    char bufPrecio[256];
    snprintf(bufPrecio, sizeof(bufPrecio), "  Precio:      %s" RESET "\n", ps.c_str());
    vista += bufPrecio;

    // Valoración con estrellas y número de reseñas

    double val = getValoracionMedia(db, p->id);
    int nResenas = getNumResenas(db, p->id);
    string estrellas = buildEstrellasStr(val);
    char bufVal[512];
    snprintf(bufVal, sizeof(bufVal), "  Valoracion:  %s  " C_GRIS "(%d resenas)\n" RESET, estrellas.c_str(), nResenas);
    vista += bufVal;

    // Stock con colores iguales que pantallaVerProducto

    int stock = getStockProducto(db, p->id, -1);
    char bufStock[128];
    if (stock > 100) snprintf(bufStock, sizeof(bufStock), "  Stock:       " ESTILO_EXITO "%d uds.\n"      RESET, stock);
    else if (stock > 0) snprintf(bufStock, sizeof(bufStock), "  Stock:       " ESTILO_WARN  "%d uds. (bajo)\n" RESET, stock);
    else snprintf(bufStock, sizeof(bufStock), "  Stock:       " ESTILO_ERROR "Sin stock\n"     RESET);
    vista += bufStock;

    // Variantes (solo si hay más de una y no es UNICA)
    bool tieneVariantes = p->categoria.nVariantes > 1 || (p->categoria.nVariantes == 1 && strcmp(p->categoria.variantes[0], "UNICA") != 0);

    if (tieneVariantes) {

        char* sec = getSeccion((char*)"VARIANTES");
        if (sec) { vista += sec; free(sec); }

        for (int i = 0; i < p->categoria.nVariantes; i++) {

            char linea[128];
            snprintf(linea, sizeof(linea), "  " ESTILO_ID "%2d" RESET "  %s\n", i + 1, p->categoria.variantes[i]);
            vista += linea;

        }

    }

    liberarProducto(p);
    free(p);

    if (vista.empty()) {

        responder(ssl, "ERR|Error al crear la vista formateada.");
        LOG_ERROR("Error de creación de vista formateada para el producto #%d", idProd);
        return;

    }

    string res = "OK|" + vista;
    responder(ssl, res);
    LOG_INFO("Se ha enviado el detalle del producto #%d", idProd);

}

void handleAddCarrito(SSL* ssl, sqlite3* db, string args[], Sesion& sesion) {

    int idProd   = stoi(args[1]);
    int cantidad = stoi(args[2]);
    string variante = args[3];

    if (cantidad <= 0) {
        responder(ssl, "ERR|Cantidad no válida.");
        LOG_WARN("Cantidad inválida (%d) para producto #%d", cantidad, idProd);
        return;
    }

    // Verificamos que el producto exista antes de tocar carrito
    Producto* p = getProductoPorId(db, idProd);
    if (!p) {
        responder(ssl, "ERR|El producto no existe en la BD.");
        LOG_WARN("Producto #%d no encontrado", idProd);
        return;
    }

    liberarProducto(p); free(p);

    if (addAlCarrito(db, sesion.getCorreo().c_str(), idProd, variante.c_str(), cantidad) == 0) {

        responder(ssl, "OK");
        LOG_INFO("%s añadió #%d var='%s' x%d", sesion.getCorreo().c_str(), idProd, variante.c_str(), cantidad);

    } else {

        responder(ssl, "ERR|Error de conexión con BD.");
        LOG_ERROR("Error al añadir #%d al carrito de %s", idProd, sesion.getCorreo().c_str());

    }

}

void handleToggleFavorito(SSL* ssl, sqlite3* db, string args[], Sesion& sesion) {

    int idProd = stoi(args[1]);

    Producto* p = getProductoPorId(db, idProd);

    if (!p) {

        responder(ssl, "ERR|El producto no existe en la BD.");
        LOG_WARN("Producto #%d no encontrado", idProd);
        return;

    }

    liberarProducto(p); free(p);

    int resultado = toggleFavorito(db, sesion.getCorreo().c_str(), idProd);

    if (resultado == -1) {

        responder(ssl, "ERR|Error de conexión con BD.");
        LOG_ERROR("Error para %s en producto #%d", sesion.getCorreo().c_str(), idProd);

    } else {

        responder(ssl, "OK|" + to_string(resultado));
        LOG_INFO("%s → #%d %s favoritos", sesion.getCorreo().c_str(), idProd, resultado ? "añadido a" : "eliminado de");

    }

}

static Columna colsResenas[] = {

    { (char*)"   VAL   ",                 9,  1 },
    { (char*)"           USUARIO            ",   30,  2 },
    { (char*)"                   COMENTARIO                   ", 48, 0 },
    { (char*)"    DATE    ",         12,  2 }

};

#define N_COLS_RESENAS 4

void handleGetResenas(SSL* ssl, sqlite3* db, string args[], Sesion& sesion) {

    int idProd = stoi(args[1]);
    int pagina = stoi(args[2]);

    // Verificamos primero que el producto exista
    Producto* p = getProductoPorId(db, idProd);

    if (!p) {

        responder(ssl, "ERR|El producto no existe en la BD.");
        LOG_WARN("Producto #%d no encontrado", idProd);
        return;

    }

    liberarProducto(p); free(p);

    int total = 0;
    Resena* resenas = getResenas(db, idProd, pagina, &total);

    if (total == 0) {

        responder(ssl, "OK|1|");
        LOG_INFO("Sin reseñas para producto #%d", idProd);
        return;

    }

    if (!resenas) {

        responder(ssl, "ERR|Error de conexión con BD.");
        LOG_ERROR("Error de BD al buscar reseñas para #%d", total, idProd);
        return;

    }

    int totalPags = (total + RESENAS_POR_PAGINA - 1) / RESENAS_POR_PAGINA;
    int enEstaPag = total - (pagina - 1) * RESENAS_POR_PAGINA;
    if (enEstaPag > RESENAS_POR_PAGINA) enEstaPag = RESENAS_POR_PAGINA;
    if (enEstaPag < 0) enEstaPag = 0;

    string tabla = "";
    char* cab = getCabeceraTabla(colsResenas, N_COLS_RESENAS);
    if (cab) { tabla += cab; free(cab); }

    for (int i = 0; i < enEstaPag; i++) {

    	Resena* r = &resenas[i];

        string valStr = buildEstrellasStr(r->valoracion);

        char usuario[31]; strncpy(usuario, r->correo, 30); usuario[30] = '\0';
        char coment[49]; strncpy(coment, r->comentario ? r->comentario : "—", 48); coment[48] = '\0';
        char fecha[13]; strncpy(fecha, r->fecha, 12); fecha[12]   = '\0';

        char* fila[] = { (char*)valStr.c_str(), usuario, coment, fecha };
        char* filaStr = getFilaTabla(fila, colsResenas, N_COLS_RESENAS, i % 2);
        if (filaStr) { tabla += filaStr; free(filaStr); }

    }

    char* pie = getPieTabla(colsResenas, N_COLS_RESENAS);
    if (pie) { tabla += pie; free(pie); }

    tabla += getPaginacion(pagina, totalPags, total);

    for (int i = 0; i < enEstaPag; i++) {

        free(resenas[i].correo);
        if (resenas[i].comentario) free(resenas[i].comentario);
        free(resenas[i].fecha);

    }

    free(resenas);

    if (tabla.empty()) {

        responder(ssl, "ERR|Error al crear la tabla formateada.");
        LOG_ERROR("Tabla vacía para producto #%d pag=%d", idProd, pagina);
        return;

    }

    string res = "OK|" + to_string(totalPags) + "|" + (sesion.isAutenticado() && comprobarResena(db, idProd, sesion.getCorreo().c_str())? "1" : "0") + "|" + tabla;
    responder(ssl, res);
    LOG_INFO("Resultados de búsqueda de reseñas: pag=%d/%d total=%d para producto #%d", pagina, totalPags, total, idProd);

}

void handleAddResena(SSL* ssl, sqlite3* db, string args[], Sesion& sesion) {

    int idProd = stoi(args[1]);
    double valoracion = stod(args[2]);
    string comentario = args[3];

    if (valoracion < 0.0 || valoracion > 5.0) {

        responder(ssl, "ERR|Valoracion fuera de rango (1-5).");
        LOG_WARN("Valoracion invalida (%.1f) para #%d por %s", valoracion, idProd, sesion.getCorreo().c_str());
        return;

    }

    Producto* p = getProductoPorId(db, idProd);
    if (!p) {

        responder(ssl, "ERR|El producto no existe en la BD.");
        LOG_WARN("Producto #%d no encontrado", idProd);
        return;

    }

    liberarProducto(p); free(p);

    int res = addResena(db, idProd, sesion.getCorreo().c_str(), valoracion, comentario.c_str());

    if (res == 0) {

        responder(ssl, "OK");
        LOG_INFO("%s → #%d valoracion=%.1f", sesion.getCorreo().c_str(), idProd, valoracion);

    } else {

        responder(ssl, "ERR|Error de conexión con BD.");
        LOG_ERROR("Error al insertar reseña de %s para #%d", sesion.getCorreo().c_str(), idProd);

    }

}

void handleEliminarResena(SSL* ssl, sqlite3* db, string args[], Sesion& sesion) {

    int idProd = stoi(args[1]);

    int res = eliminarResena(db, idProd, sesion.getCorreo().c_str());

    if (res == 0) {

        responder(ssl, "OK");
        LOG_INFO("Reseña eliminada: %s → #%d", sesion.getCorreo().c_str(), idProd);

    } else {

        responder(ssl, "ERR|Error de conexión con BD.");
        LOG_ERROR("Error al eliminar reseña de %s para #%d", sesion.getCorreo().c_str(), idProd);

    }

}
