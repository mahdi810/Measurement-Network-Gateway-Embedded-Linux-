# Measurement Network Gateway (MNG)

### Hochschule Bremerhaven – Master of Embedded Systems Design

**Team Project (8 Students)**

---

## 👥 Team Members

| Name                          | Student ID | Role / Responsibility             |
| ----------------------------- | ---------- | --------------------------------- |
| **Saeed Omidvari**            | 41490      | GitLab admin / System Integration |
| **Alin Raj Rajagopalan Nair** | 41520      | GUI (GTK / LibSlope)              |
| **Mohammad Mahdi Mohammadi**  | 42719      | Networking / TCP Protocol         |
| **Ameer Muhammed**            | 42843      | Threading & Timing Logic          |
| **Mohammad Reza Rahimi**      | 42756      | Hardware Interface & Simulation   |
| **Akshay Vastrad**            | 42723      | Test & Validation                 |
| **Mert Ali Türk**             | 42746      | Documentation Lead                |
| **Haizon Helet Cruz**         | 42708      | Architecture & Requirements       |

---

## 🧠 Project Overview

The **Measurement Network Gateway (MNG)** is an Embedded Linux–based data acquisition and communication system.
It collects sensor data from multiple sources, timestamps each measurement, and sends them over Ethernet to a control computer.
A graphical user interface (GUI) visualizes live data and allows configuration of sensors and sampling rates.

This project demonstrates the integration of:

* Multithreaded embedded software
* Real‑time scheduling
* TCP/IP communication
* GUI visualization using GTK
* Structured project management via GitLab

---

## 🧩 System Components

| Component                          | Description                                                 | Key Requirements                |
| ---------------------------------- | ----------------------------------------------------------- | ------------------------------- |
| **Gateway / Server**               | TCP server that streams measurement data to clients.        | REQ‑1 … REQ‑8, REQ‑13 … REQ‑21  |
| **Control & Monitor GUI (Client)** | GTK client that configures acquisition and plots live data. | REQ‑9 … REQ‑12, REQ‑15 … REQ‑17 |
| **Cyclic Thread Example**          | Demonstrates periodic execution with `pthread` + `timerfd`. | REQ‑13, REQ‑14, REQ‑27          |
| **Protocol Header**                | Binary message formats shared by server & client.           | REQ‑16, REQ‑17                  |

---

## 🗂️ Project Structure (V01 layout)

```text
mng/
├── zynq_MngSrv_V01/           # Gateway (server) implementation
│   ├── mngsrv.c
│   └── tcpmessage.h
├── mnggui_V01/                # GUI client implementation
│   ├── mnggui.c
│   └── tcpmessage.h
├── cyclicthread_V01/          # Multithreading & timing example
│   └── CyclicThread.c
├── Reference_Material/        # Datasheets, PDFs, board files
│   ├── BaseSystem.pdf
│   ├── Fabr_48747P01.pdf
│   ├── gcp_board.pdf
│   ├── MAX31865.pdf
│   └──── MAX31865PMB1.pdf
├── docs/                      # Requirements & design docs
│   └── requirements.md
├── report/                    # Final report & slides
│   └── README.md
└── README.md
```

> **Note:** This repository uses the `*_V01` folders as the **official** module layout going forward.

---

## ⚙️ Build Instructions

### 🔹 Prerequisites (Ubuntu / WSL)

```bash
sudo apt update
sudo apt install build-essential libgtk-3-dev libslope-dev
```

### 🔹 Build & Run — Server (Gateway)

```bash
cd zynq_MngSrv_V01
gcc -Wall -O2 -pthread mngsrv.c -o mngsrv
./mngsrv
```

### 🔹 Build & Run — GUI (Client)

```bash
cd mnggui_V01
gcc -Wall -O2 `pkg-config --cflags gtk+-3.0` mnggui.c -o mnggui \
    `pkg-config --libs gtk+-3.0` -lslope
./mnggui
```

### 🔹 Build & Run — Cyclic Thread Demo

```bash
cd cyclicthread_V01
gcc -Wall -O2 -pthread CyclicThread.c -o cyclicthread
./cyclicthread
```

---

## 🧮 GUI Command Console

| Command          | Description                 |
| ---------------- | --------------------------- |
| `conn 127.0.0.1` | Connect to the local server |
| `txstart`        | Start data transmission     |
| `txstop`         | Stop data transmission      |
| `disconn`        | Disconnect from the server  |

---

## 📊 Data Flow

**Server (`mngsrv.c`)**
• Accepts TCP client connection
• Generates timestamped sine/cosine samples
• Sends binary packets (`Srv_Send_struct`) via TCP

**Client (`mnggui.c`)**
• Connects to server
• Sends small control messages (`Clnt_Send_struct`)
• Receives packets and renders text + 2‑channel live plot (LibSlope)

**Cyclic Thread (`CyclicThread.c`)**
• Periodic scheduler using `timerfd`
• Alternating worker thread signaling
• Basis for REQ‑27 timing/jitter evaluation

---

## 🧩 Communication Protocol (`tcpmessage.h`)

```c
#define PORT 50012
#define MAX_SAMPLES 10
#define MAX_PARAMS 3

typedef struct {
    unsigned int msg_type;
    unsigned int nsamples;
    unsigned int scount;
    unsigned int stime[MAX_SAMPLES];
    unsigned int sval[MAX_SAMPLES];
} Srv_Send_struct;

typedef struct {
    unsigned int cmd;
    unsigned int param[MAX_PARAMS];
} Clnt_Send_struct;
```

---

## 🧪 Testing & Validation

| Test                       | Objective                         | Requirement     |
| -------------------------- | --------------------------------- | --------------- |
| Functional data transfer   | Verify live stream via TCP        | REQ‑1 … REQ‑12  |
| Sampling & timing accuracy | Measure jitter / latency          | REQ‑27          |
| Thread synchronization     | Validate periodic wake‑up         | REQ‑14          |
| GUI visualization          | Confirm plots & text updates      | REQ‑11, REQ‑12  |
| Safe shutdown              | Clean thread/resource termination | REQ‑18 … REQ‑21 |

---

## 🧰 Development Notes

* Ubuntu 22.04 / 24.04 / WSL2
* ANSI C, POSIX threads, GTK‑3
* LibSlope for real‑time plotting
* V01 modular layout for collaboration & CI/CD

---

## 📚 References

* GTK 3.0 API — [https://docs.gtk.org](https://docs.gtk.org)
* POSIX Threads — [https://man7.org/linux/man-pages/man7/pthreads.7.html](https://man7.org/linux/man-pages/man7/pthreads.7.html)
* LibSlope — [https://github.com/kai-mueller/libslope](https://github.com/kai-mueller/libslope)
* GitLab Docs — [https://docs.gitlab.com](https://docs.gitlab.com)
* Hochschule Bremerhaven — Embedded Systems Design Materials

---

© 2025 — Team MNG, Hochschule Bremerhaven
**All rights reserved.**

