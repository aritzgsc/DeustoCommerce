#ifndef INCLUDE_AUTH_UI_H_
#define INCLUDE_AUTH_UI_H_

#include "client.h"

// AUTOLOGIN

void intentarAutoLogin(Client& cliente);

// LOGIN

void pantallaLogin(Client& cliente);

// REGISTRO

void pantallaRegistro(Client& cliente);

// LOGOUT

void efectuarLogout(Client& cliente);


#endif /* INCLUDE_AUTH_UI_H_ */
