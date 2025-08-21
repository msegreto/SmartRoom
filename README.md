# SmartRoom  
### An IoT Solution for Energy Saving / Una Soluzione IoT per il Risparmio Energetico  

---

## English Version  

### Overview  
SmartRoom is an IoT-based system designed to improve **energy efficiency** in indoor environments while maintaining user comfort. The project integrates **low-cost sensors, actuators, and lightweight communication protocols** to demonstrate how even constrained devices can support intelligent energy management.  

The system is divided into two main modules:  
- **Environmental Module**: Collects temperature and humidity data, estimates *perceived temperature*, and controls heating/cooling systems.  
- **Lighting Module**: Manages lighting with manual switches and presence detection to automatically save energy.  

Both modules communicate through a **CoAP-based network**, ensuring distributed, scalable, and efficient control.  

---

### Methodology & Procedure  
1. **Protocol Choice**  
   - Adopted **CoAP** instead of MQTT to ensure direct, peer-to-peer communication, minimal overhead, and higher robustness in local networks.  
   - RESTful, resource-oriented structure for modular and intuitive design.  

2. **Sensor & Actuator Configuration**  
   - Environmental sensors (temperature & humidity) equipped with **local predictive models** (Decision Trees) trained on time series.  
   - Anticipatory control of heating/cooling to stabilize indoor comfort and reduce energy peaks.  
   - Lighting controlled through presence detection to prevent unnecessary waste.  

3. **Data Encoding & Cloud Integration**  
   - Used **lightweight JSON payloads** to ensure efficient communication.  
   - Cloud application (Java + Californium library) handles registration, discovery, and real-time observation of nodes.  
   - Database (MySQL) for storing logs, supporting **Grafana** visualization.  

4. **Machine Learning Integration**  
   - Time series transformed into uniform datasets (30-min intervals).  
   - Forecasting models deployed directly on embedded devices using **emlearn**.  
   - Local predictive capabilities allow **autonomous decision-making** without relying on external servers.  

---

### Network Architecture and External Components  

#### CoAP Network Structure  
The system is based on a **distributed CoAP network**, where each sensor and actuator is exposed as a **resource** accessible through RESTful URIs. The network is fully **local and self-contained**, meaning that the core functionalities do not depend on external internet connectivity.  

Main CoAP nodes:  
- **Temperature Sensor Node** → `/temp`, `/predt`, `/ont`, `/offt`  
- **Humidity Sensor Node** → `/hum`, `/predh`, `/onh`, `/offh`  
- **HAC (Heating and Cooling) Node** → `/sts`, `/setlim`, `/getlim`, `/onhac`, `/offhac`  
- **Lighting Sensor Node** → `/light`, `/onlightsens`, `/offlightsens`  
- **Lighting Actuator Node** → `/led`, `/onlightact`, `/offlightact`  

Nodes communicate directly in **peer-to-peer fashion**, enabling fast and reliable interactions without a central broker.  

#### Interaction with External Components  
- **Cloud Application (Java + Californium)** → registers nodes, manages discovery and observations, bridges local network with user services.  
- **User Application (CLI)** → manual control (ON/OFF, thresholds, LED), diagnostics, and real-time monitoring.  
- **SQL Database (MySQL)** → stores metadata, logs sensor data, backend for user app & Grafana.  
- **Grafana Dashboard** → visualization of real-time and predicted values, monitoring system performance.  

This layered structure ensures **autonomous operation** of the local CoAP network, while external components enhance **usability, persistence, and monitoring**.  

---

### Strengths of the Work  
- **Low-cost and resource-constrained devices** used to achieve intelligent automation.  
- **Decentralized, fault-tolerant architecture**: system remains operational even if the cloud service is unavailable.  
- **Predictive approach** reduces energy consumption by acting proactively rather than reactively.  
- **Scalability and modularity** thanks to RESTful CoAP resources.  
- **Real-time visualization and monitoring** via Grafana dashboard.  

---

## Versione Italiana  

### Panoramica  
SmartRoom è un sistema basato su **tecnologie IoT** pensato per migliorare l’**efficienza energetica** negli ambienti interni senza sacrificare il comfort dell’utente. Il progetto dimostra come sia possibile ottenere un controllo intelligente anche con **dispositivi a basso costo e con risorse limitate**, grazie a sensori, attuatori e protocolli di comunicazione leggeri.  

Il sistema è diviso in due moduli principali:  
- **Modulo Ambientale**: Rileva temperatura e umidità, calcola la *temperatura percepita* e regola il riscaldamento/raffrescamento.  
- **Modulo Illuminazione**: Gestisce l’illuminazione con interruttori manuali e rilevatori di presenza, spegnendo automaticamente le luci in assenza di attività.  

I moduli comunicano tramite una **rete basata su CoAP**, che garantisce controllo distribuito, scalabile ed efficiente.  

---

### Metodologia e Procedimento  
1. **Scelta del Protocollo**  
   - Utilizzo di **CoAP** al posto di MQTT per favorire la comunicazione diretta tra nodi, ridurre l’overhead e aumentare la robustezza in reti locali.  
   - Struttura RESTful per un design modulare e intuitivo.  

2. **Configurazione di Sensori e Attuatori**  
   - Sensori ambientali (temperatura e umidità) dotati di **modelli predittivi locali** (Decision Tree) allenati su serie temporali.  
   - Controllo anticipato di riscaldamento/raffrescamento per stabilizzare il comfort e ridurre i picchi energetici.  
   - Illuminazione gestita da sensori di presenza per evitare sprechi.  

3. **Codifica Dati e Integrazione Cloud**  
   - Utilizzo di **payload JSON leggeri** per comunicazioni efficienti.  
   - Applicazione cloud (Java + libreria Californium) per registrazione, discovery e osservazione in tempo reale dei nodi.  
   - Database MySQL per memorizzare i log e supportare la **visualizzazione su Grafana**.  

4. **Integrazione del Machine Learning**  
   - Trasformazione delle serie temporali in dataset uniformi (intervalli da 30 minuti).  
   - Modelli predittivi implementati direttamente sui dispositivi embedded con **emlearn**.  
   - Capacità di previsione locale che permette decisioni autonome senza dipendenza dal cloud.  

---

### Architettura di Rete e Componenti Esterni  

#### Struttura della Rete CoAP  
Il sistema si basa su una **rete distribuita CoAP**, in cui ogni sensore e attuatore è esposto come **risorsa** accessibile tramite URI RESTful. La rete è **locale e autosufficiente**, quindi le funzionalità principali non dipendono da una connessione Internet esterna.  

Principali nodi CoAP:  
- **Nodo Sensore di Temperatura** → `/temp`, `/predt`, `/ont`, `/offt`  
- **Nodo Sensore di Umidità** → `/hum`, `/predh`, `/onh`, `/offh`  
- **Nodo HAC (Heating and Cooling)** → `/sts`, `/setlim`, `/getlim`, `/onhac`, `/offhac`  
- **Nodo Sensore di Illuminazione** → `/light`, `/onlightsens`, `/offlightsens`  
- **Nodo Attuatore Luci** → `/led`, `/onlightact`, `/offlightact`  

I nodi comunicano tra loro in **modalità peer-to-peer**, garantendo interazioni rapide e affidabili senza bisogno di un broker centrale.  

#### Interazione con i Componenti Esterni  
- **Applicazione Cloud (Java + Californium)** → registra i nodi, gestisce discovery e osservazioni, fa da ponte tra rete locale e servizi utente.  
- **Applicazione Utente (CLI)** → controllo manuale (ON/OFF, soglie HAC, LED), diagnostica e monitoraggio in tempo reale.  
- **Database SQL (MySQL)** → memorizza metadati, registra i dati dei sensori, backend per app utente e Grafana.  
- **Dashboard Grafana** → visualizzazione dei valori in tempo reale e predetti, monitoraggio prestazioni del sistema.  

Questa struttura stratificata garantisce che la **rete CoAP locale funzioni autonomamente**, mentre i **componenti esterni arricchiscono il sistema con usabilità, persistenza e monitoraggio avanzato**.  

---

### Punti di Forza del Lavoro  
- Uso di **dispositivi a basso costo** per implementare sistemi intelligenti.  
- **Architettura decentralizzata e robusta**, in grado di funzionare anche offline.  
- **Approccio predittivo** che consente risparmio energetico agendo in anticipo.  
- **Scalabilità e modularità** grazie a risorse RESTful su CoAP.  
- **Monitoraggio e visualizzazione in tempo reale** con dashboard Grafana.  

---
