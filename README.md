# SmartRoom  
### An IoT Solution for Energy Saving / Una Soluzione IoT per il Risparmio Energetico  

---

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
### Academic Disclaimer

This project was developed as part of university coursework for educational and experimental purposes.

It is a prototype intended to explore and demonstrate concepts related to the Internet of Things (IoT), distributed systems, and smart-environment applications.

The implementation was created for learning and experimentation and should not be considered a production-ready system without additional review, testing, and security hardening.

The software is provided "as is", without warranty of any kind. See the MIT License for details.
---
