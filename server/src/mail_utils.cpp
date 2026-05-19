#include "server.h"
#include "sesion.h"
#include "mail_utils.h"
#include "curl/curl.h"
#include <iostream>
#include <cstring>
#include <vector>
#include <ctime>
#include <string>

extern "C" {
    #include "config.h"
    #include "log.h"
}

using namespace std;

// Estructura auxiliar para que libcurl lea el cuerpo del mensaje
struct WriteThis {
    const char *readptr;
    size_t sizeleft;
};

// Auxiliar para la fecha del correo
static string obtenerFechaRFC2822() {
    char buffer[128];
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    // Formato: Thu, 21 Dec 2023 16:01:07 +0100
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S %z", timeinfo);
    return string(buffer);
}

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp) {
    struct WriteThis *upload = (struct WriteThis *)userp;
    size_t max = size * nmemb;

    if (upload->sizeleft) {
        size_t copy = upload->sizeleft;
        if (copy > max) copy = max;
        memcpy(ptr, upload->readptr, copy);
        upload->readptr += copy;
        upload->sizeleft -= copy;
        return copy;
    }
    return 0;
}

int enviarEmail(const char* destino, const char* asunto, const char* cuerpo, bool esHtml) {

	CURL *curl;
    CURLcode res = CURLE_OK;
    struct curl_slist *recipients = NULL;
    struct WriteThis upload_ctx;

    char user[128], pass[128];
    configGet(ENV_PATH, "MAIL_USER", user, sizeof(user));
    configGet(ENV_PATH, "MAIL_PASS", pass, sizeof(pass));

    // Construcción de la carga útil (headers + cuerpo)
    string headers = "Date: " + obtenerFechaRFC2822() + "\r\n";
    headers += "To: " + string(destino) + "\r\n";
    headers += "From: DeustoCommerce <" + string(user) + ">\r\n";
    headers += "Subject: " + string(asunto) + "\r\n";

    // Identificador único (vital para no ser spam)
    headers += "Message-ID: <msg-" + to_string(time(NULL)) + "@deustocommerce.com>\r\n";

    // Cabeceras MIME obligatorias
    headers += "MIME-Version: 1.0\r\n";
    if (esHtml) headers += "Content-Type: text/html; charset=UTF-8\r\n";
    headers += "\r\n"; // Línea vacía obligatoria entre headers y body
    headers += cuerpo;

    upload_ctx.readptr = headers.c_str();
    upload_ctx.sizeleft = headers.size();

    curl = curl_easy_init();

    if (curl) {

        curl_easy_setopt(curl, CURLOPT_USERNAME, user);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, pass);
        curl_easy_setopt(curl, CURLOPT_URL, "smtps://smtp.gmail.com:465");

        // Gmail requiere SSL/TLS
        curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);

        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, user);
        recipients = curl_slist_append(recipients, destino);
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            LOG_ERROR("Error libcurl al enviar mail: %s", curl_easy_strerror(res));
        }

        curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);
    }

    return (res == CURLE_OK) ? 1 : 0;

}

int enviarMailVerificacion(const char* destino, const char* codigo) {

	char cuerpoFinal[2048];

	const char* htmlTemplate =
	        "<!DOCTYPE html>"
	        "<html>"
	        "<head><meta charset=\"UTF-8\"></head>"
	        "<body style=\"margin: 0; padding: 0;\">"
	        "  <div style=\"text-align: center; font-family: sans-serif; color: #333;\">"
	        "    <h1 style=\"color: #2980b9;\">¡Hola! 👋</h1>"
	        "    <h2 style=\"color: #2c3e50;\">Bienvenido a DeustoCommerce 🚀</h2>"
	        "    <p style=\"font-size: 14pt;\">¡Estamos encantados de que te unas a nosotros!</p>"
	        "    <p style=\"font-size: 14pt;\">Para completar tu registro, introduce este código:</p>"
	        "    <div style=\"background-color: #e8f8f5; border: 2px dashed #1abc9c; padding: 15px; width: 50%%; margin: 20px auto; border-radius: 10px;\">"
	        "        <span style=\"font-size: 24pt; font-weight: bold; letter-spacing: 5px; color: #16a085;\">%s</span>"
	        "    </div>"
	        "    <p style=\"font-size: 12pt;\">Este código es válido para verificar tu cuenta.</p>"
	        "    <hr style=\"width: 80%%; border: 1px solid #ccc; margin: 20px auto;\">"
	        "    <p style=\"font-size: 10pt; color: #999;\">El equipo de DeustoCommerce</p>"
	        "  </div>"
	        "</body>"
	        "</html>";

    sprintf(cuerpoFinal, htmlTemplate, codigo);

    return enviarEmail(destino, "Verifica tu cuenta - DeustoCommerce", cuerpoFinal, true);

}

int enviarMailPedidoEnProceso(const char* correo, const char* nombre, int idPedido, time_t fecha, const ItemCarrito items[], int numItems, const Ubicacion& ubicacion) {

	tm* fechaLlegada = localtime(&fecha);

    char fechaLlegadaStr[128];
    strftime(fechaLlegadaStr, sizeof(fechaLlegadaStr), (char*)"%dd-%mm-%YYYY", fechaLlegada);

    string filasProductos = "";
    double totalCalculado = 0.0;

    // Generar dinámicamente las filas de la tabla de productos y calcular el total

    for (int i = 0; i < numItems; i++) {
        double precioConDescuento = items[i].precioUnitario * (1.0 - items[i].descuento);
        double subtotalItem = items[i].cantidad * precioConDescuento;
        totalCalculado += subtotalItem;

        char fila[1024];
        snprintf(fila, sizeof(fila),
            "<tr style=\"border-bottom: 1px solid #e2e8f0;\">"
            "  <td style=\"padding: 12px 0; color: #334155; text-align: left;\">"
            "    <span style=\"font-weight: 600; display: block;\">%s</span>"
            "    <span style=\"font-size: 9pt; color: #64748b;\">Var: %s</span>"
            "  </td>"
            "  <td style=\"padding: 12px 0; text-align: center; color: #334155;\">%d</td>"
            "  <td style=\"padding: 12px 0; text-align: right; color: #334155;\">%.2f €</td>"
            "  <td style=\"padding: 12px 0; text-align: right; color: #dc2626; font-weight: 500;\">-%.0f%%</td>"
            "  <td style=\"padding: 12px 0; text-align: right; font-weight: 600; color: #334155;\">%.2f €</td>"
            "</tr>",
            items[i].nombreProducto,
            (items[i].variante && items[i].variante[0] != '\0') ? items[i].variante : "Estándar",
            items[i].cantidad,
            items[i].precioUnitario,
            items[i].descuento * 100.0,
            subtotalItem
        );
        filasProductos += fila;
    }

    // Formatear la información detallada de la ubicación
    char infoUbicacion[1024];
    snprintf(infoUbicacion, sizeof(infoUbicacion),
        "<span style=\"font-weight: 600; color: #1e293b;\">%s</span><br>"
        "<span style=\"color: #475569;\">%s, %s</span><br>"
        "<span style=\"font-size: 9pt; color: #94a3b8;\">Coordenadas: %.4f, %.4f</span>",
        ubicacion.direccion,
        ubicacion.ciudad.nombre,
        ubicacion.ciudad.pais.nombre,
        ubicacion.latitud,
        ubicacion.longitud
    );

    // Formatear el string del precio total total
    char totalStr[32];
    snprintf(totalStr, sizeof(totalStr), "%.2f", totalCalculado);

    // Construcción de la plantilla HTML robusta y extendida
    string htmlTemplate =
        "<!DOCTYPE html>"
        "<html>"
        "<head><meta charset=\"UTF-8\"></head>"
        "<body style=\"margin: 0; padding: 0; background-color: #f8fafc; font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;\">"
        "  <div style=\"max-width: 600px; margin: 30px auto; background-color: #ffffff; border-radius: 12px; overflow: hidden; box-shadow: 0 4px 20px rgba(0,0,0,0.08); color: #334155;\">"
        "    "
        "    <div style=\"background-color: #1e3a8a; padding: 40px 20px; text-align: center;\">"
        "      <h1 style=\"color: #ffffff; margin: 0; font-size: 24pt; font-weight: 800; letter-spacing: 1px;\">DEUSTO<span style=\"color: #38bdf8;\">COMMERCE</span></h1>"
        "      <p style=\"color: #93c5fd; margin: 10px 0 0 0; font-size: 11pt;\">Confirmación de recepción de pedido</p>"
        "    </div>"
        "    "
        "    <div style=\"padding: 40px;\">"
        "      <h2 style=\"color: #0f172a; margin-top: 0; font-size: 16pt;\">¡Hola, " + string(nombre) + "! 👋</h2>"
        "      <p style=\"font-size: 11pt; color: #64748b; line-height: 1.6; margin-bottom: 30px;\">"
        "        Hemos recibido tu solicitud de compra correctamente. Nuestro almacén central ya está preparando los artículos "
        "        para su envío. A continuación, tienes el desglose y el ticket oficial de tu transacción:"
        "      </p>"
        "      "
        "      <div style=\"border: 1px solid #e2e8f0; border-radius: 8px; overflow: hidden; margin-bottom: 30px;\">"
        "        <div style=\"background-color: #f1f5f9; padding: 12px 20px; border-bottom: 1px solid #e2e8f0;\">"
        "          <span style=\"font-weight: bold; font-size: 10pt; color: #475569; letter-spacing: 0.5px;\">DATOS GENERALES DEL TICKET #" + to_string(idPedido) + "</span>"
        "        </div>"
        "        <div style=\"padding: 20px;\">"
        "          <table style=\"width: 100%; font-size: 11pt; border-collapse: collapse;\">"
        "            <tr style=\"border-bottom: 1px solid #f1f5f9;\">"
        "              <td style=\"padding: 10px 0; color: #64748b;\">Fecha estimada de Entrega:</td>"
        "              <td style=\"padding: 10px 0; font-weight: 500; text-align: right; color: #334155;\">" + string(fechaLlegadaStr) + "</td>"
        "            </tr>"
        "            <tr style=\"border-bottom: 1px solid #f1f5f9;\">"
        "              <td style=\"padding: 10px 0; color: #64748b;\">Estado del Pedido:</td>"
        "              <td style=\"padding: 10px 0; font-weight: bold; text-align: right; color: #2563eb;\">En Proceso</td>"
        "            </tr>"
        "            <tr>"
        "              <td style=\"padding: 12px 0 0 0; color: #64748b; vertical-align: top;\">Dirección de Envío:</td>"
        "              <td style=\"padding: 12px 0 0 0; text-align: right; max-width: 250px; word-wrap: break-word;\">" + string(infoUbicacion) + "</td>"
        "            </tr>"
        "          </table>"
        "        </div>"
        "      </div>"
        "      "
        "      <h3 style=\"color: #0f172a; font-size: 13pt; margin: 0 0 10px 0;\">Desglose del Carrito</h3>"
        "      <table style=\"width: 100%; font-size: 10.5pt; border-collapse: collapse; margin-bottom: 20px;\">"
        "        <thead>"
        "          <tr style=\"border-bottom: 2px solid #cbd5e1; text-align: left; color: #475569; font-size: 9.5pt;\">"
        "            <th style=\"padding: 8px 0; text-align: left;\">Producto</th>"
        "            <th style=\"padding: 8px 0; text-align: center;\">Cant.</th>"
        "            <th style=\"padding: 8px 0; text-align: right;\">Precio U.</th>"
        "            <th style=\"padding: 8px 0; text-align: right;\">Desc.</th>"
        "            <th style=\"padding: 8px 0; text-align: right;\">Total</th>"
        "          </tr>"
        "        </thead>"
        "        <tbody>" + filasProductos + "</tbody>"
        "      </table>"
        "      "
        "      <div style=\"border-top: 2px solid #e2e8f0; padding-top: 15px; text-align: right; margin-bottom: 30px;\">"
        "        <span style=\"font-size: 12pt; color: #0f172a; font-weight: bold; margin-right: 15px;\">Importe Total:</span>"
        "        <span style=\"font-weight: 800; color: #16a34a; font-size: 16pt;\">" + string(totalStr) + " €</span>"
        "      </div>"
        "      "
        "      <div style=\"background-color: #eff6ff; border-left: 4px solid #2563eb; padding: 15px 20px; border-radius: 4px; margin-bottom: 30px;\">"
        "        <p style=\"margin: 0; font-size: 10pt; color: #1e40af; line-height: 1.5;\">"
        "          <strong>Información sobre el seguimiento:</strong> En cuanto el transportista deje el paquete en su destino, recibirás un nuevo correo con el aviso de entrega."
        "        </p>"
        "      </div>"
        "      "
        "      <hr style=\"border: 0; border-top: 1px solid #e2e8f0; margin: 30px 0;\">"
        "      <p style=\"font-size: 9.5pt; color: #94a3b8; text-align: center; margin: 0;\">Este es un correo automático, por favor no lo respondas de forma directa.</p>"
        "      <p style=\"font-size: 10pt; color: #64748b; text-align: center; font-weight: bold; margin-top: 10px;\">El equipo de DeustoCommerce</p>"
        "    </div>"
        "  </div>"
        "</body>"
        "</html>";

    string asunto = "Ticket de Compra - Pedido #" + to_string(idPedido) + " [En Proceso]";

    return enviarEmail(correo, asunto.c_str(), htmlTemplate.c_str(), true);

}

int enviarMailPedidoEntregado(const char* correo, const char* nombre, const char* apellido, const Pedido& pedido) {

    char cuerpoFinal[8192];

    const char* htmlTemplate =
        "<!DOCTYPE html>"
        "<html>"
        "<head><meta charset=\"UTF-8\"></head>"
        "<body style=\"margin: 0; padding: 0; background-color: #f8fafc; font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;\">"
        "  <div style=\"max-width: 600px; margin: 30px auto; background-color: #ffffff; border-radius: 12px; overflow: hidden; box-shadow: 0 4px 20px rgba(0,0,0,0.08); color: #334155;\">"
        "    "
        "    "
        "    <div style=\"background-color: #16a34a; padding: 40px 20px; text-align: center;\">"
        "      <h1 style=\"color: #ffffff; margin: 0; font-size: 24pt; font-weight: 800; letter-spacing: 1px;\">¡PEDIDO ENTREGADO! 🎉</h1>"
        "      <p style=\"color: #dcfce7; margin: 10px 0 0 0; font-size: 11pt;\">Tu paquete ha llegado a su destino</p>"
        "    </div>"
        "    "
        "    "
        "    <div style=\"padding: 40px;\">"
        "      <h2 style=\"color: #0f172a; margin-top: 0; font-size: 16pt;\">¡Buenas noticias, %s %s!</h2>"
        "      <p style=\"font-size: 11pt; color: #64748b; line-height: 1.6; margin-bottom: 30px;\">"
        "        El servicio de mensajería nos acaba de confirmar que tu envío se ha depositado correctamente en el lugar de destino. "
        "        Aquí tienes el resumen definitivo del cierre del envío:"
        "      </p>"
        "      "
        "      "
        "      <div style=\"border: 1px solid #e2e8f0; border-radius: 8px; overflow: hidden; margin-bottom: 30px;\">"
        "        <div style=\"background-color: #f0fdf4; padding: 12px 20px; border-bottom: 1px solid #bbf7d0;\">"
        "          <span style=\"font-weight: bold; font-size: 10pt; color: #166534; letter-spacing: 0.5px;\">RESUMEN DE OPERACIÓN #%d</span>"
        "        </div>"
        "        <div style=\"padding: 20px;\">"
        "          <table style=\"width: 100%%; font-size: 11pt; border-collapse: collapse;\">"
        "            <tr style=\"border-bottom: 1px solid #f1f5f9;\">"
        "              <td style=\"padding: 10px 0; color: #64748b;\">Fecha de Compra:</td>"
        "              <td style=\"padding: 10px 0; font-weight: 500; text-align: right; color: #334155;\">%s</td>"
        "            </tr>"
        "            <tr style=\"border-bottom: 1px solid #f1f5f9;\">"
        "              <td style=\"padding: 10px 0; color: #64748b;\">Entregado en:</td>"
        "              <td style=\"padding: 10px 0; font-weight: bold; text-align: right; color: #1e293b; max-width: 250px; word-wrap: break-word;\">%s</td>"
        "            </tr>"
        "            <tr>"
        "              <td style=\"padding: 12px 0 0 0; color: #64748b;\">Coste total de la transacción:</td>"
        "              <td style=\"padding: 12px 0 0 0; font-weight: bold; text-align: right; color: #334155;\">%.2f €</td>"
        "            </tr>"
        "          </table>"
        "        </div>"
        "      </div>"
        "      "
        "      "
        "      <div style=\"text-align: center; margin: 35px 0;\">"
        "        <span style=\"background-color: #16a34a; color: #ffffff; padding: 12px 30px; border-radius: 6px; font-weight: bold; font-size: 11pt; display: inline-block; box-shadow: 0 4px 6px rgba(22,163,74,0.2);\">"
        "          ¡Disfruta de tu pedido!"
        "        </span>"
        "      </div>"
        "      "
        "      <p style=\"font-size: 10pt; color: #94a3b8; text-align: center;\">Si hay algún problema con el estado de los artículos recibidos, ponte en contacto con nosotros.</p>"
        "      <hr style=\"border: 0; border-top: 1px solid #e2e8f0; margin: 30px 0;\">"
        "      <p style=\"font-size: 10pt; color: #64748b; text-align: center; font-weight: bold; margin: 0;\">Gracias por comprar en DeustoCommerce</p>"
        "    </div>"
        "  </div>"
        "</body>"
        "</html>";

    int resultado = snprintf(cuerpoFinal, sizeof(cuerpoFinal), htmlTemplate, nombre, apellido, pedido.id, pedido.fecha, pedido.resumenDir, pedido.total);

    string asunto = "Confirmación de entrega: Pedido #" + to_string(pedido.id);

    return enviarEmail(correo, asunto.c_str(), cuerpoFinal, true);

}
