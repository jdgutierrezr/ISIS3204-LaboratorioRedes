# 🧠 Manual de ejecución y análisis — Sistema Pub/Sub (TCP & UDP)

## 📌 Objetivo

Validar el funcionamiento de un sistema **publicador–suscriptor** mediante:
- Ejecución concurrente de procesos
- Verificación de entrega de mensajes
- Análisis de tráfico con Wireshark

---

## ⚙️ Requisitos

Antes de empezar, asegúrate de tener:

- Compilador C (`gcc`)
- Sistema tipo Linux / WSL / macOS
- Wireshark instalado

---

## 🧱 Estructura esperada

```
tcp/
  broker_tcp.c
  publisher_tcp.c
  subscriber_tcp.c

udp/
  broker_udp.c
  publisher_udp.c
  subscriber_udp.c
```

---

## 🚀 Ejecución del sistema (TCP)

### 🔹 Terminal 1 — Broker

```bash
gcc -o tcp/broker_tcp tcp/broker_tcp.c && ./tcp/broker_tcp
```

✔ Este proceso:
- Escucha conexiones
- Gestiona suscripciones
- Distribuye mensajes

---

### 🔹 Terminal 2 — Suscriptor (Topic A)

```bash
gcc -o tcp/subscriber_tcp tcp/subscriber_tcp.c && ./tcp/subscriber_tcp partido_A
```

✔ Este proceso:
- Se conecta al broker
- Se suscribe al topic `partido_A`
- Espera mensajes

---

### 🔹 Terminal 3 — Suscriptor adicional (Topic A o B)

```bash
./tcp/subscriber_tcp partido_A
```

o

```bash
./tcp/subscriber_tcp partido_B
```

---

### 🔹 Terminal 4 — Publicador (Topic A)

```bash
gcc -o tcp/publisher_tcp tcp/publisher_tcp.c && ./tcp/publisher_tcp partido_A
```

Luego escribe mensajes manualmente:

```
Gol de Equipo A al minuto 32
Penal al minuto 45
Final del primer tiempo
```

⚠️ Envía al menos **10 mensajes**.

---

### 🔹 Terminal 5 — Publicador adicional

```bash
./tcp/publisher_tcp partido_B
```

---

## 🔬 Captura con Wireshark

### 1. Iniciar captura

Abre Wireshark y:

- Selecciona la interfaz activa (loopback o red)
- Aplica filtro:

```
tcp.port == 8080
```

---

### 2. Qué deberías ver

#### 🔹 Conexión TCP (handshake)
- `SYN`
- `SYN-ACK`
- `ACK`

→ Esto prueba que usas **TCP correctamente**

---

#### 🔹 Envío de comandos

Mensajes tipo:

```
SUB partido_A
PUB partido_A Gol...
```

→ Verifica que el protocolo de aplicación funciona

---

#### 🔹 Distribución de mensajes

Un `PUB` debe generar:
- múltiples paquetes hacia suscriptores

Si no pasa eso → tu broker está mal

---

### 3. Guardar captura

Guardar como:

```
tcp_pubsub.pcap
```

---

## 🔁 Prueba equivalente en UDP

Repite el mismo proceso usando los archivos en `/udp`.

### Filtro en Wireshark:

```
udp.port == 8080
```

Guardar como:

```
udp_pubsub.pcap
```

---

## ✅ Validaciones obligatorias

No es solo “que corra”. Debes comprobar:

### ✔ Entrega correcta
- Cada mensaje publicado llega a todos los suscriptores del topic

---

### ✔ Aislamiento de topics
- Mensajes de `partido_A` NO llegan a `partido_B`

---

### ✔ Diferencia TCP vs UDP

Debes notar:

| Característica | TCP | UDP |
|------|------|------|
| Conexión | Sí | No |
| Fiabilidad | Alta | Baja |
| Orden | Garantizado | No |
| Retransmisión | Sí | No |

---

## ⚠️ Errores comunes (y cómo detectarlos)

### ❌ No ves tráfico en Wireshark
- Estás en interfaz incorrecta
- Usa loopback (`lo`)

---

### ❌ Solo ves un paquete por mensaje
- No estás manejando múltiples suscriptores

---

### ❌ Mensajes incompletos
- TCP es un stream, no mensajes
- Estás leyendo mal el buffer

---

### ❌ No llegan mensajes
- Suscriptor no se registró correctamente
- Broker no enruta bien

---

## 🧪 Criterios de entrega

Debes cumplir:

1. Ejecutar:
   - 1 broker
   - ≥2 suscriptores
   - ≥2 publicadores

2. Enviar:
   - ≥10 mensajes por publicador

3. Validar:
   - Entrega correcta
   - Separación por topics

4. Entregar:
   - `tcp_pubsub.pcap`
   - `udp_pubsub.pcap`

---

## 🧠 Conclusión

No basta con que funcione.

Debes entender:

- Diferencia entre **TCP vs UDP**
- Qué es un **protocolo de aplicación**
- Cómo fluye un mensaje en la red
- Cómo analizarlo con Wireshark

Si no puedes explicarlo viendo la captura, no entendiste tu propio sistema.
