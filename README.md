# NS-3 Routing Simulator

A configurable network routing simulator implemented using **NS-3.48** to study routing behavior across different network topologies and evaluate network performance metrics such as packet delivery ratio, throughput, and end-to-end delay.

---

## Features

* Multiple network topologies

  * Random Connected Graph
  * Ring
  * Full Mesh
  * Binary Tree

* Configurable network size

  * Tested with 20–500 routers

* UDP-based packet transmission

* IPv4 routing using NS-3 Global Routing

* FlowMonitor integration for performance analysis

* NetAnim support for packet visualization

* Scalable and modular simulation design

---

## Project Overview

This project ports a custom routing simulator into the NS-3 simulation framework.

The simulator creates configurable network topologies, installs network stacks on routers, generates packet traffic between nodes, and measures network performance using NS-3's built-in monitoring tools.

The objective is to provide a realistic packet-level simulation environment for analyzing routing behavior under different network structures.

---

## Supported Topologies

| Topology | Description                                  |
| -------- | -------------------------------------------- |
| Random   | Connected graph with additional random links |
| Ring     | Circular chain of routers                    |
| Mesh     | Every router connected to every router       |
| Tree     | Binary hierarchical topology                 |

---

## Simulation Components

### Network Layer

* Point-to-Point Channels
* Internet Stack
* IPv4 Address Assignment
* Global Routing Tables

### Traffic Layer

* UDP Echo Server
* UDP Echo Client

### Analysis Layer

* FlowMonitor
* NetAnim Visualization

---

## Performance Metrics

The simulator measures:

| Metric                | Description                    |
| --------------------- | ------------------------------ |
| Packet Delivery Ratio | Successfully delivered packets |
| End-to-End Delay      | Average packet delay           |
| Throughput            | Data transfer rate             |
| Packet Loss           | Lost packets                   |
| Flow Statistics       | Per-flow analysis              |

---

## Experimental Results

Initial testing was performed on all supported topologies.

| Parameter             | Value               |
| --------------------- | ------------------- |
| Router Range          | 20 – 500            |
| Packet Delivery Ratio | 100%                |
| Routing Method        | NS-3 Global Routing |
| Transport Protocol    | UDP                 |

The simulator successfully delivered packets across all tested topologies while collecting detailed performance statistics.

---

## Requirements

| Software | Version |
| -------- | ------- |
| NS-3     | 3.48    |
| C++      | C++17   |
| Python   | 3.x     |
| CMake    | 3.16+   |

---

## Build

### Configure

```bash
./ns3 configure --enable-examples
```

### Build

```bash
./ns3 build
```

### Run

```bash
./ns3 run routing-sim
```

---

## Example Usage

### Random Topology

```bash
./ns3 run "routing-sim --routers=100 --topology=random"
```

### Ring Topology

```bash
./ns3 run "routing-sim --routers=50 --topology=ring"
```

### Mesh Topology

```bash
./ns3 run "routing-sim --routers=20 --topology=mesh"
```

### Tree Topology

```bash
./ns3 run "routing-sim --routers=31 --topology=tree"
```

---

## Output

The simulation generates:

```text
animation.xml
```

which can be opened using **NetAnim** for visualization.

FlowMonitor statistics are displayed in the terminal after simulation completion.

---

## Repository Structure

```text
ns3-Routing-Simulator/
│
├── src/
│   └── routing-sim.cc
│
├── screenshots/
│
├── docs/
│
└── README.md
```

---

## Screenshots

Add screenshots of:

* Random topology
* Ring topology
* Mesh topology
* Tree topology
* NetAnim visualization
* Terminal output

inside the `screenshots/` directory.

---

## Future Work

* Additional routing algorithms
* Dynamic topology changes
* Link failure simulation
* Large-scale benchmarking
* Docker-based deployment


