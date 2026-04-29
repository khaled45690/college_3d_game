# Pepsi Man 3D

A 3D endless runner / dodge game built with OpenGL, GLUT, and C++17.  
Run down a procedurally generated city road, dodge roadblocks, and navigate through a fully lit 3D environment with real FBX buildings and characters.

---

## Table of Contents

- [Screenshots](#screenshots)
- [Controls](#controls)
- [Project Structure](#project-structure)
- [Dependencies](#dependencies)
- [Building on Linux](#building-on-linux)
- [Building on Windows](#building-on-windows)
- [VSCode Setup](#vscode-setup)
- [Compile Command Reference](#compile-command-reference)

---

## Controls

| Key | Action |
|-----|--------|
| `W` / `S` | Zoom camera in / out |
| `8` / `5` | Zoom in / out (numpad) |
| `4` / `6` | Rotate camera left / right |
| `2` / `9` | Tilt camera down / up |
| `R` | Reset camera |
| `ESC` | Exit game |
| `↑ ↓` | Navigate menu |
| `Enter` | Confirm menu selection |

---

## Project Structure

```
pepsi_man/
├── src/
│   ├── main.cpp          — Entry point, scene setup, GLUT callbacks
│   ├── Scene.cpp/.h      — Scene manager (OBJ + FBX models, road)
│   ├── Model.cpp/.h      — OBJ/MTL loader (tinyobjloader + stb_image)
│   ├── AssimpModel.cpp/.h— FBX/DAE/GLTF loader (Assimp)
│   ├── Road.cpp/.h       — Procedural road geometry
│   └── Menu.cpp/.h       — Main menu (OpenCV background image)
├── assets/
│   └── models/           — OBJ, MTL, and texture files
├── objects/
│   ├── Free_Building/    — FBX building + textures
│   ├── fbx_Clean/        — Character FBX
│   └── animations/       — (place animated FBX files here)
├── include/              — stb_image, tinyobjloader headers
└── README.md
```

---

## Dependencies

| Library | Purpose | Version |
|---------|---------|---------|
| **OpenGL** | Core 3D rendering | system |
| **GLUT / FreeGLUT** | Window, input, main loop | 3.x |
| **GLM** | Math (vectors, matrices) | 0.9.9+ |
| **OpenCV** | Menu background image | 4.x |
| **Assimp** | FBX / DAE / GLTF loading | 5.x+ |

> OpenCV is used minimally — only `core`, `imgcodecs`, `highgui`, and `imgproc` modules are linked.

---

## Building on Linux

### Arch / CachyOS / Manjaro

```bash
# Install all dependencies
sudo pacman -S mesa freeglut glm opencv assimp
```

### Ubuntu / Debian / Pop!_OS

```bash
sudo apt update
sudo apt install build-essential libgl1-mesa-dev freeglut3-dev \
                 libglm-dev libopencv-dev libassimp-dev
```

### Fedora

```bash
sudo dnf install mesa-libGL-devel freeglut-devel glm-devel \
                 opencv-devel assimp-devel
```

### Compile and Run

```bash
# Clone / navigate to project folder
cd pepsi_man

# Compile
g++ -std=c++17 -I./include -I/usr/include/opencv4 \
    src/main.cpp src/Model.cpp src/Scene.cpp src/Road.cpp \
    src/AssimpModel.cpp src/Menu.cpp \
    -lGL -lGLU -lglut -lm \
    -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc \
    -lassimp \
    -o pepsi_man

# Run
./pepsi_man
```

---

## Building on Windows

### Option A — MSYS2 + MinGW-w64 (Recommended)

**Step 1 — Install MSYS2**

Download and install MSYS2 from https://www.msys2.org  
Open the **MSYS2 MinGW 64-bit** terminal.

**Step 2 — Install dependencies**

```bash
pacman -S mingw-w64-x86_64-toolchain
pacman -S mingw-w64-x86_64-freeglut
pacman -S mingw-w64-x86_64-glm
pacman -S mingw-w64-x86_64-opencv
pacman -S mingw-w64-x86_64-assimp
```

**Step 3 — Compile**

```bash
cd /c/Users/YourName/pepsi_man

g++ -std=c++17 -I./include -IC:/msys64/mingw64/include/opencv4 \
    src/main.cpp src/Model.cpp src/Scene.cpp src/Road.cpp \
    src/AssimpModel.cpp src/Menu.cpp \
    -lGL -lGLU -lfreeglut -lm \
    -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc \
    -lassimp \
    -o pepsi_man.exe
```

**Step 4 — Run**

```bash
./pepsi_man.exe
```

> If you get a missing `.dll` error on launch, copy the required DLLs from  
> `C:\msys64\mingw64\bin\` into the same folder as `pepsi_man.exe`.  
> Required DLLs: `libfreeglut.dll`, `libopencv_*.dll`, `libassimp*.dll`, `libgcc_s_seh-1.dll`

---

### Option B — vcpkg + MSVC (Visual Studio)

**Step 1 — Install vcpkg**

```powershell
git clone https://github.com/microsoft/vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

**Step 2 — Install packages**

```powershell
.\vcpkg install freeglut:x64-windows
.\vcpkg install glm:x64-windows
.\vcpkg install opencv4[core,highgui,imgcodecs,imgproc]:x64-windows
.\vcpkg install assimp:x64-windows
```

**Step 3 — Compile via Developer Command Prompt**

```cmd
cl /std:c++17 /EHsc /I.\include /I<vcpkg>\installed\x64-windows\include \
   src\main.cpp src\Model.cpp src\Scene.cpp src\Road.cpp \
   src\AssimpModel.cpp src\Menu.cpp \
   /link opengl32.lib glu32.lib freeglut.lib assimp.lib \
         opencv_core4.lib opencv_imgcodecs4.lib opencv_highgui4.lib opencv_imgproc4.lib \
   /OUT:pepsi_man.exe
```

---

## VSCode Setup

### 1. Install Extensions

Open VSCode and install:
- **C/C++** by Microsoft (`ms-vscode.cpptools`)
- **C/C++ Extension Pack** (`ms-vscode.cpptools-extension-pack`)

### 2. c_cpp_properties.json (Linux)

Create `.vscode/c_cpp_properties.json`:

```json
{
    "configurations": [
        {
            "name": "Linux",
            "includePath": [
                "${workspaceFolder}/include",
                "${workspaceFolder}/src",
                "/usr/include/opencv4",
                "/usr/include/GL"
            ],
            "compilerPath": "/usr/bin/g++",
            "cppStandard": "c++17",
            "intelliSenseMode": "linux-gcc-x64"
        }
    ],
    "version": 4
}
```

### 3. tasks.json — Build Task

Create `.vscode/tasks.json`:

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build Pepsi Man",
            "type": "shell",
            "command": "g++",
            "args": [
                "-std=c++17",
                "-I./include",
                "-I/usr/include/opencv4",
                "src/main.cpp",
                "src/Model.cpp",
                "src/Scene.cpp",
                "src/Road.cpp",
                "src/AssimpModel.cpp",
                "src/Menu.cpp",
                "-lGL", "-lGLU", "-lglut", "-lm",
                "-lopencv_core",
                "-lopencv_imgcodecs",
                "-lopencv_highgui",
                "-lopencv_imgproc",
                "-lassimp",
                "-o", "pepsi_man"
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": "$gcc"
        }
    ]
}
```

Press `Ctrl+Shift+B` to build from VSCode.

### 4. launch.json — Debug / Run

Create `.vscode/launch.json`:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Run Pepsi Man",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/pepsi_man",
            "args": [],
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": true,
            "MIMode": "gdb",
            "preLaunchTask": "Build Pepsi Man"
        }
    ]
}
```

Press `F5` to build and run with the debugger.

---

## Compile Command Reference

### Linux — one-liner

```bash
g++ -std=c++17 -I./include -I/usr/include/opencv4 \
    src/main.cpp src/Model.cpp src/Scene.cpp src/Road.cpp \
    src/AssimpModel.cpp src/Menu.cpp \
    -lGL -lGLU -lglut -lm \
    -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc \
    -lassimp -o pepsi_man
```

### Linux — with debug symbols

```bash
g++ -std=c++17 -g -I./include -I/usr/include/opencv4 \
    src/main.cpp src/Model.cpp src/Scene.cpp src/Road.cpp \
    src/AssimpModel.cpp src/Menu.cpp \
    -lGL -lGLU -lglut -lm \
    -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc \
    -lassimp -o pepsi_man_debug
```

### Linux — optimised release build

```bash
g++ -std=c++17 -O2 -I./include -I/usr/include/opencv4 \
    src/main.cpp src/Model.cpp src/Scene.cpp src/Road.cpp \
    src/AssimpModel.cpp src/Menu.cpp \
    -lGL -lGLU -lglut -lm \
    -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc \
    -lassimp -o pepsi_man
```

---

## Troubleshooting

**`fatal error: opencv2/opencv.hpp: No such file or directory`**  
→ Add `-I/usr/include/opencv4` to your compile command. Do **not** use `pkg-config --libs opencv4` — it pulls in VTK/HDF5 modules that are unneeded and may fail to link.

**`error while loading shared libraries: libassimp.so`**  
→ Run `sudo ldconfig` after installing Assimp.

**Black screen on launch**  
→ Make sure you run the binary from the project root (`./pepsi_man`), not from inside `src/`. Asset paths are relative to the working directory.

**Model not found / failed to load**  
→ Check that asset paths in `main.cpp` match your folder layout. All paths are relative to where you run the binary from.
