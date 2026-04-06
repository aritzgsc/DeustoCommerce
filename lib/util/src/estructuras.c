#include "estructuras.h"

int cmpCandidatos(const void* a, const void* b) {

    double distA = ((AlmCandidato*)a)->distancia;
    double distB = ((AlmCandidato*)b)->distancia;

    if (distA < distB) return -1;
    if (distA > distB) return 1;
    return 0;

}
