#ifndef DAEMON_H
#define DAEMON_H

#include <string>
#include <thread>
#include <windows.h>
#include <time.h>

extern "C" {
	#include "sqlite3.h"
}

class Daemon {
private:

    bool ejecutando;
    std::thread hiloCron;
    std::thread hiloCola;

    sqlite3* db;

    int ultimoDiaRestock;
    int ultimoMesBalance;

    void rutinaTareasPeriodicas();
    void rutinaGestorCola();

public:

    Daemon();
    ~Daemon();

    bool iniciar();
    void detener();

};

#endif // DAEMON_H
