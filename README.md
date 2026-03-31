# 🏙️ Urban Waste Management — Smart City Animation

A professional-grade, high-fidelity 2D animation built purely in **C** using the **OpenGL/GLUT** fixed-function pipeline. This project simulates a 5-minute journey from a modern waste crisis to a zero-waste smart city future, featuring advanced procedural animation, custom particle systems, and cinematic visual effects.

---

## 🚀 Quick Start

### Prerequisites
- **Windows**: MinGW-w64 with `freeglut` installed.
- **Linux**: `freeglut3-dev`, `libglu1-mesa-dev`, and `mesa-common-dev`.

### Compilation

**Windows (MinGW):**
```bash
gcc waste_management.c -o waste_management -lfreeglut -lglu32 -lopengl32 -lm
```

**Linux (GCC):**
```bash
gcc waste_management.c -o waste_management -lglut -lGLU -lGL -lm
```

### Controls
| Key | Action |
| :--- | :--- |
| `P` | Pause / Resume |
| `R` | Restart Animation |
| `1-7` | Jump to Specific Scene |
| `8` | Reset to Start |
| `Q` / `Esc` | Quit |

---

## 🎭 Scene Breakdown

The animation is divided into 8 distinct cinematic segments:

1.  **S0: Intro** — Typewriter text reveal with a "breathing" space background and orbital energy particles.
2.  **S1: Waste Crisis** — A grimy city scene featuring multi-layer smog, scurrying rats, and flickering neon signs.
3.  **S2: IoT Network** — Visualization of a smart bin mesh network with radar sweeps and glowing data packets.
4.  **S3: Collection Routes** — Automated trucks navigate traffic-light controlled intersections with suspension bounce and GPS trails.
5.  **S4: Sorting Facility** — AI-driven sorting belt with computer vision bounding boxes, safety beacons, and machine vibration.
6.  **S5: Waste-to-Energy** — An evening industrial scene with incinerator embers, turbine glows, and high-voltage energy flow.
7.  **S6: City Dashboard** — Real-time analytics view with animated filling gauges and progressive line graphs.
8.  **S7: Zero-Waste Future** — A lush, green eco-city featuring wind turbines, bird flocks, and atmospheric rainbows.

---

## 🛠️ Technical Architecture

### Core Visual Engine
Despite using the legacy fixed-function pipeline (`glBegin`/`glEnd`), the project implements several "modern-feel" abstractions:
- **Additive Glow Nodes**: Multi-layered radial gradients for localized light sources.
- **Soft Shadows**: Elliptical alpha-blended shadows for object depth perception.
- **Cinematic Vignette**: Darkened screen corners to focus the viewer's attention.
- **Scene Fades**: Global alpha-blending for smooth transitions between segments.

### Procedural Animation System
Every element in the scene is driven by a global time variable (`gt`) and local scene time (`st`):
- **Secondary Motion**: Subtle wobbles on full bins, wind sway on trees, and belt vibration on sorting items.
- **Easing Functions**: Implementation of `smoothstep`, `ease_in_out`, and `bounce_out` for organic movement.
- **Camera Effects**: Handheld-style camera drift (parallax) and impact-based camera shake.

### Particle Engine
A light-weight, high-performance particle system supporting up to 600 concurrent particles:
- **Physics**: Per-particle drag coefficients and gravity multipliers.
- **Types**: Specialized rendering for Smoke, Sparks (with motion trails), Leaves (erratic float), Energy (glow-trails), and Embers.

---

## 📁 File Structure
- `waste_management.c`: The entire application source code.
---

## ⚖️ Technical Constraints
This project was developed with a strict "No External Assets" policy. All visuals—from the city skyline to the birds' wings—are procedurally drawn using mathematical primitives. This ensures zero dependencies and maximum cross-platform portability.

---

