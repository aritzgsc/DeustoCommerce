#ifndef INCLUDE_MAIL_UTILS_H_
#define INCLUDE_MAIL_UTILS_H_

#include <string>

extern "C" {
    #include "estructuras.h"
	#include "usuario_db.h"
}

// FUNCIÓN PADRE

int enviarEmail(const char* destino, const char* asunto, const char* cuerpo, bool esHtml);

// FUNCIONES ESPECÍFICAS

int enviarMailVerificacion(const char* destino, const char* codigo);
int enviarMailPedidoEnProceso(const char* correo, const char* nombre, int idPedido, time_t fecha, const ItemCarrito items[], int numItems, const Ubicacion& ubicacion);
int enviarMailPedidoEntregado(const char* correo, const char* nombre, const char* apellido, const Pedido& pedido);

#endif /* INCLUDE_MAIL_UTILS_H_ */
