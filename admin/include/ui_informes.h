#ifndef INCLUDE_UI_INFORMES_H_
#define INCLUDE_UI_INFORMES_H_

#include "sqlite3.h"

// Pantalla principal de informes
void pantallaInformes(sqlite3* db);

// Informe financiero con exportación a Excel
int pantallaBalance(sqlite3* db, char* rutaCsv, char* rutaReports);

// Detección de dead stock con exportación a CSV
int pantallaDeadStock(sqlite3* db, char* rutaReports);

// Ranking histórico de ventas con exportación a CSV
int pantallaTopVentas(sqlite3* db, char* rutaReports);

#endif /* INCLUDE_UI_INFORMES_H_ */
