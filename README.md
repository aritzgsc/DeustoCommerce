# 📦 DeustoCommerce

> 🚧 **ESTADO DEL REPOSITORIO: EN DESARROLLO ACTIVO** 🚧
>
> *Este proyecto se encuentra actualmente en la **Fase 1**, la cual se compone del **Módulo de Administración local**. Toda la infraestructura correspondiente al **Módulo Cliente** y al **Módulo Servidor** (Fase 2) se encuentra actualmente pendiente de desarrollo.*

**DeustoCommerce** es un sistema distribuido de alto rendimiento diseñado para simular de manera realista el ecosistema de una plataforma de comercio electrónico a gran escala. Operando mediante una Interfaz de Línea de Comandos (CLI) altamente optimizada, el sistema garantiza una gestión robusta y eficiente de recursos.

El repositorio documenta y contiene la **Fase 1** del proyecto: una aplicación en C desarrollada para satisfacer las demandas de control logístico descentralizado, gestión avanzada de inventario y analítica financiera de la plataforma.

---

## 💻 Requisitos del Sistema y Entorno de Compilación

El proyecto ha sido estructurado bajo estrictos estándares de desarrollo. Para garantizar la correcta compilación y ejecución del código fuente, el entorno de despliegue debe cumplir con las siguientes especificaciones:

* 🪟 **Arquitectura del Sistema Operativo:** Microsoft Windows (64 bits).
* 🛠️ **Entorno de Desarrollo Integrado (IDE):** Eclipse IDE for C/C++ Developers.
* ⚙️ **Proceso de Importación y Compilación:** La configuración del compilador, las dependencias de enlazado (*linking*) y las rutas de inclusión (*includes*) están predefinidas en el entorno. Por ello, **es estrictamente obligatorio** copiar e importar los archivos `.project` y `.cproject` provistos en el repositorio al cargar el workspace en Eclipse. Omitir este paso impedirá que el IDE reconozca las librerías externas y la estructura modular, resultando en errores de compilación.

---

## 🚀 Arquitectura Funcional y Características Principales

El Módulo Administrador se divide en tres grandes ejes operativos que cubren todas las necesidades de *back-office* de la plataforma:

### 🛒 1. Gestión Avanzada del Catálogo Comercial

El motor de catálogo permite una administración profunda del portfolio de productos ofertados:

* **Exploración interactiva** del catálogo completo con algoritmos de búsqueda rápida y filtrado multicriterio por nombre, categoría y rango de precio.
* **Operaciones CRUD** completas para dar de alta nuevos artículos, retirar productos descatalogados o modificar metadatos como descripciones, precios y descuentos.
* **Selector interactivo de categorías** con paginación y búsqueda en tiempo real, que además expone las variantes disponibles (talla, color, etc.) asociadas a cada categoría.
* **Validación de integridad** referencial para evitar inconsistencias en el histórico de ventas al modificar o eliminar productos.

### 🏭 2. Inteligencia Logística y Red de Almacenes

El sistema implementa una red descentralizada de distribución de mercancías con capacidades de automatización:

* **Monitorización de Stock:** Trazabilidad en tiempo real de la disponibilidad global y el inventario granularizado por sede, con indicador visual de ocupación por almacén.
* **Restock Automatizado (*Smart Replenishment*):** Un algoritmo evalúa el flujo de inventario y genera órdenes de abastecimiento automáticas para reponer la capacidad operativa de los almacenes hasta un umbral óptimo del 80%, asegurando además un mínimo de ~1.300 referencias distintas por sede.
* **Transferencias Inter-Sedes:** Herramienta de movilidad de activos para trasladar lotes de mercancía manualmente entre diferentes puntos logísticos, con cálculo de coste y tiempo estimado de transporte.
* **Protocolo de Cierre y Reubicación:** Ante la clausura de un centro logístico, el sistema ejecuta un algoritmo de reubicación de emergencia. Utilizando la **Fórmula del Haversine**, calcula las distancias esféricas y transfiere automáticamente el stock al almacén operativo más cercano con capacidad disponible, estimando el coste total de la operación antes de confirmarla.

### 📊 3. Analítica de Datos y Reportes Financieros

El módulo cuenta con un sistema de Inteligencia de Negocio (BI) para apoyar la toma de decisiones gerenciales:

* **Balances Financieros Visuales:** Motor de cálculo que contrasta ingresos frente a gastos operativos. Exporta directamente a formato Microsoft Excel (`.xlsx`) generando dashboards con KPIs clave (beneficio neto, margen), evolución mensual mediante gráficos de líneas y columnas, y distribución circular de ingresos vs. gastos.
* **Detección de Dead Stock:** Algoritmo de auditoría que identifica productos que, teniendo stock en almacén, no han registrado ninguna venta en el histórico. El listado es exportable a `.csv` para planes de liquidación o renegociación con proveedores.
* **Top Ventas:** Generación de informes históricos sobre los artículos con mayor tracción en el mercado, con acumulado de unidades vendidas e ingresos generados, exportables a `.csv`.

### 📋 4. Registro y Auditoría

El sistema mantiene un registro completo y persistente de la actividad del administrador:

* **Log de operaciones** (`admin.log`): Registro cronológico de todas las acciones relevantes — creaciones, modificaciones, errores y operaciones logísticas — con niveles de severidad `INFO`, `WARN`, `ERROR` y `FATAL`.
* **Registro financiero** (`reg_financiero.csv`): Cada operación con impacto económico (venta, restock, trasvase, cierre de almacén) genera automáticamente una entrada contable con timestamp, tipo, concepto e importe, alimentando directamente los informes de balance.

---

## ⚙️ Configuración

El sistema evita el *hardcoding* de rutas y parámetros mediante ficheros de configuración `.ini` ubicados en `data/config/`. El administrador lee `server_config.ini` al arrancar para obtener, entre otras, las rutas a la base de datos, el log y los informes, permitiendo desplegar el sistema en distintos entornos sin necesidad de recompilar.

---

## 🗄️ Persistencia de Datos Masiva y Escalabilidad

Para asegurar un rendimiento óptimo bajo condiciones de estrés, la capa de persistencia se apoya en **SQLite**. El sistema ha sido sometido a pruebas de carga con una base de datos poblada masivamente con información extraída de entornos reales:

* 📍 **+400** Almacenes distribuidos con ubicaciones geográficas reales.
* 🏙️ **+30.000** Ciudades y municipios indexados.
* 🏷️ **+250** Categorías jerárquicas reales extraídas de la taxonomía de Amazon UK.
* 📦 **+200.000** Referencias de productos reales (nomenclatura en inglés).
* 🔄 **+1.500.000** Registros transaccionales de stock distribuidos entre almacenes.

El rendimiento de las consultas más frecuentes está optimizado mediante un conjunto de índices estratégicos sobre las tablas de mayor volumen (`STOCK_ALMACEN`, `PRODUCTO`, `PRODUCTOS_PEDIDO`).

---

## 📂 Topología del Workspace

A nivel de desarrollo, la carpeta raíz **DeustoCommerce** opera como un entorno de trabajo unificado (*Workspace* de Eclipse). Su arquitectura interna divide las responsabilidades en subproyectos independientes y directorios de recursos:

~~~text
📦 DeustoCommerce (Workspace)
 ┣ 📂 admin          # Proyecto en C — Módulo de Administrador (Fase 1)
 ┃ ┣ 📂 include      # Cabeceras específicas del módulo
 ┃ ┣ 📂 src          # Código fuente: pantallas UI (ui_catalogo, ui_almacenes, ui_informes...)
 ┃ ┣ 📂 logs         # admin.log — registro de auditoría de sesión
 ┃ ┗ 📜 main.c       # Punto de entrada: carga config, abre BD, lanza bucle
 ┣ 📂 db_seeder      # Herramienta de volcado masivo de datos a la BD
 ┣ 📂 lib            # Proyecto de librerías compartidas (reutilizables en Fase 2)
 ┃ ┣ 📂 db           # Módulo de persistencia (productos_db, almacenes_db, informes_db...)
 ┃ ┣ 📂 third_party  # Librerías externas (libxlsxwriter, SQLite, libcurl, libsodium, cJSON)
 ┃ ┗ 📂 util         # Utilidades: logística (Haversine), finanzas, config, log
 ┣ 📂 bin            # Ejecutables preparados para distribución
 ┗ 📂 data           # Recursos de datos del sistema
   ┣ 📂 docs         # Documentación del proyecto
   ┣ 📂 config       # Ficheros de configuración (.ini)
   ┣ 📂 db           # Base de datos SQLite (.db) y seeds de poblado
   ┗ 📂 reports      # Informes Excel y CSV generados por el admin
~~~

---

## 🛠️ Stack Tecnológico y Dependencias

La solución ha sido construida seleccionando herramientas orientadas al rendimiento y la portabilidad:

* **Lenguaje de Programación:** C (Fase 1 — Admin) / C++ (Fase 2 — Cliente y Servidor).
* **Motor de Base de Datos:** SQLite3 (embebido, relacional, sin servidor).
* **Integraciones y Dependencias de Terceros (`third_party`):**
  * `libxlsxwriter`: Generación en tiempo de ejecución de hojas de cálculo Excel con formato condicional y gráficos financieros embebidos.
  * `libcurl`: Comunicación HTTP/HTTPS con la API REST de OpenStreetMap, utilizada durante el proceso de poblado de datos geográficos.
  * `libsodium`: Biblioteca criptográfica integrada como dependencia de `lib` en preparación para el cifrado de contraseñas y comunicaciones seguras de la Fase 2. No activa en el módulo administrador.

---

*Arquitectura y Desarrollo por el **Grupo 11***
