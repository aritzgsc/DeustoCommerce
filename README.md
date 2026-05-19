# 📦 DeustoCommerce

**DeustoCommerce** es un sistema distribuido de alto rendimiento diseñado para simular de manera
realista el ecosistema de una plataforma de comercio electrónico a gran escala. Operando mediante
una Interfaz de Línea de Comandos (CLI) altamente optimizada, el sistema garantiza una gestión
robusta y eficiente de recursos.

El repositorio contiene el proyecto completo: el **Módulo Administrador** (Fase 1), desarrollado
en C, y el **Módulo Servidor** y **Módulo Cliente** (Fase 2), desarrollados en C++, que juntos
forman la plataforma distribuida completa.

---

## 💻 Requisitos del Sistema y Entorno de Compilación

El proyecto ha sido estructurado bajo estrictos estándares de desarrollo. Para garantizar la
correcta compilación y ejecución del código fuente, el entorno de despliegue debe cumplir con
las siguientes especificaciones:

* 🪟 **Arquitectura del Sistema Operativo:** Microsoft Windows (64 bits).
* 🛠️ **Entorno de Desarrollo Integrado (IDE):** Eclipse IDE for C/C++ Developers.
* ⚙️ **Proceso de Importación y Compilación:** La configuración del compilador, las dependencias
de enlazado (*linking*) y las rutas de inclusión (*includes*) están predefinidas en el entorno.
Por ello, **es estrictamente obligatorio** copiar e importar los archivos `.project` y `.cproject`
provistos en el repositorio al cargar el workspace en Eclipse. Omitir este paso impedirá que el
IDE reconozca las librerías externas y la estructura modular, resultando en errores de compilación.

---

## 🚀 Arquitectura Funcional y Características Principales

### 🖥️ Módulo Administrador (Fase 1 — C)

El Módulo Administrador se divide en cuatro grandes ejes operativos que cubren todas las
necesidades de *back-office* de la plataforma:

#### 🛒 1. Gestión Avanzada del Catálogo Comercial

El motor de catálogo permite una administración profunda del portfolio de productos ofertados:

* **Exploración interactiva** del catálogo completo con algoritmos de búsqueda rápida y filtrado
multicriterio por nombre, categoría y rango de precio.
* **Operaciones CRUD** completas para dar de alta nuevos artículos, retirar productos
descatalogados o modificar metadatos como descripciones, precios y descuentos.
* **Selector interactivo de categorías** con paginación y búsqueda en tiempo real, que además
expone las variantes disponibles (talla, color, etc.) asociadas a cada categoría.
* **Validación de integridad** referencial para evitar inconsistencias en el histórico de ventas
al modificar o eliminar productos.

#### 🏭 2. Inteligencia Logística y Red de Almacenes

El sistema implementa una red descentralizada de distribución de mercancías con capacidades
de automatización:

* **Monitorización de Stock:** Trazabilidad en tiempo real de la disponibilidad global y el
inventario granularizado por sede, con indicador visual de ocupación por almacén.
* **Restock Automatizado (*Smart Replenishment*):** Un algoritmo evalúa el flujo de inventario
y genera órdenes de abastecimiento automáticas para reponer la capacidad operativa de los
almacenes hasta un umbral óptimo del 80%, asegurando además un mínimo de ~1.300 referencias
distintas por sede.
* **Transferencias Inter-Sedes:** Herramienta de movilidad de activos para trasladar lotes de
mercancía manualmente entre diferentes puntos logísticos, con cálculo de coste y tiempo estimado
de transporte.
* **Protocolo de Cierre y Reubicación:** Ante la clausura de un centro logístico, el sistema
ejecuta un algoritmo de reubicación de emergencia. Utilizando la **Fórmula del Haversine**,
calcula las distancias esféricas y transfiere automáticamente el stock al almacén operativo más
cercano con capacidad disponible, estimando el coste total de la operación antes de confirmarla.

#### 📊 3. Analítica de Datos y Reportes Financieros

El módulo cuenta con un sistema de Inteligencia de Negocio (BI) para apoyar la toma de
decisiones gerenciales:

* **Balances Financieros Visuales:** Motor de cálculo que contrasta ingresos frente a gastos
operativos. Exporta directamente a formato Microsoft Excel (`.xlsx`) generando dashboards con
KPIs clave (beneficio neto, margen), evolución mensual mediante gráficos de líneas y columnas,
y distribución circular de ingresos vs. gastos.
* **Detección de Dead Stock:** Algoritmo de auditoría que identifica productos que, teniendo
stock en almacén, no han registrado ninguna venta en el histórico. El listado es exportable
a `.csv` para planes de liquidación o renegociación con proveedores.
* **Top Ventas:** Generación de informes históricos sobre los artículos con mayor tracción en
el mercado, con acumulado de unidades vendidas e ingresos generados, exportables a `.csv`.

#### 📋 4. Registro y Auditoría

El sistema mantiene un registro completo y persistente de la actividad del administrador:

* **Log de operaciones** (`admin.log`): Registro cronológico de todas las acciones relevantes
— creaciones, modificaciones, errores y operaciones logísticas — con niveles de severidad
`INFO`, `WARN`, `ERROR` y `FATAL`.
* **Registro financiero** (`reg_financiero.csv`): Cada operación con impacto económico (venta,
restock, trasvase, cierre de almacén) genera automáticamente una entrada contable con timestamp,
tipo, concepto e importe, alimentando directamente los informes de balance.

---

### 🌐 Módulo Servidor (Fase 2 — C++)

#### 🔀 1. Arquitectura Multihilo y Protocolo

El servidor gestiona cada conexión entrante en un hilo independiente, de forma que la carga de
un cliente no afecta al resto. Todas las peticiones pasan por el protocolo, que actúa como
enrutador y las delega al handler adecuado. Una clase `Sesion` mantiene el estado de cada
cliente durante su conexión, y un dashboard de consola permite monitorizar el sistema en
tiempo real, además de avanzar el tiempo virtual del servidor para probar las diferentes 
funcionalidades implementadas.

#### 👻 2. Demonio del Servidor

Al arrancar, el servidor lanza un proceso demonio en background con dos hilos de
responsabilidades bien diferenciadas:

* **Hilo de tareas periódicas:** Ejecuta el restock automático sobre almacenes con stock crítico
cada fin de semana, y genera una nueva pestaña en el Anuario Financiero al final de cada mes.
* **Hilo de cola de tareas:** Lee cada minuto un fichero `.csv` de tareas programadas y ejecuta
las acciones cuyo timestamp ha sido superado por el tiempo virtual del servidor (cambios de
estado de pedidos, envío de notificaciones, etc.).

#### 🔐 3. Autenticación y Seguridad

* Verificación de correo electrónico obligatoria durante el registro.
* Contraseñas cifradas en tránsito vía **TLS (OpenSSL)** y almacenadas hasheadas con
**libsodium**.
* Tokens de sesión con caducidad de 7 días para auto-login.

#### 📧 4. Notificaciones por Email

Mediante **libcurl + SMTP**, el servidor envía códigos de verificación en el registro,
confirmaciones de pedido y actualizaciones de estado gestionadas por el demonio.

---

### 📱 Módulo Cliente (Fase 2 — C++)

Interfaz de consola ligera que se comunica con el servidor a través de una conexión
**TLS cifrada**, sin acceso directo a la base de datos en ningún momento:

* **Autenticación:** Registro con verificación por email, login seguro y auto-login por token.
* **Catálogo:** Navegación y búsqueda con filtros por nombre, categoría y precio.
* **Carrito y Compra:** Gestión del carrito, selección de dirección de entrega y confirmación
de pedido.
* **Seguimiento:** Historial completo de pedidos y actualizaciones de estado en tiempo real.

---

## 🗄️ Persistencia de Datos Masiva y Escalabilidad

Para asegurar un rendimiento óptimo bajo condiciones de estrés, la capa de persistencia se
apoya en **SQLite**. El sistema ha sido sometido a pruebas de carga con una base de datos
poblada masivamente con información extraída de entornos reales:

* 📍 **+400** Almacenes distribuidos con ubicaciones geográficas reales.
* 🏙️ **+30.000** Ciudades y municipios indexados.
* 🏷️ **+250** Categorías jerárquicas reales extraídas de la taxonomía de Amazon UK.
* 📦 **+200.000** Referencias de productos reales (nomenclatura en inglés).
* 🔄 **+1.500.000** Registros transaccionales de stock distribuidos entre almacenes.

El rendimiento de las consultas más frecuentes está optimizado mediante un conjunto de índices
estratégicos sobre las tablas de mayor volumen (`STOCK_ALMACEN`, `PRODUCTO`, `PRODUCTOS_PEDIDO`).

---

## ⚙️ Configuración

El sistema evita el *hardcoding* de rutas y parámetros mediante ficheros de configuración `.ini`
ubicados en `data/config/`. Antes de ejecutar el servidor es necesario introducir en
`env/.env` las credenciales SMTP (dirección de Gmail y contraseña de aplicación) en
la sección `[MAIL]`. El resto de parámetros —rutas a la base de datos, logs e informes— están
preconfigurados y no requieren modificación para un despliegue estándar.

---

## 📂 Topología del Workspace

```text
📦 DeustoCommerce (Workspace)
 ┣ 📂 admin          # Proyecto en C — Módulo Administrador (Fase 1)
 ┃ ┣ 📂 include      # Cabeceras específicas del módulo
 ┃ ┣ 📂 src          # Código fuente (catalogo_ui, almacenes_ui, informes_ui...)
 ┃ ┣ 📂 logs         # admin.log — registro de auditoría de sesión
 ┃ ┗ 📜 main.c       # Punto de entrada: carga config, abre BD, lanza bucle
 ┣ 📂 server         # Proyecto en C++ — Módulo Servidor (Fase 2)
 ┃ ┣ 📂 include
 ┃ ┣ 📂 src
 ┃ ┃ ┣ 📂 handlers   # auth_h, catalogo_h, client_h
 ┃ ┃ ┣ 📂 daemon     # Tareas periódicas y cola de tareas asíncrona
 ┃ ┃ ┗ 📂 session    # Gestión de sesiones por cliente
 ┃ ┣ 📂 logs         # server.log
 ┃ ┗ 📜 main.cpp
 ┣ 📂 client         # Proyecto en C++ — Módulo Cliente (Fase 2)
 ┃ ┣ 📂 config       # Fichero de configuración del cliente (.ini)
 ┃ ┣ 📂 include
 ┃ ┣ 📂 src
 ┃ ┃ ┗ 📂 ui         # ui_auth, ui_catalogo, ui_pedidos
 ┃ ┣ 📂 logs         # client.log
 ┃ ┗ 📜 main.cpp
 ┣ 📂 db_seeder      # Herramienta de volcado masivo de datos a la BD
 ┣ 📂 lib            # Librerías compartidas entre todos los módulos
 ┃ ┣ 📂 db           # Módulo de persistencia (catalogo_db, almacenes_db...)
 ┃ ┣ 📂 third_party  # Librerías externas (SQLite, libcurl, libsodium, libxlsxwriter, cJSON)
 ┃ ┗ 📂 util         # Utilidades: logística (Haversine), finanzas, config, log, utils_ui...
 ┣ 📂 bin            # Ejecutables preparados para distribución
 ┗ 📂 data           # Recursos de datos del sistema
   ┣ 📂 config       # Fichero de configuración del servidor (.ini)
   ┣ 📂 db           # Base de datos SQLite (.db) y seeds de la BD
   ┣ 📂 reports      # Informes Excel y CSV
   ┗ 📂 docs         # Documentación del proyecto
```

---

## 🛠️ Stack Tecnológico y Dependencias

* **Lenguajes:** C (Fase 1 — Admin) / C++ (Fase 2 — Servidor y Cliente).
* **Motor de Base de Datos:** SQLite3 (embebido, relacional, sin servidor).
* **Integraciones y Dependencias de Terceros (`third_party`):**
  * `OpenSSL`: Cifrado TLS de toda la comunicación cliente-servidor.
  * `libsodium`: Hashing seguro de contraseñas y gestión criptográfica.
  * `libcurl`: Envío de correos transaccionales vía SMTP y comunicación con la API de
  OpenStreetMap durante el poblado de datos y creación de ubicaciones.
  * `libxlsxwriter`: Generación de hojas de cálculo Excel con gráficos financieros embebidos.
  * `cJSON`: Serialización y deserialización de mensajes en el protocolo cliente-servidor.

---

*Arquitectura y Desarrollo por el **Grupo 11***
