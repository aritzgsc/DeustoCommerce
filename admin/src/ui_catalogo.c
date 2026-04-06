#include "ui_admin.h"
#include "ui_catalogo.h"
#include "ui_utils.h"
#include "resenas_db.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// AUXILIARES INTERNAS

// Imprime estrellas de valoración: ★★★★☆ (4/5)

static void imprimirEstrellas(double val) {

    if (val < 0) {
        printf(C_GRIS "Sin valoración" RESET);
        return;
    }

    int llenas  = (int)round(val / 5.0 * 5);
    int vacias  = 5 - llenas;
    printf(C_AMARILLO);

    for (int i = 0; i < llenas; i++)  printf("★");

    printf(C_GRIS);

    for (int i = 0; i < vacias; i++)  printf("☆");

    printf(RESET " " C_GRIS "%.1f" RESET, val);

}

// Imprime el precio con descuento aplicado si lo hay

static void imprimirPrecio(double precio, double descuento) {

    if (descuento > 0) {

        double precioFinal = precio * (1.0 - descuento);
        printf(ESTILO_PRECIO "%.2f€" RESET, precioFinal);
        printf(C_GRIS " (-%d%%)" RESET, (int)(descuento * 100));

    } else {

        printf(ESTILO_PRECIO "%.2f€" RESET, precio);

    }

}

// Columnas del listado de catálogo

static Columna colsCatalogo[] = {
    { "  ID  ", 6,  1 },
    { "               NOMBRE               ", 36, 0 },
    { "       CATEGORIA       ", 23, 0 },
    { "  PRECIO (€)  ", 14, 1 },
    { " STOCK ", 7, 1 },
};

#define N_COLS_CATALOGO 5

// Imprime el listado de productos en tabla

static void imprimirListadoProductos(sqlite3* db, Producto* prods, int n) {

    imprimirCabeceraTabla(colsCatalogo, N_COLS_CATALOGO);

    for (int i = 0; i < n; i++) {

        Producto* p = &prods[i];

        // Truncamos nombre y categoria si es muy largo
        char nombre[37];
        strncpy(nombre, p->nombre, 36);
        nombre[36] = '\0';

        char categoria[24];
        strncpy(categoria, p->categoria.nombre, 23);
        categoria[23] = '\0';

        char idStr[8], precioStr[15], stockStr[8];
        snprintf(idStr, sizeof(idStr), "%d", p->id);
        snprintf(stockStr, sizeof(stockStr), "%d", getStockProducto(db, p->id, -1));

        // Precio con descuento

        if (p->descuento > 0) snprintf(precioStr, sizeof(precioStr), "%.2f € -%d%%", p->precio * (1.0 - p->descuento), (int)(p->descuento * 100));
        else snprintf(precioStr, sizeof(precioStr), "%.2f €", p->precio);

        char* fila[] = { idStr, nombre, categoria, precioStr, stockStr };

        imprimirFilaTabla(fila, colsCatalogo, N_COLS_CATALOGO, i % 2);

    }

    imprimirPieTabla(colsCatalogo, N_COLS_CATALOGO);
}

// SELECCIONAR PRODUCTO

static Columna colsSelProd[] = {
	{ "  ID  ", 6,  1 },
	{ "               NOMBRE               ", 36, 0 },
	{ "       CATEGORIA       ", 23, 0 },
	{ "  PRECIO (€)  ", 14, 1 },
};
#define N_COLS_SEL_PROD 4

static char* cmdsSelProd[] = {

    "SELECCIONAR", "NOMBRE", "CATEGORIA", "PRECIO", "BORRAR", "ANTERIOR", "SIGUIENTE", "VOLVER"

};

#define N_CMDS_SEL_PROD 9

int seleccionarProducto(sqlite3* db, char* varianteSalida, int maxLen, int idAlm) {

    FiltrosProducto f = filtrosVacios();
    f.idAlm = idAlm;
    int pagina  = 1;
    int total   = 0;
    Producto* prods = NULL;

    while (!salir) {

    	prods = buscarProductos(db, f, pagina, &total);

        int totalPags = total > 0 ? (total + ITEMS_POR_PAGINA - 1) / ITEMS_POR_PAGINA : 1;

        char subtitulo[64];
        if (idAlm != -1) snprintf(subtitulo, sizeof(subtitulo), "Productos disponibles en almacen #%d", idAlm);
        else snprintf(subtitulo, sizeof(subtitulo), "Busca y selecciona un producto");
        imprimirCabecera("SELECCIONAR PRODUCTO", subtitulo);

        // Filtros activos
        imprimirSeccion("FILTROS");
        printf("  " ESTILO_SUBTITULO "Nombre:    " RESET);
        if (strlen(f.nombre) > 0) printf(C_BLANCO "%s\n" RESET, f.nombre);
        else printf(C_GRIS "—\n" RESET);

        printf("  " ESTILO_SUBTITULO "Categoria: " RESET);
        if (f.idCategoria != -1) printf(C_BLANCO "ID %d\n" RESET, f.idCategoria);
        else printf(C_GRIS "—\n" RESET);

        printf("  " ESTILO_SUBTITULO "Precio:    " RESET);
        if (f.precioMin >= 0 || f.precioMax >= 0) printf(C_BLANCO "%.2f — %.2f €\n" RESET, f.precioMin >= 0 ? f.precioMin : 0, f.precioMax >= 0 ? f.precioMax : 99999);
        else printf(C_GRIS "—\n" RESET);

        imprimirSeccion("RESULTADOS");

        if (!prods || total == 0) {

            imprimirWarn("Sin resultados.");

        } else {

            imprimirCabeceraTabla(colsSelProd, N_COLS_SEL_PROD);

            int enEstaPagina = total - (pagina - 1) * ITEMS_POR_PAGINA;
            if (enEstaPagina > ITEMS_POR_PAGINA) enEstaPagina = ITEMS_POR_PAGINA;
            if (enEstaPagina < 0) enEstaPagina = 0;

            for (int i = 0; i < enEstaPagina; i++) {

                Producto* p = &prods[i];

                char idStr[8], precioStr[16], nombre[35], cat[21];

                snprintf(idStr, sizeof(idStr), "%d", p->id);
                strncpy(nombre, p->nombre, 34); nombre[34] = '\0';
                strncpy(cat, p->categoria.nombre, 20); cat[20] = '\0';

                if (p->descuento > 0) snprintf(precioStr, sizeof(precioStr), "%.2f € -%d%%", p->precio * (1.0 - p->descuento), (int)(p->descuento * 100));
                else snprintf(precioStr, sizeof(precioStr) + 2, "%.2f €", p->precio);

                char* fila[] = { idStr, nombre, cat, precioStr };
                imprimirFilaTabla(fila, colsSelProd, N_COLS_SEL_PROD, i % 2);

            }

            imprimirPieTabla(colsSelProd, N_COLS_SEL_PROD);
            imprimirPaginacion(pagina, totalPags, total);

        }

        imprimirSeccion("COMANDOS");
        printf("  " ESTILO_CMD "SELECCIONAR [ID]       " RESET "Elegir producto\n");
        printf("  " ESTILO_CMD "NOMBRE [texto]         " RESET "Filtrar por nombre\n");
        printf("  " ESTILO_CMD "CATEGORIA              " RESET "Filtrar por categoria\n");
        printf("  " ESTILO_CMD "PRECIO [min] [max]     " RESET "Filtrar por precio\n");
        printf("  " ESTILO_CMD "BORRAR                 " RESET "Limpiar filtros\n");
        if (prods && total > 0) printf("  " ESTILO_CMD "ANTERIOR / SIGUIENTE   " RESET "Navegar paginas\n");
        printf("  " ESTILO_CMD "VOLVER                 " RESET "Cancelar\n\n");

        Entrada e = leerComando(cmdsSelProd, N_CMDS_SEL_PROD, ">");

        if (strcmp(e.comando, "SELECCIONAR") == 0 && strlen(e.arg1) > 0) {

        	int idSel = atoi(e.arg1);

            // Verificamos que existe
            Producto* p = getProductoPorId(db, idSel);
            if (!p) {

                imprimirError("ID de producto no valido.");
                pausar();
                continue;

            }

            if (idAlm > 0 && !isProductoInAlmacen(db, idSel, idAlm)) {

            	imprimirError("El producto no está en el almacén.");
            	pausar();
            	continue;

            }

            char displayNom[1024] = "";
            wordWrap(displayNom, p->nombre, 16);
            char displayCat[1024] = "";
            wordWrap(displayCat, p->categoria.nombre, 16);

            // Mostramos info del producto seleccionado
            printf("\n  " ESTILO_SUBTITULO "Seleccionado: " RESET  C_BLANCO "#%d — %s\n" RESET, p->id, displayNom);
            printf("  " ESTILO_SUBTITULO "Categoria:    " RESET C_GRIS "%s\n" RESET, displayCat);

            // Elegimos variante aquí mismo
            if (varianteSalida && maxLen > 0) {
            	filtrarVariantesConStockEnAlm(db, p, idAlm);
            	seleccionarVariante(&p->categoria, varianteSalida, maxLen);
            }

            liberarProducto(p);

            if (prods) free(prods);
            return idSel;

        } else if (strcmp(e.comando, "NOMBRE") == 0) {

            if (strlen(e.arg1) > 0) strncpy(f.nombre, e.arg1, sizeof(f.nombre) - 1);
            else leerTexto("Nombre:", f.nombre, sizeof(f.nombre));
            if (prods) free(prods);

        } else if (strcmp(e.comando, "CATEGORIA") == 0) {

            f.idCategoria = seleccionarCategoria(db);
            if (prods) free(prods);

        } else if (strcmp(e.comando, "PRECIO") == 0) {

            if (strlen(e.arg1) > 0) f.precioMin = atof(e.arg1);
            if (strlen(e.arg2) > 0) f.precioMax = atof(e.arg2);
            if (f.precioMin < 0 && f.precioMax < 0) {
                f.precioMin = leerDouble("Precio minimo (0 = sin limite):", 0, 999999);
                f.precioMax = leerDouble("Precio maximo (0 = sin limite):", 0, 999999);
                if (f.precioMin == 0) f.precioMin = -1;
                if (f.precioMax == 0) f.precioMax = -1;
            }

            if (prods) free(prods);

        } else if (strcmp(e.comando, "BORRAR") == 0) {

            f = filtrosVacios();
            pagina = 1;
            if (prods) free(prods);

        } else if (strcmp(e.comando, "SIGUIENTE") == 0) {

            if (pagina < totalPags) {
                pagina++;
                if (prods) free(prods);
            } else {
                pagina = 1;
                if (prods) free(prods);
            }

        } else if (strcmp(e.comando, "ANTERIOR") == 0) {

            if (pagina > 1) {
                pagina--;
            } else {
                int tp = total > 0 ? (total + ITEMS_POR_PAGINA - 1) / ITEMS_POR_PAGINA : 1;
                pagina = tp;
            }
            if (prods) free(prods);

        } else if (strcmp(e.comando, "VOLVER") == 0) {

            if (prods) free(prods);
            return -1;

        } else {

            imprimirError("Comando no reconocido.");
            pausar();

        }
    }

    return -1;

}

// SELECCIONAR CATEGORIA

// Columnas para la tabla de categorías

static Columna colsCategorias[] = {

    { "  ID  ",         6,  1 },
    { "               NOMBRE               ",    36,  0 },

};

#define N_COLS_CAT 2

static char* cmdsCats[] = {

    "SELECCIONAR", "ANTERIOR", "SIGUIENTE", "BUSCAR", "VOLVER"

};

#define N_CMDS_CATS 5

// Selector de categoría con paginación y búsqueda
// Devuelve el ID seleccionado o -1 si cancela

int seleccionarCategoria(sqlite3* db) {

    int nCats = 0;
    Categoria* cats = getCategorias(db, &nCats);
    if (!cats || nCats == 0) {
        imprimirError("No hay categorias disponibles.");
        pausar();
        return -1;
    }

    // Array filtrado (apunta a elementos de cats)
    Categoria** filtradas = malloc(sizeof(Categoria*) * nCats);
    int nFiltradas = nCats;
    char filtro[64] = {0};

    for (int i = 0; i < nCats; i++) filtradas[i] = &cats[i];

    int pagina = 1;

    while (!salir) {

        int totalPags = nFiltradas > 0 ? (nFiltradas + CATS_POR_PAGINA - 1) / CATS_POR_PAGINA : 1;

        imprimirCabecera("SELECCIONAR CATEGORIA", "Elige la categoria del producto");

        if (strlen(filtro) > 0) printf("  " ESTILO_HINT "Filtro activo: " RESET C_CYAN "\"%s\"\n\n" RESET, filtro);

        imprimirCabeceraTabla(colsCategorias, N_COLS_CAT);

        int inicio = (pagina - 1) * CATS_POR_PAGINA;
        int fin    = inicio + CATS_POR_PAGINA < nFiltradas ? inicio + CATS_POR_PAGINA : nFiltradas;

        for (int i = inicio; i < fin; i++) {
            char idStr[8], nombre[36];
            snprintf(idStr, sizeof(idStr), "%d", filtradas[i]->id);
            strncpy(nombre, filtradas[i]->nombre, 35);
            nombre[35] = '\0';

            char* fila[] = { idStr, nombre };
            imprimirFilaTabla(fila, colsCategorias, N_COLS_CAT, i % 2);
        }

        imprimirPieTabla(colsCategorias, N_COLS_CAT);
        imprimirPaginacion(pagina, totalPags, nFiltradas);

        imprimirSeccion("COMANDOS");
        printf("  " ESTILO_CMD "SELECCIONAR [ID]       " RESET "Elegir categoria\n");
        printf("  " ESTILO_CMD "BUSCAR [texto]         " RESET "Filtrar por nombre\n");
        printf("  " ESTILO_CMD "ANTERIOR / SIGUIENTE   " RESET "Navegar paginas\n");
        printf("  " ESTILO_CMD "VOLVER                 " RESET "Cancelar\n\n");

        Entrada e = leerComando(cmdsCats, N_CMDS_CATS, ">");

        if (strcmp(e.comando, "SELECCIONAR") == 0 && strlen(e.arg1) > 0) {

            int idSel = atoi(e.arg1);

            // Verificamos que el ID existe en el array filtrado
            for (int i = 0; i < nFiltradas; i++) {
                if (filtradas[i]->id == idSel) {
                    free(filtradas);
                    for (int j = 0; j < nCats; j++) free(cats[j].nombre);
                    free(cats);
                    return idSel;
                }
            }
            imprimirError("ID de categoria no valido.");
            pausar();

        } else if (strcmp(e.comando, "BUSCAR") == 0) {

            // Recogemos el texto del filtro
            if (strlen(e.arg1) > 0) {
                strncpy(filtro, e.arg1, sizeof(filtro) - 1);
            } else {
                leerTexto("Texto a buscar:", filtro, sizeof(filtro));
            }

            // Refiltrar
            nFiltradas = 0;
            for (int i = 0; i < nCats; i++) {
                if (strlen(filtro) == 0 ||
                    _strnicmp(cats[i].nombre, filtro, strlen(filtro)) == 0 ||
                    strstr(cats[i].nombre, filtro) != NULL) {
                    filtradas[nFiltradas++] = &cats[i];
                }
            }
            pagina = 1;

            if (nFiltradas == 0) {
                imprimirWarn("Sin resultados para ese filtro.");
                memset(filtro, 0, sizeof(filtro));
                nFiltradas = nCats;
                for (int i = 0; i < nCats; i++) filtradas[i] = &cats[i];
                pausar();
            }

        } else if (strcmp(e.comando, "SIGUIENTE") == 0) {

            if (pagina < totalPags) pagina++;
            else pagina = 1;

        } else if (strcmp(e.comando, "ANTERIOR") == 0) {

            if (pagina > 1) pagina--;
            else pagina = totalPags;

        } else if (strcmp(e.comando, "VOLVER") == 0) {

            free(filtradas);
            for (int i = 0; i < nCats; i++) free(cats[i].nombre);
            free(cats);
            return -1;

        } else {

            imprimirError("Comando no reconocido.");
            pausar();

        }

    }

    return -1;

}

// SELECCIONAR VARIANTE

int seleccionarVariante(Categoria* cat, char* varianteSalida, int maxLen) {

    if (!cat || cat->nVariantes == 0 || (cat->nVariantes == 1 && strcmp(cat->variantes[0], "UNICA") == 0)) {

        strncpy(varianteSalida, "UNICA", maxLen - 1);
        return 0;

    } else if (cat->nVariantes == 1) {

    	strncpy(varianteSalida, cat->variantes[0], maxLen - 1);
    	return 0;

    }

    imprimirSeccion("VARIANTES DISPONIBLES");

    for (int i = 0; i < cat->nVariantes; i++) printf("  " ESTILO_ID "%2d" RESET "  %s\n", i + 1, cat->variantes[i]);

    printf("\n");

    int sel = leerEntero("Numero de variante:", 1, cat->nVariantes);

    strncpy(varianteSalida, cat->variantes[sel - 1], maxLen - 1);

    return sel;

}

// CATÁLOGO

// Comandos disponibles en el catálogo
static char* cmdsCatalogo[] = {

    "VER_PROD", "NUEVO_PROD", "BUSCAR", "ANTERIOR", "SIGUIENTE", "HOME", "EXIT"

};

#define N_CMDS_CATALOGO 7

void pantallaCatalogo(sqlite3* db) {

    int pagina = 1;
    int total  = 0;
    FiltrosProducto f = filtrosVacios();

    while (!salir) {

        Producto* prods = buscarProductos(db, f, pagina, &total);
        int totalPags = total > 0 ? (total + ITEMS_POR_PAGINA - 1) / ITEMS_POR_PAGINA : 1;

        int enEstaPagina = total - (pagina - 1) * ITEMS_POR_PAGINA;
        if (enEstaPagina > ITEMS_POR_PAGINA) enEstaPagina = ITEMS_POR_PAGINA;
        if (enEstaPagina < 0) enEstaPagina = 0;

        imprimirCabecera("CATÁLOGO DE PRODUCTOS", "Listado completo de productos");

        if (!prods || total == 0) {

            imprimirWarn("No hay productos en el catálogo.");

        } else {

            imprimirListadoProductos(db, prods, enEstaPagina);
            imprimirPaginacion(pagina, totalPags, total);
            free(prods);

        }

        imprimirSeccion("COMANDOS");
        printf("  " ESTILO_CMD "VER_PROD [ID]          " RESET "Ver detalle de producto\n");
        printf("  " ESTILO_CMD "NUEVO_PROD             " RESET "Dar de alta un producto\n");
        printf("  " ESTILO_CMD "BUSCAR                 " RESET "Filtrar productos\n");
        printf("  " ESTILO_CMD "ANTERIOR / SIGUIENTE   " RESET "Navegar páginas\n");
        printf("  " ESTILO_CMD "HOME                   " RESET "Volver al panel\n\n");

        Entrada e = leerComando(cmdsCatalogo, N_CMDS_CATALOGO, ">");

        if (strcmp(e.comando, "VER_PROD") == 0 && strlen(e.arg1) > 0) {

            int home = pantallaVerProducto(db, atoi(e.arg1), -1);
            if (home) return;

        } else if (strcmp(e.comando, "NUEVO_PROD") == 0) {

            pantallaNuevoProducto(db);

        } else if (strcmp(e.comando, "BUSCAR") == 0) {

            pantallaBuscar(db);
            return; // volvemos al home tras buscar

        } else if (strcmp(e.comando, "SIGUIENTE") == 0) {

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

            imprimirError("Comando no reconocido.");
            pausar();

        }

    }

}

// BUSCAR

static char* cmdsBuscarConResultados[] = {

    "NOMBRE", "CATEGORIA", "PRECIO", "BORRAR", "VER_PROD", "ANTERIOR", "SIGUIENTE", "HOME", "EXIT"

};

#define N_CMDS_BUSCAR_RESULTADOS 9

void pantallaBuscar(sqlite3* db) {

    FiltrosProducto f = filtrosVacios();
    int pagina  = 1;
    int total   = 0;
    Producto* prods = NULL;

    while (!salir) {

    	prods  = buscarProductos(db, f, pagina, &total);

        imprimirCabecera("BUSCAR PRODUCTOS", "Añade filtros y ejecuta la busqueda");

        imprimirSeccion("FILTROS ACTIVOS");

        printf("  " ESTILO_SUBTITULO "Nombre:    " RESET);
        if (strlen(f.nombre) > 0) printf(C_BLANCO "%s\n" RESET, f.nombre);
        else printf(C_GRIS "—\n" RESET);

        printf("  " ESTILO_SUBTITULO "Categoria: " RESET);
        if (f.idCategoria != -1) printf(C_BLANCO "ID %d\n" RESET, f.idCategoria);
        else printf(C_GRIS "—\n" RESET);

        printf("  " ESTILO_SUBTITULO "Precio:    " RESET);
        if (f.precioMin >= 0 || f.precioMax >= 0) {

            printf(C_BLANCO "%.2f — %.2f €\n" RESET, f.precioMin >= 0 ? f.precioMin : 0, f.precioMax >= 0 ? f.precioMax : 99999);

        } else {

            printf(C_GRIS "—\n" RESET);

        }

        int totalPags = total > 0 ? (total + ITEMS_POR_PAGINA - 1) / ITEMS_POR_PAGINA : 1;

        int enEstaPagina = total - (pagina - 1) * ITEMS_POR_PAGINA;
        if (enEstaPagina > ITEMS_POR_PAGINA) enEstaPagina = ITEMS_POR_PAGINA;
        if (enEstaPagina < 0) enEstaPagina = 0;

        imprimirSeccion("RESULTADOS");
        if (!prods || total == 0) {

            imprimirWarn("No se encontraron productos con esos filtros.");

        } else {

            imprimirListadoProductos(db, prods, enEstaPagina);
            imprimirPaginacion(pagina, totalPags, total);

        }

        imprimirSeccion("COMANDOS");
        printf("  " ESTILO_CMD "VER_PROD [ID]          " RESET "Ver detalle de producto\n");
        printf("  " ESTILO_CMD "NOMBRE [texto]         " RESET "Filtrar por nombre\n");
        printf("  " ESTILO_CMD "CATEGORIA [id]         " RESET "Filtrar por categoria\n");
        printf("  " ESTILO_CMD "PRECIO [min] [max]     " RESET "Filtrar por rango de precio\n");
        printf("  " ESTILO_CMD "BORRAR                 " RESET "Limpiar filtros\n");
        if (prods && total > 0) {
            printf("  " ESTILO_CMD "ANTERIOR / SIGUIENTE   " RESET "Navegar paginas\n");
        }
        printf("  " ESTILO_CMD "HOME                   " RESET "Volver al panel\n\n");

        // Usamos el array de comandos adecuado según estado
        Entrada e = leerComando(cmdsBuscarConResultados, N_CMDS_BUSCAR_RESULTADOS, ">");

        if (strcmp(e.comando, "NOMBRE") == 0) {

            if (strlen(e.arg1) > 0) strncpy(f.nombre, e.arg1, sizeof(f.nombre) - 1);
            else leerTexto("Nombre a buscar:", f.nombre, sizeof(f.nombre));
            if (prods) free(prods);

        } else if (strcmp(e.comando, "CATEGORIA") == 0) {

            if (strlen(e.arg1) > 0) f.idCategoria = atoi(e.arg1);
            else f.idCategoria = seleccionarCategoria(db);
            if (prods) free(prods);

        } else if (strcmp(e.comando, "PRECIO") == 0) {

            if (strlen(e.arg1) > 0) f.precioMin = atof(e.arg1);
            if (strlen(e.arg2) > 0) f.precioMax = atof(e.arg2);
            if (f.precioMin < 0 && f.precioMax < 0) {
                f.precioMin = leerDouble("Precio minimo (0 = sin limite):", 0, 999999);
                f.precioMax = leerDouble("Precio maximo (0 = sin limite):", 0, 999999);
                if (f.precioMin == 0) f.precioMin = -1;
                if (f.precioMax == 0) f.precioMax = -1;
            }
            if (prods) free(prods);

        } else if (strcmp(e.comando, "BORRAR") == 0) {

            f = filtrosVacios();
            pagina  = 1;
            if (prods) free(prods);
            imprimirExito("Filtros eliminados.");

        } else if (strcmp(e.comando, "VER_PROD") == 0 && strlen(e.arg1) > 0) {

            int home = pantallaVerProducto(db, atoi(e.arg1), -1);
            if (home) return;

        } else if (strcmp(e.comando, "SIGUIENTE") == 0) {

            int totalPags = total > 0? (total + ITEMS_POR_PAGINA - 1) / ITEMS_POR_PAGINA : 1;
            if (pagina < totalPags) {

                pagina++;
                if (prods) free(prods);

            } else {

                pagina = 1;
                if (prods) free(prods);

            }

        } else if (strcmp(e.comando, "ANTERIOR") == 0) {

            if (pagina > 1) {

                pagina--;
                if (prods) free(prods);

            } else {

            	int totalPags = total > 0? (total + ITEMS_POR_PAGINA - 1) / ITEMS_POR_PAGINA : 1;
            	pagina = totalPags;
            	if (prods) free(prods);

            }

        } else if (strcmp(e.comando, "HOME") == 0) {

            if (prods) free(prods);
            return;

        } else if (strcmp(e.comando, "EXIT") == 0) {

            if (prods) free(prods);
            salir = 1;
            return;

        } else {

            imprimirError("Comando no reconocido.");
            pausar();

        }

    }

}

// VER PRODUCTO

// Comandos disponibles en ver producto
static char* cmdsVerProd[] = {

    "EDITAR", "ELIMINAR", "VOLVER", "HOME", "EXIT"

};

#define N_CMDS_VER_PROD 5

int pantallaVerProducto(sqlite3* db, int idProd, int idAlm) {

    Producto* p = getProductoPorId(db, idProd);

    if (!p) {

        imprimirError("Producto no encontrado.");
        pausar();
        return 0;

    }

    if (idAlm > 0 && !isProductoInAlmacen(db, idProd, idAlm)) {

    	imprimirError("El producto no está en el almacén.");
    	pausar();
    	return 0;

    }

    while (!salir) {

        char subtitulo[64];
        snprintf(subtitulo, sizeof(subtitulo), idAlm > 0 ? "Detalle - Almacén #%d" : "Detalle - Todos los almacenes", idAlm);
        imprimirCabecera("DETALLE DE PRODUCTO", subtitulo);

        // Nombre y descripción con un wrapper para que no se salga de la consola

        char nombre[1024] = "";
        wordWrap(nombre, p->nombre, 6);

        // Info principal

        printf("  " ESTILO_ID "#%d" RESET "  " NEGRITA C_BLANCO "%s\n\n" RESET, p->id, nombre);
        printf("  " ESTILO_HINT "%s\n\n" RESET, p->categoria.nombre);

        if (p->descripcion && strlen(p->descripcion) > 0) {

        	// Hacemos lo mismo que con el nombre por si la desc es muy larga

        	char descripcion[1024] = "";
        	wordWrap(descripcion, p->descripcion, 2);
        	printf("  %s\n\n", descripcion);

        }

        // Precio y descuento
        printf("  Precio:      ");
        imprimirPrecio(p->precio, p->descuento);
        printf("\n");

        // Valoración
        double val = getValoracionMedia(db, p->id);
        int num = getNumResenas(db, p->id);
        printf("  Valoración:  ");
        imprimirEstrellas(val);
        printf("  " C_GRIS "(%d reseñas)\n" RESET, num);

        // Stock
        int stock = getStockProducto(db, p->id, idAlm);
        printf("  Stock:       ");
        if (stock > 100) printf(ESTILO_EXITO "%d uds.\n" RESET, stock);
        else if (stock > 0) printf(ESTILO_WARN  "%d uds. (bajo)\n" RESET, stock);
        else printf(ESTILO_ERROR  "Sin stock\n" RESET);

        imprimirSeccion("COMANDOS");
        printf("  " ESTILO_CMD "EDITAR     " RESET "Modificar campos\n");
        printf("  " ESTILO_CMD "ELIMINAR   " RESET "Borrar producto\n");
        printf("  " ESTILO_CMD "VOLVER     " RESET "Volver al listado\n\n");

        Entrada e = leerComando(cmdsVerProd, N_CMDS_VER_PROD, ">");

        if (strcmp(e.comando, "EDITAR") == 0) {

            pantallaEditarProducto(db, idProd);
            // Recargamos el producto tras editar
            liberarProducto(p);
            p = getProductoPorId(db, idProd);
            if (!p) return 0;

        } else if (strcmp(e.comando, "ELIMINAR") == 0) {

            pantallaEliminarProducto(db, idProd, idAlm);
            liberarProducto(p);
            return 0;

        } else if (strcmp(e.comando, "VOLVER") == 0) {

            break;

        } else if (strcmp(e.comando, "HOME") == 0) {

        	liberarProducto(p);
        	return 1;

    	} else if (strcmp(e.comando, "EXIT") == 0){

    		liberarProducto(p);
        	salir = 1;
        	return 1;

        } else {

            imprimirError("Comando no reconocido.");
            pausar();

        }
    }

    liberarProducto(p);

    return 0;

}

// NUEVO PRODUCTO

void pantallaNuevoProducto(sqlite3* db) {

    imprimirCabecera("NUEVO PRODUCTO", "Introduce los datos del nuevo producto");

    Producto p = {0};
    char nombre[256] = {0};
    char descripcion[1024] = {0};

    // Nombre
    leerTexto("Nombre:", nombre, sizeof(nombre));
    if (strlen(nombre) == 0) {
        imprimirError("El nombre no puede estar vacio.");
        pausar();
        return;
    }
    p.nombre = nombre;

    // Descripcion
    leerTexto("Descripcion (opcional, ENTER para omitir):", descripcion, sizeof(descripcion));
    p.descripcion = strlen(descripcion) > 0 ? descripcion : NULL;

    // Precio
    p.precio = leerDouble("Precio (€):", 0.01, 999999);

    // Descuento
    printf("  " ESTILO_HINT "Introduce el descuento como decimal: 0.10 = 10%%, 0 = sin descuento\n" RESET);
    p.descuento = leerDouble("Descuento (0.0 - 0.90):", 0, 0.90);

    // Categoria con selector paginado
    imprimirInfo("Selecciona la categoria del producto.");
    pausar();

    int idCat = seleccionarCategoria(db);
    if (idCat == -1) {
        imprimirWarn("Creacion cancelada.");
        pausar();
        return;
    }
    p.categoria.id = idCat;

    // Resumen antes de confirmar
    imprimirCabecera("NUEVO PRODUCTO", "Confirmar datos");

    imprimirSeccion("RESUMEN");
    printf("  " ESTILO_SUBTITULO "Nombre:      " RESET C_BLANCO "%s\n"   RESET, p.nombre);
    printf("  " ESTILO_SUBTITULO "Descripcion: " RESET C_GRIS   "%s\n"   RESET,
           p.descripcion ? p.descripcion : "—");
    printf("  " ESTILO_SUBTITULO "Precio:      " RESET ESTILO_PRECIO "%.2f €\n" RESET, p.precio);
    printf("  " ESTILO_SUBTITULO "Descuento:   " RESET C_AMARILLO "%d%%\n" RESET,
           (int)(p.descuento * 100));
    printf("  " ESTILO_SUBTITULO "Categoria:   " RESET C_BLANCO "ID #%d\n" RESET, p.categoria.id);
    printf("\n");

    if (confirmar("Confirmar creacion del producto?")) {

        int id = crearProducto(db, p);
        if (id > 0) {
            char msg[64];
            snprintf(msg, sizeof(msg), "Producto creado con ID #%d", id);
            LOG_INFO("Producto creado con éxito: #%d — %s", id, nombre);
            imprimirExito(msg);
        } else {
            imprimirError("Error al crear el producto.");
            LOG_ERROR("Error al crear producto: #%d — %s", id, nombre);
        }
        pausar();

    } else {

        imprimirWarn("Creación del producto cancelada");
        LOG_WARN("Cancelación de producto cancelada");
        pausar();

    }
}

// EDITAR PRODUCTO

void pantallaEditarProducto(sqlite3* db, int idProd) {

    Producto* p = getProductoPorId(db, idProd);
    if (!p) { imprimirError("Producto no encontrado."); pausar(); return; }

    char subtitulo[72];
    strncpy(subtitulo, p->nombre, 71);
    subtitulo[71] = '\0';

    imprimirCabecera("EDITAR PRODUCTO", subtitulo);

    char nombre[512], descripcion[1024];

    char displayNom[1024];
    wordWrap(displayNom, p->nombre, 2);

    char displayDesc[1024];
    p->descripcion ? wordWrap(displayDesc, p->descripcion, 2) : strcpy(displayDesc, "—");

    printf(C_GRIS "  Nombre actual: %s\n\n" RESET, displayNom);
    leerTexto("Nuevo nombre (ENTER para mantener):", nombre, sizeof(nombre));
    if (strlen(nombre) == 0) strncpy(nombre, p->nombre, sizeof(nombre));

    printf(C_GRIS "  Descripción actual: %s\n" RESET, displayDesc);
    leerTexto("Nueva descripción (ENTER para mantener):", descripcion, sizeof(descripcion));
    if (strlen(descripcion) == 0 && p->descripcion) strncpy(descripcion, p->descripcion, sizeof(descripcion));

    printf(C_GRIS "  Precio actual: %.2f€\n" RESET, p->precio);
    double precio = leerDouble("Nuevo precio (0 para mantener):", 0, 999999);
    if (precio == 0) precio = p->precio;

    printf(C_GRIS "  Descuento actual: %d%%\n" RESET, (int)(p->descuento * 100));
    double descuento = leerDouble("Nuevo descuento (0.0-0.9, -1 para mantener):", -1, 0.9);
    if (descuento < 0) descuento = p->descuento;

    // Construimos el producto actualizado
    Producto actualizado = *p;
    actualizado.nombre = nombre;
    actualizado.descripcion = strlen(descripcion) > 0 ? descripcion : NULL;
    actualizado.precio = precio;
    actualizado.descuento = descuento;

    if (confirmar("¿Confirmar cambios?")) {

    	if (editarProducto(db, actualizado) == 0) {
    		imprimirExito("Producto actualizado correctamente.");
    		LOG_INFO("Producto editado: #%d — %s", idProd, nombre);
    	}
        else {
        	imprimirError("Error al actualizar el producto.");
        	LOG_ERROR("Edición de producto errónea: #%d — %s", idProd, nombre);
        }

        pausar();

    } else {

    	imprimirWarn("Edición del producto cancelada");
    	LOG_WARN("Edición de producto cancelada: #%d — %s", idProd, nombre);
    	pausar();

    }

    liberarProducto(p);

}

// ELIMINAR PRODUCTO

void pantallaEliminarProducto(sqlite3* db, int idProd, int idAlm) {

    Producto* p = getProductoPorId(db, idProd);

    if (!p) { imprimirError("Producto no encontrado."); pausar(); return; }

    // Truncamos nombre si es muy largo
    char nombre[72];
    strncpy(nombre, p->nombre, 71);
    nombre[71] = '\0';

    imprimirCabecera("ELIMINAR PRODUCTO", nombre);

    if (idAlm > 0) {

        printf("  " ESTILO_WARN "Esta acción eliminará el producto solo del almacén #%d.\n" RESET, idAlm);

    } else {

        printf("  " ESTILO_ERROR "Esta acción eliminará el producto de TODOS los almacenes.\n" RESET);

    }


    char displayNom[1024] = "";
    wordWrap(displayNom, p->nombre, 2);
    printf("  " ESTILO_ID "Producto: #%d — %s\n\n" RESET, p->id, displayNom);

    if (confirmar("¿Confirmar eliminación?")) {

        int res = eliminarProducto(db, idProd, idAlm);

        if (res == 0) {
        	imprimirExito("Producto eliminado correctamente.");
        	LOG_INFO("Producto eliminado correctamente: #%d — %s", idProd, nombre);
        }
        else {
        	imprimirError("Error al eliminar el producto.");
        	LOG_ERROR("Error al eliminar el producto: #%d — %s", idProd, nombre);
        }

        pausar();

    } else {

    	imprimirWarn("Eliminación del producto cancelada");
    	LOG_WARN("Eliminación del producto cancelada: #%d — %s", idProd, nombre);
    	pausar();

    }

    liberarProducto(p);

}
