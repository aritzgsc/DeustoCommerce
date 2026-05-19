#include "auth_db.h"
#include <string.h>
#include <time.h>
#include <stdio.h>

// CONSULTAS

int getUsuarioPorToken(sqlite3* db, const char* token, Usuario* u) {

	sqlite3_stmt* pstmt;

	char* sql = "SELECT CORREO, NOM_U, AP_U, EXP_TOK_U FROM USUARIO "
				"WHERE TOKEN_U = ?";

	if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return 0;

	sqlite3_bind_text(pstmt, 1, token, -1, SQLITE_STATIC);

	int encontrado = 0;			// ERR|Sesión inválida.

	if (sqlite3_step(pstmt) == SQLITE_ROW) {

		strncpy(u->correo, (char*) sqlite3_column_text(pstmt, 0), 127);		u->correo[127] = '\0';
		strncpy(u->nombre, (char*) sqlite3_column_text(pstmt, 1), 63);		u->nombre[63] = '\0';
		strncpy(u->apellido, (char*) sqlite3_column_text(pstmt, 2), 63);	u->apellido[63] = '\0';
		time_t expiracion = sqlite3_column_int64(pstmt, 3);
		if (expiracion > time(NULL)) {
			encontrado = 1;		// OK
		} else {
			encontrado = 2;		// ERR|Sesión expirada.
		}
	}

	sqlite3_finalize(pstmt);
	return encontrado;

}

int getUsuarioPorCorreo(sqlite3* db, const char* correo, Usuario* u) {

	sqlite3_stmt* pstmt;

	char* sql = "SELECT NOM_U, AP_U, CONTR_U FROM USUARIO WHERE CORREO = ?";

	if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return 0;

	sqlite3_bind_text(pstmt, 1, correo, -1, SQLITE_STATIC);

	int encontrado = 0;
	if (sqlite3_step(pstmt) == SQLITE_ROW) {
		strncpy(u->nombre, (char*) sqlite3_column_text(pstmt, 0), 63);		u->nombre[63] = '\0';
		strncpy(u->apellido, (char*) sqlite3_column_text(pstmt, 1), 63);	u->apellido[63] = '\0';
		strncpy(u->contrasenaHash, (char*) sqlite3_column_text(pstmt, 2), 127);	u->contrasenaHash[127] = '\0';
		encontrado = 1;
	}

	sqlite3_finalize(pstmt);
	return encontrado;

}

int existeUsuario(sqlite3* db, const char* correo) {

	sqlite3_stmt* pstmt;

    const char* sql = "SELECT COUNT(*) FROM USUARIO WHERE CORREO = ?";

    int existe = 0;

    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) == SQLITE_OK) {

        sqlite3_bind_text(pstmt, 1, correo, -1, SQLITE_STATIC);

        if (sqlite3_step(pstmt) == SQLITE_ROW) {

            existe = sqlite3_column_int(pstmt, 0);

        }

    }

    sqlite3_finalize(pstmt);

    return existe > 0;

}

// ESCRITURA

int crearUsuario(sqlite3* db, const char* correo, const char* nom, const char* ap, const char* hash) {

	sqlite3_stmt* pstmt;
    const char* sql = "INSERT INTO USUARIO (CORREO, NOM_U, AP_U, CONTR_U) VALUES (?, ?, ?, ?)";

    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return 0;

    sqlite3_bind_text(pstmt, 1, correo, -1, SQLITE_STATIC);
    sqlite3_bind_text(pstmt, 2, nom, -1, SQLITE_STATIC);
    sqlite3_bind_text(pstmt, 3, ap, -1, SQLITE_STATIC);
    sqlite3_bind_text(pstmt, 4, hash, -1, SQLITE_STATIC);

    int res = (sqlite3_step(pstmt) == SQLITE_DONE);
    sqlite3_finalize(pstmt);

    return res;

}

int actualizarToken(sqlite3* db, const char* correo, const char* nuevoToken) {

	sqlite3_stmt* pstmt;

	char* sql = "UPDATE USUARIO SET TOKEN_U = ?, EXP_TOK_U = strftime('%s', 'now') + 604800 "
				"WHERE CORREO = ?";

	if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return 0;

	sqlite3_bind_text(pstmt, 1, nuevoToken, -1, SQLITE_STATIC);
	sqlite3_bind_text(pstmt, 2, correo, -1, SQLITE_STATIC);

	int res = (sqlite3_step(pstmt) == SQLITE_DONE);
	sqlite3_finalize(pstmt);

	return res;

}

int invalidarToken(sqlite3* db, const char* correo) {

	sqlite3_stmt* pstmt;
    const char* sql = "UPDATE USUARIO SET TOKEN_U = NULL, EXP_TOK_U = NULL WHERE CORREO = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return 0;

    sqlite3_bind_text(pstmt, 1, correo, -1, SQLITE_STATIC);

    int res = (sqlite3_step(pstmt) == SQLITE_DONE);
    sqlite3_finalize(pstmt);

    return res;

}
