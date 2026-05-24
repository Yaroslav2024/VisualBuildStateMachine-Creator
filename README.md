# ⚙️ VisualBuildStateMachine Creator
<img width="2048" height="2064" alt="VisualBuildStateMachine Creator icon" src="https://github.com/user-attachments/assets/39e0b768-1548-4bae-a87f-7a51290f32eb" />

![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Qt](https://img.shields.io/badge/Qt-%23217346.svg?style=for-the-badge&logo=Qt&logoColor=white)
![Windows](https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white)

**VisualBuildStateMachine Creator** — is a native desktop application designed for visual modeling of Finite State Machines (FSM) and automatic source code generation.

⚠️ **System Requirements:**
* **OS:** Windows 64-bit (Windows 32-bit is strictly NOT supported).

## ✨ Available Code Generation Targets
The application supports instant generation of optimized code for the following platforms and languages:
* **C Micro** (built-in support for Arduino / microcontrollers)
* **C++ Desktop**
* **Python**
* **Java**

## 🔥 Core Features

### 1. Embedded-First Architecture (No RTOS Required)
VisualBuild is designed with strict embedded constraints in mind:
* **Zero Dynamic Allocation:** The generated FSM relies strictly on static memory. No `malloc` or `new` during runtime, completely eliminating heap fragmentation and Memory Leaks.
* **Mathematical Parallelism:** Running multiple concurrent states? The compiler intelligently switches from scalar routing to vector routing, allowing true logical parallelism on single-core microcontrollers *without* the overhead of an RTOS. 
* **Safe Self-Loops:** Implements a localized `transition_taken` flag to handle "self-to-self" transitions flawlessly, which is critical for non-blocking timers and sensor polling.

### 2. Live Hardware & PC Debugging
Don't just write code—watch it execute in real-time.
* **Hardware Debugger:** Inject lightweight telemetry (`[B:id]`, `[T:id]`) directly into your microcontroller via USB (Serial) or over-the-air via Wi-Fi (TCP/IP). Watch the active nodes light up green/red on the canvas.
* **PC Emulator:** Run your generated logic locally within the IDE's virtual machine. No external hardware required.

<img width="1280" height="720" alt="Untitled" src="https://github.com/user-attachments/assets/a0a14ac2-30f8-4039-8e0f-bee03403e9fc" />
An example of a PC emulator running

### 3. Smart Canvas & Routing
* **Orthogonal Auto-Routing:** The "Smart Anchors" mathematically calculate optimal midpoints to prevent arrow overlapping. 
* **Pre-flight Linter:** Built-in Abstract Syntax Tree (AST) scanning and compiler integration (`g++ -fsyntax-only`) to catch dead ends, syntax mixing, or division by zero *before* compilation.

### 4. Remote Library Ecosystem
Extend your capabilities dynamically. The built-in Network Manager allows you to fetch and install `.json` FSM modules from an online catalog directly into your workspace.

## 🚀 Quick Start

1. **Configure Toolchains:** Go to `Project Settings` -> `Toolchains & Compilers` and link your system's executable (`g++`, `python`, or `javac`).
2. **Draw Logic:** Add States, define `OnEnter`/`OnExit` routines, and connect them with conditional Transitions.
3. **Generate:** Click `Generate SCXML` and select your target language. The IDE will perform "Variable Hoisting" to resolve scopes and output clean code.
4. **Debug:** Hit the `Debug` button to trace your logic visually.

## ⚖️ License & Usage Rights
VisualBuildStateMachine Creator is open-source software distributed under the GNU GPL v3.0 license. Please note the following key licensing specifics:

* **Generated Code Ownership:** The generated source code belongs entirely to you (the user). It is not subject to the GPL restrictions of the IDE. You are free to use your generated code in any private, commercial, or closed-source projects.
* **Scope of License:** The GNU GPL v3.0 license applies strictly to the IDE software itself. 
* **Provided "As Is":** This software is provided "as is", without warranty of any kind.
* **Forking and Modifications:** You are free to use, modify, fork, and distribute the IDE software. 
* **No Closed-Source Commercialization:** You may not close the source code of the IDE. Any modified versions or forks of this program must also be open-sourced and distributed under the same GPLv3 license terms.
