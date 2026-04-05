#include "resenas_db.h"
#include <stdlib.h>
#include <string.h>

double getValoracionMedia(sqlite3* db, int idProd) {

	if (!db) return -1;

    sqlite3_stmt* pstmt;

    // Media ponderada: SUM(VALORACION * PESO) / SUM(PESO)

    char sql[] =
        "SELECT CAST(SUM(VALORACION * PESO) AS REAL) / SUM(PESO) "
        "FROM RESENA WHERE ID_PR = ? AND PESO > 0";

    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_int(pstmt, 1, idProd);

    double val = -1;
    if (sqlite3_step(pstmt) == SQLITE_ROW && sqlite3_column_type(pstmt, 0) != SQLITE_NULL) val = sqlite3_column_double(pstmt, 0);

    sqlite3_finalize(pstmt);
    return val;

}

int getNumResenas(sqlite3* db, int idProd) {

    if (!db) return 0;

    sqlite3_stmt* pstmt;
    char sql[] = "SELECT SUM(PESO) FROM RESENA WHERE ID_PR = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK)
        return 0;

    sqlite3_bind_int(pstmt, 1, idProd);

    int n = 0;
    if (sqlite3_step(pstmt) == SQLITE_ROW && sqlite3_column_type(pstmt, 0) != SQLITE_NULL) n = sqlite3_column_int(pstmt, 0);

    sqlite3_finalize(pstmt);
    return n;

}
