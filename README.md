<img width="1280" height="720" alt="pc-usb-telemetry-gif-2" src="https://github.com/user-attachments/assets/7aed80fe-1b8a-4421-b027-698a06cbd276" /># ⚙️ VisualBuildStateMachine Creator
<img width="2048" height="2064" alt="VisualBuildStateMachine Creator icon" src="https://github.com/user-attachments/assets/39e0b768-1548-4bae-a87f-7a51290f32eb" />

![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Qt](https://img.shields.io/badge/Qt-%23217346.svg?style=for-the-badge&logo=Qt&logoColor=white)
![Windows](https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white)


**VisualBuildStateMachine Creator** — is a native desktop application designed for visual modeling of Finite State Machines (FSM) and automatic source code generation.


⚠️ **System Requirements:**
 **OS:** Windows 64-bit (Windows 32-bit is strictly NOT supported).


## ✨ Available Code Generation Targets
The application supports instant generation of optimized code for the following platforms and languages:
 **C Micro** (built-in support for Arduino / microcontrollers)
 **C++ Desktop**
 **Python**
 **Java**

<img width="1280" height="720" alt="демонстрація-сгенероаного-коду" src="https://github.com/user-attachments/assets/73d1675a-ed05-42e2-8464-0901407e2388" />


## 🔥 Core Features


### 1. Embedded-First Architecture (No RTOS Required)
VisualBuild is designed with strict embedded constraints in mind:
 **Zero Dynamic Allocation:** The generated FSM relies strictly on static memory. No `malloc` or `new` during runtime, completely eliminating heap fragmentation and Memory Leaks.
 **Mathematical Parallelism:** Running multiple concurrent states? The compiler intelligently switches from scalar routing to vector routing, allowing true logical parallelism on single-core microcontrollers *without* the overhead of an RTOS. 
 **Safe Self-Loops:** Implements a localized `transition_taken` flag to handle "self-to-self" transitions flawlessly, which is critical for non-blocking timers and sensor polling.

### 2. Live Hardware & PC Debugging
Don't just write code—watch it execute in real-time.
  **Hardware Debugger:** Inject lightweight telemetry (`[B:id]`, `[T:id]`) directly into your microcontroller via USB (Serial) or over-the-air via Wi-Fi (TCP/IP). Watch the active nodes light up green on the canvas.
  **PC Emulator:** Run your generated logic locally within the IDE's virtual machine. No external hardware required.

<img width="1280" height="720" alt="Untitled" src="https://github.com/user-attachments/assets/a0a14ac2-30f8-4039-8e0f-bee03403e9fc" />
An example of a PC emulator running...

<img width="1280" height="720" alt="pc-usb-telemetry-gif-2 (1)" src="https://github.com/user-attachments/assets/bf07fc5c-db62-45e7-9f39-0062868a55b2" />
An example of demonstrating the operation of USB telemetry...

<img width="692" height="388" alt="Wifi Telemetry debug gif" src="https://github.com/user-attachments/assets/d377ac5e-32cf-4840-8d96-3b35743a3b31" />


An example of a demonstration of Wi-Fi telemetry...


## 📖 Documentation & Guides

[usb_debugging_guide.md](https://github.com/user-attachments/files/28191282/usb_debugging_guide.md)

[wifi_debugging_guide.md](https://github.com/user-attachments/files/28191284/wifi_debugging_guide.md)

### 3. Smart Canvas & Routing
 **Orthogonal Auto-Routing:** The "Smart Anchors" mathematically calculate optimal midpoints to prevent arrow overlapping. 
 **Pre-flight Linter:** Built-in Abstract Syntax Tree (AST) scanning and compiler integration (`g++ -fsyntax-only`) to catch dead ends, syntax mixing, or division by zero *before* compilation.

### 4. Remote Library Ecosystem
Extend your capabilities dynamically. The built-in Network Manager allows you to fetch and install `.json` FSM modules from an online catalog directly into your workspace.


## 🚀 Quick Start

1. **Configure Toolchains:** Go to `Project Settings` -> `Toolchains & Compilers` and link your system's executable (`g++`, `python`, or `javac`).
2. **Draw Logic:** Add States, define `OnEnter`/`OnExit` routines, and connect them with conditional Transitions.
3. **Generate:** Click `Generate SCXML` and select your target language. The IDE will perform "Variable Hoisting" to resolve scopes and output clean code.
4. **Debug:** Hit the `Debug` button to trace your logic visually.

## 🧮 Core Architecture & Mathematical Model

This document outlines the fundamental mathematical models and graph theory algorithms underlying the **VisualBuildStateMachine** engine. The core's primary goal is to translate visual Statecharts into strictly deterministic, platform-independent C++ code **without** relying on a Real-Time Operating System (RTOS).

### 1. Flat Automaton Model & $O(1)$ Dispatch (Single FSM)
A classical Deterministic Finite Automaton (DFA) is defined by the 5-tuple:
$$M = \langle S, \Sigma, \delta, s_0, F \rangle$$

  **The Problem:** In classical switch-case implementations, computing the next state $\delta(s_i, \sigma)$ takes $O(|S|)$ time due to sequential condition evaluations.
  **Our Solution:** The engine transforms the set of visual states $S$ into an injective mapping of memory pointers $P$: $f: S \to P$. 
By dereferencing function pointers directly, the computational complexity of any state transition becomes strictly constant: **$T(\delta) = O(1)$**. This guarantees absolute hardware determinism regardless of graph size.

### 2. Deterministic Parallelism (Orthogonal Regions)
  **The Problem:** Running multiple state machines with $|S_1|$ and $|S_2|$ states causes a combinatorial explosion ($|S_1| \times |S_2|$ states).
  **Our Solution:** We implement tokenized concurrency without an RTOS. Let $n$ be the number of active processes. The global system state at time $t$ is described by a vector:
$$\vec{A}(t) = [a_1(t), a_2(t), \dots, a_n(t)]$$
The system update function per CPU tick unites isolated transitions:
$$\vec{A}(t+1) = \bigcup_{i=1}^{n} \delta_i(a_i(t), \Sigma)$$
Since each element is evaluated independently, **Data Races are mathematically impossible**. The Worst-Case Execution Time (WCET) is strictly bounded: **$T_{tick} = O(n)$**.

### 3. Graph Optimization (Dead Code Elimination)
The compiler solves the topological routing of the directed graph $G = (V, E)$.
  **Unreachable States Pruning (BFS):** A state $v \in V$ is compiled only if a path from the start node $v_0$ exists: $\forall v \in V_{compiled}, \exists \text{ path } p: v_0 \leadsto v$.
  **Dead Edge Stripping:** If an outgoing edge $e_1 = (u, v)$ has an unconditional truth $C(e_1) \equiv \text{True}$, any other edges $e_2$ with lower priority are algorithmically stripped from the generated firmware to save Flash memory.

### 4. Topological Memory (History Block $H^*$)
To implement David Harel's history pseudo-states, we introduce a context capture function.
For a macro-state $G \subset S$, the engine defines a memory function $H$:
$$H: G \to S_{internal}$$
Upon token exit, the state is saved: $H(G) = a_i(t)$. Upon return via the History block, the transition is evaluated dynamically:
  $s_{next} = H(G)$ if $H(G) \neq \emptyset$
  $s_{next} = s_{default}$ if $H(G) = \emptyset$

### 5. Super States & Hierarchy Flattening
A Super State $G$ is a subset $S_{sub} \subset S$. If a global outgoing transition $E(G, \Sigma) \to s_{target}$ exists, the compiler applies priority preemption to all inner blocks:
$$\forall s \in S_{sub}, \delta(s, \sigma_{global}) = s_{target}$$
During translation, this visual hierarchy is mathematically **flattened** into a 1D pointer array, maintaining $O(1)$ dispatch.

### 6. Virtual Routing (Bridge Blocks)
  **The Problem:** "Spaghetti code" from overlapping visual connections.
  **Our Solution:** The Bridge Block $B$ acts as a teleportation node. The compiler generates a surjective mapping $\phi: B \to v_{target}$. During translation, the transit edge $(u, B)$ is dynamically replaced with a direct edge:
$$E_{compiled} = (E_{vis} \setminus \{(u, B)\}) \cup \{(u, v_{target})\}$$
The Bridge Block is physically erased from the C++ code, leaving a direct pointer.

### 7. Stackless Dynamic Return (Return Blocks)
  **The Problem:** Standard subroutine calls cause Stack Overflow on weak MCUs.
  **Our Solution:** The engine acts as a Pushdown Automaton with a fixed stack depth of 1.
When transitioning from $s_{caller}$ to a subroutine, the return address is saved: $R \leftarrow s_{caller}$. Upon hitting the Return Block ($B_{return}$), the target is resolved via the saved register: $\delta(B_{return}, \sigma) = R$.
This enables reusable functions with **$O(1)$ RAM overhead**.


## ⚖️ License & Usage Rights
VisualBuildStateMachine Creator is open-source software distributed under the GNU GPL v3.0 license. Please note the following key licensing specifics:

 **Generated Code Ownership:** The generated source code belongs entirely to you (the user). It is not subject to the GPL restrictions of the IDE. You are free to use your generated code in any private, commercial, or closed-source projects.
 **Scope of License:** The GNU GPL v3.0 license applies strictly to the IDE software itself. 
 **Provided "As Is":** This software is provided "as is", without warranty of any kind.
 **Forking and Modifications:** You are free to use, modify, fork, and distribute the IDE software. 
 **No Closed-Source Commercialization:** You may not close the source code of the IDE. Any modified versions or forks of this program must also be open-sourced and distributed under the same GPLv3 license terms.
