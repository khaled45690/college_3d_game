# Pepsi Man 3D Project Plan

## 1. Project Overview

Build a 3D game/animation inspired by "Pepsi Man" using free 3D models from TurboSquid and the OpenGL/GLUT stack. The project will feature a 3D character moving through a scene with obstacles and collectibles, demonstrating 3D model loading, rendering, animation, and basic game mechanics. This is a one-time college project with a focus on simplicity over production-grade architecture.

**Goal**: Create a playable 3D scene where a Pepsi Man-style character navigates an environment, collects items, and avoids obstacles.

---

## 2. Recommended Tech Stack (Windows / MinGW)

Beyond your chosen GLUT and OpenGL, here are essential and optional libraries to make development easier. All instructions target **Windows with MinGW compiler** (used by Code::Blocks IDE).

### Core Libraries (Highly Recommended)

#### GLM (OpenGL Mathematics)
- **What**: Header-only C++ math library for vector, matrix, and quaternion operations
- **Why**: Makes vertex transformations, rotations, and camera math trivial. Essential for 3D graphics
- **Install on Windows**: 
  - Download from https://github.com/g-truc/glm/releases
  - Extract the zip file to your project's `include/` folder
  - No compilation needed; just add to your compiler's include path

#### GLEW (OpenGL Extension Wrangler Library)
- **What**: Simplifies OpenGL extension loading; handles version differences
- **Why**: GLUT alone doesn't handle modern OpenGL features. GLEW bridges this gap
- **Install on Windows**:
  - Download prebuilt MinGW binaries from http://glew.sourceforge.net/
  - Extract files:
    - `glew32.dll` → copy to your project root (next to the .exe when built)
    - `lib/libglew32.a` → copy to a folder like `C:\dev\glew\lib\` (or your project's `lib/`)
    - `include/GL/glew.h` → copy to your project's `include/GL/` folder
  - In Code::Blocks: Add the lib path to **Project → Build Options → Linker Settings → Search directories**

#### Freeglut for Windows (GLUT Implementation)
- **What**: Cross-platform GLUT implementation; required for windowing and input
- **Why**: Standard GLUT is outdated; freeglut is actively maintained and includes modern extensions
- **Install on Windows**:
  - Download prebuilt MinGW package from https://www.transmissionzero.co.uk/software/freeglut-devel/
  - Extract files:
    - `freeglut.dll` → copy to your project root (next to the .exe when built)
    - `lib/libfreeglut.a` → copy to a folder like `C:\dev\freeglut\lib\` (or your project's `lib/`)
    - `include/GL/freeglut.h` → copy to your project's `include/GL/` folder
  - In Code::Blocks: Add the lib path to **Project → Build Options → Linker Settings → Search directories**

#### tinyobjloader (Recommended for Easiest Path)
- **What**: Single-header C++ .obj file parser
- **Why**: Zero dependencies, ~50 lines of code to load a complete model. Perfect for a college project
- **Install on Windows**: Download header from https://github.com/tinyobjloader/tinyobjloader and drop into your `include/` folder (no package needed)
- **Alternative**: Assimp (below) if you need advanced features

#### Assimp (Alternative: More Features)
- **What**: Open Asset Import Library; loads .obj, .fbx, .dae, .gltf, and 20+ other 3D formats
- **Why**: More robust than tinyobjloader; handles materials, triangulation, and transforms automatically
- **Install on Windows**: 
  - **Easiest**: Download prebuilt MinGW binaries from https://github.com/assimp/assimp/releases
  - **Alternative**: Use vcpkg: `vcpkg install assimp:x86-windows` (if vcpkg is installed)
  - Extract and place `.dll`, `.a`, and headers in your project structure (similar to GLEW/freeglut)
- **Trade-off**: More powerful but heavier; slower to compile and learn. For a college project using only .obj files, **tinyobjloader is strongly recommended instead**

#### stb_image (Single-Header Image Loader)
- **What**: Lightweight image loading library (PNG, JPG, BMP, etc.)
- **Why**: Load texture images without external dependencies
- **Install on Windows**: Download from https://github.com/nothings/stb and drop into `include/` folder (no compilation)

### Optional Libraries

#### GLFW3 (Modern Window Handling)
- **What**: Modern alternative to GLUT; handles windowing, input, and context creation
- **Why**: GLUT is outdated; GLFW is lighter and more flexible
- **Install on Windows**: 
  - Download prebuilt MinGW binaries from https://www.glfw.org/download.html
  - Extract and set up the same way as freeglut
- **Note**: If you use GLFW, you can drop GLUT, but stick with GLUT/freeglut if you want minimal setup

#### Bullet Physics (Physics Simulation)
- **What**: Real-time physics engine for collision detection and rigid body dynamics
- **Why**: Optional; only if you want realistic physics for character movement and collisions
- **Install on Windows**: Similar download/extract process as Assimp; prebuilt MinGW binaries available
- **Easiest path**: Skip this; use simple AABB (Axis-Aligned Bounding Box) collision checks instead

#### OpenCV (You Already Planned This)
- **What**: Computer vision library; useful for image processing and animation frame capture
- **Why**: Great for recording gameplay or post-processing scenes. Use sparingly to keep project scope manageable
- **Install on Windows**: 
  - **Note**: OpenCV's MinGW support is limited. Options:
    1. Download prebuilt MinGW binaries from https://opencv.org/releases/ (if available for your MinGW version)
    2. Build from source (time-consuming; not recommended for a college project)
    3. Use MSVC prebuilt binaries if you can compile with MSVC instead of MinGW
  - **Recommendation**: If you don't strictly need OpenCV, skip it. Use native Windows APIs or a simpler library for recording

### Recommended Path for Maximum Ease

```
Core: freeglut + OpenGL + GLEW + GLM + tinyobjloader + stb_image
Optional: OpenCV (only if needed for recording; see warning above), GLFW (if you want to replace freeglut)
Skip: Assimp (unless you need FBX support), Bullet Physics (use simple collision checks)
```

---

## 3. How to Get 3D Models from TurboSquid

### Step-by-Step: Downloading Free Models from TurboSquid

1. **Navigate to TurboSquid Free Section**
   - Go to https://www.turbosquid.com/?dd_referrer=
   - Use filter: `Price: Free` (or add `?max_price=0` to URL)
   - Filter by `File Format: OBJ` or `FBX`

2. **Create a Free Account** (if you don't have one)
   - Click "Sign Up"
   - Use your email and create a password
   - No payment method required for free downloads

3. **Download the Model**
   - Click on a model you like
   - Click "Download" or "Get Model"
   - A `.zip` file will download

4. **Extract and Organize**
   - Unzip the file; typical structure:
     ```
     model_name/
       model_name.obj          (geometry data)
       model_name.mtl          (material definitions)
       textures/
         diffuse.jpg           (color)
         normal.png            (normal map, optional)
         roughness.png         (PBR, optional)
     README.txt                (license info)
     ```
   - Move all files to your `assets/models/` folder
   - Keep `.obj`, `.mtl`, and texture files in the same directory (or update .mtl paths)

5. **License Considerations**
   - Read the model's license (usually in `README.txt`)
   - TurboSquid Standard License: allows educational use, but check specific model terms
   - For college projects, you're usually safe; include a `CREDITS.txt` in your submission citing model sources

### Alternative Free Model Sources

If TurboSquid's free selection is limited:

- **Mixamo (Highly Recommended for Characters)**: https://www.mixamo.com/
  - Free rigged and animated human characters
  - Includes pre-made walking, running, jumping animations
  - Best source for a Pepsi Man-style character
  - Requires free Adobe ID login

- **Sketchfab**: https://sketchfab.com/
  - Filter by "Downloadable" and "Free"
  - Many high-quality models in multiple formats

- **Free3D**: https://free3d.com/
  - Large collection of free models
  - Good for environment props

- **CGTrader Free Section**: https://www.cgtrader.com/free-3d-models
  - Professional models, free tier

- **Poly Haven**: https://polyhaven.com/models
  - High-quality, CC0 licensed
  - Excellent for props and environments

- **Quaternius**: https://quaternius.com/
  - Low-poly, stylized models (great for games)
  - Perfect aesthetic for a Pepsi Man game

### Finding a Character Model Specifically

For a Pepsi Man humanoid character:
- **Best Option**: Download a free rigged character from Mixamo (walking/running animations included)
- **Alternative**: Find a generic human base-mesh from Sketchfab or Free3D and texture it as Pepsi Man (blue/red colors)
- **DIY Route**: Model a simple character in Blender (overkill for this project, but possible)

---

## 4. Loading .obj Files in OpenGL

### Option A: tinyobjloader (Recommended for Easiest Path)

#### Why Choose tinyobjloader?
- Single header file; no compilation needed
- ~50 lines of code to load a complete model
- Perfect for college projects and quick prototyping
- Lightweight; no external dependencies

#### Basic Structure

```cpp
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

// In your Model class:
void loadModel(const std::string& filepath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str());
    
    if (!warn.empty()) std::cout << "Warning: " << warn << std::endl;
    if (!ret) throw std::runtime_error(err);
    
    // Extract vertices, normals, texCoords from attrib and shapes
    // Create VBO/VAO and upload to GPU
    // Store mesh data for rendering
}
```

#### What tinyobjloader Provides
- **attrib**: vertex positions, normals, texture coordinates
- **shapes**: groups of faces (triangles)
- **materials**: material names and references

#### Workflow
1. Parse .obj and .mtl files
2. Extract vertex data (positions, normals, UVs)
3. Load texture images using stb_image
4. Create OpenGL Vertex Buffer Objects (VBOs) and Vertex Array Objects (VAOs)
5. Upload data to GPU
6. Render using glDrawArrays/glDrawElements

---

### Option B: Assimp (More Features, More Complexity)

#### Why Choose Assimp?
- Supports .obj, .fbx, .dae, .gltf, and 20+ formats
- Automatic triangulation and mesh optimization
- Built-in transformation and animation support
- Better for production-quality code

#### Trade-offs
- Heavier library; slower to compile
- Steeper learning curve
- Overkill for a college project using only .obj files

#### Basic Structure

```cpp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

void loadModel(const std::string& filepath) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filepath,
        aiProcess_Triangulate | aiProcess_FlipWindingOrder);
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        throw std::runtime_error("Assimp loading error");
    }
    
    // Iterate through meshes, materials, and load to GPU
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];
        // Extract and upload vertex data
    }
}
```

---

### Simple Rendering Pipeline (Fixed-Function)

If shaders feel like too much work, use GLUT's fixed-function pipeline:

```cpp
void renderModel() {
    glBegin(GL_TRIANGLES);
    for (const auto& vertex : vertices) {
        glNormal3f(vertex.normal.x, vertex.normal.y, vertex.normal.z);
        glTexCoord2f(vertex.texCoord.x, vertex.texCoord.y);
        glVertex3f(vertex.pos.x, vertex.pos.y, vertex.pos.z);
    }
    glEnd();
}
```

**Trade-off**: Less performant but minimal code; fine for a college project with small models.

---

### Modern Rendering Pipeline (Shaders)

For better performance and quality:

```cpp
// Vertex Shader
#version 120
varying vec2 texCoord;
void main() {
    texCoord = gl_MultiTexCoord0;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}

// Fragment Shader
#version 120
uniform sampler2D diffuseTexture;
varying vec2 texCoord;
void main() {
    gl_FragColor = texture2D(diffuseTexture, texCoord);
}
```

---

## 5. Project Structure

Organize your files like this:

```
pepsi-man/
├── assets/
│   ├── models/
│   │   ├── pepsi_character.obj
│   │   ├── pepsi_character.mtl
│   │   ├── can.obj
│   │   ├── can.mtl
│   │   ├── obstacle.obj
│   │   ├── textures/
│   │   │   ├── character_diffuse.jpg
│   │   │   ├── character_normal.png
│   │   │   ├── can_diffuse.jpg
│   │   └── environment.obj
│   ├── textures/
│   │   └── (additional texture files if needed)
│   └── sounds/ (optional)
│       ├── bgm.wav
│       └── collect.wav
├── src/
│   ├── main.cpp                 (entry point, GLUT setup)
│   ├── Model.cpp / Model.h      (tinyobjloader wrapper)
│   ├── Shader.cpp / Shader.h    (if using shaders)
│   ├── Camera.cpp / Camera.h    (camera controls)
│   ├── Player.cpp / Player.h    (character logic)
│   ├── Game.cpp / Game.h        (game state, score, etc.)
│   └── Scene.cpp / Scene.h      (scene graph, rendering)
├── include/
│   ├── tiny_obj_loader.h
│   ├── stb_image.h
│   ├── GL/
│   │   ├── glew.h
│   │   └── freeglut.h
│   └── glm/                     (GLM headers)
├── lib/                         (MinGW libraries)
│   ├── libfreeglut.a
│   ├── libglew32.a
│   └── (other .a files)
├── PepsiMan.cbp                 (Code::Blocks project file)
├── CMakeLists.txt               (optional; alternative to .cbp)
├── README.md
├── CREDITS.txt                  (model attributions)
└── PROJECT_PLAN.md              (this file)
```

**Note on DLL files**: Place `freeglut.dll` and `glew32.dll` in the **project root** (or the same folder as the compiled `.exe`). Windows will look there first when the program starts.

---

## 6. Step-by-Step Implementation Roadmap

Break the project into manageable phases. Complete one phase before moving to the next.

### Phase 1: Project Setup & Hello Triangle
**Goal**: Verify that freeglut, OpenGL, GLEW, and Code::Blocks are working

- **Setup**:
  - Download and install Code::Blocks IDE (if not already installed)
  - Download freeglut, GLEW, and GLM prebuilt Windows packages
  - Extract headers and libraries to your project's `include/` and `lib/` folders
  - Place `freeglut.dll` and `glew32.dll` in your project root
- **Create Code::Blocks project**:
  - File → New → Project → Empty Project
  - Add `src/main.cpp` to the project
  - Project → Build Options → Compiler Settings → Add Flags: `-std=c++17`
  - Project → Build Options → Linker Settings → Search directories: add `lib/` folder path
  - Project → Build Options → Linker Settings → Other linker options: add `-lfreeglut -lopengl32 -lglu32 -lglew32`
  - Project → Build Options → Search directories (Compiler tab): add `include/` folder path
- **Write a simple main.cpp** that opens a window and renders a colored triangle
- **Success**: A triangle appears on screen

### Phase 2: Load and Render a Single .obj Model
**Goal**: Successfully load and display a 3D model

- Download a simple model from TurboSquid (e.g., a cube, sphere, or can)
- Integrate tinyobjloader into your project (copy `tiny_obj_loader.h` to `include/`)
- Write a Model class that loads .obj files
- Parse vertices, normals, and faces
- Create VBO/VAO and upload to GPU
- Render using fixed-function glBegin/glEnd or basic shaders
- **Success**: The model appears in your window (may look gray/flat)

### Phase 3: Add Textures and Materials
**Goal**: Make models look realistic with color and detail

- Integrate stb_image for texture loading (copy `stb_image.h` to `include/`)
- Extend Model class to load .mtl files
- Extract texture paths from .mtl
- Load texture images and bind to GPU
- Apply textures in fragment shader or fixed-function pipeline
- **Success**: Models have colors and surface detail

### Phase 4: Add Camera Controls
**Goal**: Navigate the scene with keyboard/mouse

- Implement Camera class (position, forward, up vectors)
- Handle GLUT keyboard callbacks for WASD movement
- Handle mouse movement for first-person view rotation
- Update view matrix before rendering
- **Success**: You can walk around and look around in the scene

### Phase 5: Load Character Model and Scene Props
**Goal**: Build a more complete scene

- Download a Pepsi Man-style character from Mixamo or Free3D
- Load character model and scale/position appropriately
- Load prop models: cans, obstacles, platforms
- Arrange them in your scene (hardcode positions or simple level file)
- **Success**: Scene contains character and objects

### Phase 6: Add Basic Character Movement
**Goal**: Move the character in the scene

- Extend Player class with position and velocity
- Implement basic movement: WASD or arrow keys translate character position
- Update character transform matrix before rendering
- Add simple idle/walk animation (if using Mixamo, load pre-made animations)
- **Success**: Character moves when you press keys

### Phase 7: Add Game Logic
**Goal**: Make it a playable game with objectives

- Implement collision detection (simple AABB checks)
- Add collectibles (cans) with pickup logic
- Add obstacles that block movement
- Implement win/lose conditions
- Track score and game state
- **Success**: You can collect items and lose when hitting obstacles

### Phase 8: Polish
**Goal**: Refine presentation and user experience

- Add lighting (directional light for environment)
- Add a simple menu screen (welcome, instructions, restart)
- Add sound effects (optional; use OpenAL or ALUT if needed)
- Add HUD (score, health, instructions on screen)
- Add particle effects for item collection (optional)
- **Success**: Game feels complete and playable

---

## 7. Build System: Code::Blocks Setup

### Creating a Code::Blocks Project (.cbp)

**Option A: Manual Setup via GUI (Recommended for Beginners)**

1. **File → New → Project → Empty Project**
2. **Add source files**: Right-click "Sources" → Add files → select all `.cpp` files
3. **Configure Compiler**:
   - Project → Build Options → Compiler: `MinGW (TDM-GCC)`
   - Set C++ Standard: `-std=c++17`
4. **Configure Include Paths**:
   - Project → Build Options → Search directories (Compiler tab)
   - Add: `C:\path\to\your\project\include` (absolute path)
5. **Configure Library Paths**:
   - Project → Build Options → Search directories (Linker tab)
   - Add: `C:\path\to\your\project\lib` (absolute path)
6. **Configure Linker Options**:
   - Project → Build Options → Linker Settings → Other linker options
   - Add: `-lfreeglut -lopengl32 -lglu32 -lglew32`
7. **Set Output Path**:
   - Project → Build Options → Output Filename: `bin/Release/pepsi-man.exe`
   - Ensure DLLs are in `bin/Release/` or project root
8. **Build and Run**: Build → Build and run (F9)

**Option B: Using CMakeLists.txt (More Advanced)**

If you prefer to use CMake and let it generate a Code::Blocks project:

```cmake
cmake_minimum_required(VERSION 3.10)
project(PepsiMan)

set(CMAKE_CXX_STANDARD 17)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

# Find libraries (Windows MinGW paths)
find_library(FREEGLUT freeglut PATHS ${CMAKE_SOURCE_DIR}/lib)
find_library(GLEW glew32 PATHS ${CMAKE_SOURCE_DIR}/lib)
find_library(OPENGL opengl32)
find_library(GLU glu32)

# Source files
set(SOURCES
    src/main.cpp
    src/Model.cpp
    src/Shader.cpp
    src/Camera.cpp
    src/Player.cpp
    src/Game.cpp
    src/Scene.cpp
)

# Create executable
add_executable(pepsi-man ${SOURCES})

# Link libraries (order matters on Windows)
target_link_libraries(pepsi-man
    ${FREEGLUT}
    ${GLEW}
    ${OPENGL}
    ${GLU}
)
```

**Build with CMake**:
```bash
mkdir build
cd build
cmake -G "CodeBlocks - MinGW Makefiles" ..
cmake --build .
```

This generates a `.cbp` file you can open in Code::Blocks.

### Linker Order (Windows)

**Important**: On Windows, linker order matters. Use this order in "Other linker options":
```
-lfreeglut -lopengl32 -lglu32 -lglew32
```

NOT:
```
-lopengl32 -lfreeglut -lglew32 -lglu32  (wrong order)
```

### Copying DLLs

After building, **copy `freeglut.dll` and `glew32.dll`** to the folder containing your `.exe`:
- If output is `bin/Release/pepsi-man.exe`, copy DLLs there
- If output is `pepsi-man.exe` in the project root, copy DLLs there

Without the DLLs, you'll get: **"The program can't start because freeglut.dll is missing"**

---

## 8. Tips for the "Easiest" Path (Windows Edition)

Since this is a one-time college project, take these shortcuts:

### Use tinyobjloader, Not Assimp
- Single header file, no build dependency or compilation
- ~100 lines of code vs. Assimp's complexity
- Plenty for .obj files
- No need to download MinGW binaries; just a header

### Use Fixed-Function Pipeline, Not Shaders
- `glBegin(GL_TRIANGLES) ... glEnd()` is simpler to debug
- Less boilerplate code
- GLUT was designed for this
- Trade-off: slower performance, but fine for a college project with small models

### Use Mixamo for Character Animation
- Free rigged character with built-in walking/running/jumping animations
- No need to rig or animate yourself
- Export as .obj or .fbx (load with Assimp if you need animations)

### Skip Physics; Use Simple Collision Checks
- Use AABB (Axis-Aligned Bounding Box) collision detection
- Just check: `if (abs(char_x - obstacle_x) < threshold && abs(char_z - obstacle_z) < threshold)`
- No need for Bullet Physics

### Pre-Bake Lighting into Textures
- Use textured models that already have lighting baked in
- No need to compute real-time lighting
- Simpler shaders or fixed-function pipeline

### Hardcode Game Values
- Don't build a config file system
- Hardcode level layouts, enemy positions, spawn points
- Makes code faster to write and debug

### One Big Render Loop, Not a Scene Graph
- Don't over-engineer; a single big `render()` function is fine
- Load models into memory once, render them each frame
- Optimize later if needed

### Put All Headers and DLLs in Your Project
- Don't try to install libraries system-wide or in `Program Files`
- Keep a simple structure: `include/`, `lib/`, DLLs in project root
- Makes it portable and easier to debug

---

## 9. Common Pitfalls and Solutions (Windows)

### Missing DLL at Runtime
- **Problem**: "freeglut.dll is missing" or "glew32.dll not found" when running the .exe
- **Solution**: Copy `freeglut.dll` and `glew32.dll` to the **same folder as your .exe**
  - If using Code::Blocks output folder, copy to `bin/Release/` (or wherever your .exe is built)
  - Check your Project → Build Options → Output Filename to see where the .exe goes
  - Don't install them system-wide; keep them next to the executable

### 32-bit vs 64-bit Library Mismatch
- **Problem**: Linker error like "undefined reference to ..." or runtime crash
- **Solution**: Ensure your MinGW compiler and libraries match:
  - Code::Blocks default MinGW is often **32-bit**
  - Download **32-bit prebuilt binaries** for freeglut, GLEW, etc.
  - Or: Check your Code::Blocks compiler version: Settings → Compiler → Toolchain executables
  - If it says `mingw32`, use 32-bit libraries; if `mingw64`, use 64-bit

### Using MSVC .lib Files with MinGW Compiler
- **Problem**: Linker doesn't recognize `.lib` files; expects `.a` (static) or `.dll` (shared)
- **Solution**: Always download **MinGW-compatible** binaries (ending in `.a`, not `.lib`)
  - Check the download page; look for "MinGW" or "GCC" labeled binaries
  - MSVC `.lib` files only work with the MSVC compiler, not MinGW

### Spaces in File Paths
- **Problem**: Linker can't find libraries if path contains spaces (e.g., `C:\Program Files\...`)
- **Solution**: 
  - Install libraries to a path without spaces: `C:\dev\freeglut\`, `C:\mingw\`, etc.
  - Or: In Code::Blocks, quote the path in linker options: `-L"C:\Program Files\..."` (add quotes)

### Wine Path Translation (If Running Through Wine)
- **Problem**: You're using Wine to run Code::Blocks on Linux, and Windows paths are confusing
- **Solution**:
  - Windows path like `C:\dev\freeglut\` maps to `~/.wine/drive_c/dev/freeglut/` in Wine
  - Relative paths (e.g., `./include/`, `./lib/`) are safer in Code::Blocks `.cbp` files
  - Use relative paths in your project settings when possible
  - DLL lookup still works the same: copy DLLs next to the .exe

### Include Path Issues
- **Problem**: Compiler says `GL/freeglut.h: No such file or directory`
- **Solution**:
  - Use `#include <GL/freeglut.h>` (note the `GL/` prefix), not `#include <freeglut.h>`
  - Ensure `include/GL/` folder exists in your project with `freeglut.h` inside
  - In Code::Blocks, add `include/` to compiler search directories (not `include/GL/`)
  - Code::Blocks automatically searches subdirectories (GL/, glm/, etc.)

### .obj and .mtl Path Issues
- **Problem**: Model doesn't load; "mtl file not found"
- **Solution**: .obj files reference .mtl by relative path. Keep them in the same directory or update the .obj file to point to correct path
- **Example**: In the .obj file, change `mtllib model.mtl` to `mtllib ./textures/model.mtl` if needed

### Texture Coordinates Flipped Vertically
- **Problem**: Textures appear upside-down
- **Solution**: OpenGL expects (0,0) at bottom-left; images usually have (0,0) at top-left
  - Option 1: Flip textures in image editor before loading
  - Option 2: Invert V-coordinate in shader: `texCoord.y = 1.0 - texCoord.y;`
  - Option 3: In stb_image, use `stbi_set_flip_vertically_on_load(true);`

### Model Scale and Position Issues
- **Problem**: Model is huge, tiny, or in the wrong location
- **Solution**: TurboSquid models vary wildly in scale (some in cm, some in m)
  - Load model, measure bounds, scale to fit: `model.scale(1.0 / model.maxDimension());`
  - Translate to origin: `model.translate(-model.center());`
  - Adjust camera far plane if needed: `gluPerspective(45.0, aspect, 0.1, 1000.0);`

### Model Normals Point Inward
- **Problem**: Model looks dark or invisible in certain views
- **Solution**: Some exporters flip normals; re-export from Blender with correct settings
  - Or flip them in code: multiply normals by -1 in shader
  - Or check if tinyobjloader has a flip flag

### License Attribution Missing
- **Problem**: Forgot to credit model sources
- **Solution**: Create `CREDITS.txt` in your project root:
  ```
  Assets Attribution:
  - Pepsi Character: Downloaded from Mixamo (Adobe)
  - Can Model: Downloaded from TurboSquid (https://www.turbosquid.com/...)
  - Environment: Created with Blender
  
  All free/open-source or appropriately licensed for educational use.
  ```

---

## 10. Fixing Rendering Issues: Lighting, Materials, and Consistent Scale

This section diagnoses two symptoms that commonly hit this project on first run:

1. Every loaded model appears as a **black silhouette**.
2. Models render at wildly **different relative sizes** even after hand-tuning `glScalef`, and the scene never feels "homogeneous".

Both have a clean fix. They are also independent — fix lighting/materials first (you'll see colour), then fix scaling (you'll see correct proportions).

### 10.1 Why everything renders black

Three causes typically combine:

1. **Lighting is disabled, so only `glColor*` paints the surface.** With `glDisable(GL_LIGHTING)` (see [src/main.cpp:197](src/main.cpp#L197)), OpenGL ignores normals and material properties entirely — the only colour comes from the most recent `glColor*` call. If [src/Model.cpp:138](src/Model.cpp#L138) calls `glColor3f(diffuse.r, diffuse.g, diffuse.b)` and the material's diffuse is `(0,0,0)`, every face is black.
2. **TurboSquid / Mixamo `.mtl` files often ship with `Kd 0 0 0` or `Kd 1 1 1`** because the real colour is in a `map_Kd` texture (e.g. `diffuse.jpg`). If your loader doesn't bind that texture, you fall back to plain black/white.
3. **A mesh with no assigned material gets `Material{}`,** which leaves `diffuseColor = (0,0,0)` because the struct in [src/Model.h](src/Model.h) has no defaults — same result, black.

#### Minimum fix to see colours (no textures yet)

In `init()`, replace the lighting-disabled block with:

```cpp
glEnable(GL_LIGHTING);
glEnable(GL_LIGHT0);
glEnable(GL_NORMALIZE);            // re-normalises after glScalef
glEnable(GL_COLOR_MATERIAL);       // glColor* drives diffuse + ambient
glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

GLfloat lightPos[] = {  5.0f, 10.0f,  5.0f, 0.0f };  // directional (w=0)
GLfloat lightDif[] = {  0.9f,  0.9f,  0.9f, 1.0f };
GLfloat lightAmb[] = {  0.3f,  0.3f,  0.3f, 1.0f };
glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
glLightfv(GL_LIGHT0, GL_DIFFUSE,  lightDif);
glLightfv(GL_LIGHT0, GL_AMBIENT,  lightAmb);

GLfloat globalAmb[] = { 0.25f, 0.25f, 0.25f, 1.0f };
glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);
```

Give `Material` real defaults so unassigned meshes aren't black:

```cpp
struct Material {
    glm::vec3 ambientColor  = glm::vec3(0.2f);
    glm::vec3 diffuseColor  = glm::vec3(0.8f);   // light grey
    glm::vec3 specularColor = glm::vec3(0.1f);
    float     shininess     = 16.0f;
};
```

In `renderMesh`, clamp degenerate-black diffuse so texture-only materials still show *something*:

```cpp
glm::vec3 d = mesh.material.diffuseColor;
if (d.x + d.y + d.z < 0.05f) d = glm::vec3(0.75f);
glColor3f(d.x, d.y, d.z);
```

**Sanity check before going further**: temporarily hardcode `glColor3f(1.0f, 0.5f, 0.2f)`. If everything turns orange, geometry and lighting are fine and only material colour was the problem. If it stays black, the issue is normals (next paragraph).

#### Black-because-of-normals trap

If the `.obj` has no `vn` lines, the loader currently writes `vec3(0,1,0)` for every vertex (see [src/Model.cpp:94](src/Model.cpp#L94)). With lighting on, every face is treated as if it points straight up, so vertical walls receive zero diffuse contribution and stay black. Either:

- Compute per-face normals after loading: for each triangle, `n = normalize(cross(v1-v0, v2-v0))`, assign to all three vertices.
- Or set `tinyobj::ObjReaderConfig` to triangulate and let it generate normals when missing.

#### Once colour is working, add textures

Walk `material_t::diffuse_texname` in the loader, call `stbi_load`, upload with `glGenTextures + glTexImage2D`, store the GL id on `Material`. Before drawing each mesh: `glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, mesh.material.diffuseTex);`. tinyobjloader already gave you UVs in `texCoord`.

---

### 10.2 Why sizes are inconsistent — and the bounding-box fix

Every `.obj` is authored in its own unit system. A Mixamo character is usually in metres; a TurboSquid car might be centimetres or arbitrary "Maya units"; a Sketchfab export could be millimetres. The intrinsic scale of the geometry has nothing to do with your scene's units.

The current code in [src/main.cpp:230-261](src/main.cpp#L230-L261) compensates by guessing per-model scales (`0.0033`, `0.0067`, `0.0053`, `0.01`). This is fragile because:

- A different camera distance changes which scales "look right".
- Bounding centres are not at the origin, so models sit at different heights and feet sink through the ground.
- Adding a new model requires tuning from scratch.

**The fix: normalise every model to a unit bounding box at load time, then specify the desired real-world size (in metres) per scene object.** After normalisation, `scale = (1.7f, 1.7f, 1.7f)` means "1.7 metres tall", regardless of which file the mesh came from.

#### Step 1 — compute bounding box during `Model::load`

```cpp
// add to Model.h:
glm::vec3 bbMin, bbMax, bbCenter;
float     maxDim = 1.0f;

// at the end of Model::load, after meshes are built:
bbMin = glm::vec3( FLT_MAX);
bbMax = glm::vec3(-FLT_MAX);
for (const auto& mesh : meshes)
    for (const auto& v : mesh.vertices) {
        bbMin = glm::min(bbMin, v.position);
        bbMax = glm::max(bbMax, v.position);
    }
bbCenter = 0.5f * (bbMin + bbMax);
glm::vec3 size = bbMax - bbMin;
maxDim = std::max({ size.x, size.y, size.z });
std::cout << "BBox " << filepath << ": size=("
          << size.x << "," << size.y << "," << size.z
          << ") maxDim=" << maxDim << std::endl;
```

The print is deliberate — surprises in `maxDim` (e.g. `2400` vs `1.7`) explain almost every "model is invisible / fills the screen" mystery.

#### Step 2 — apply normalisation transform first in `Model::render`

```cpp
void Model::render() {
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    glRotatef(rotation.y, 0, 1, 0);
    glRotatef(rotation.x, 1, 0, 0);
    glRotatef(rotation.z, 0, 0, 1);
    glScalef(scale.x, scale.y, scale.z);

    // --- normalisation: fits model into unit cube, sits on ground (y=0) ---
    float k = 1.0f / maxDim;
    glScalef(k, k, k);
    glTranslatef(-bbCenter.x, -bbMin.y, -bbCenter.z);
    // ----------------------------------------------------------------------

    for (const auto& mesh : meshes) renderMesh(mesh);
    glPopMatrix();
}
```

Order matters: the normalisation must run *closest to the geometry*, i.e. last in C++ source but first in the matrix stack relative to the vertices.

#### Step 3 — rewrite scene placement in real-world units

```cpp
// scales below are the real-world size in metres, no more 0.0053 magic numbers
g_scene->addModel("city",        "assets/models/China_town_city_blocks.obj",
                  glm::vec3(0, 0, 0),       glm::vec3(40.0f),  glm::vec3(0));
g_scene->addModel("player_car",  "assets/models/old_car.obj",
                  glm::vec3(0, 0, 5),       glm::vec3(1.7f),   glm::vec3(0));
g_scene->addModel("golf_left",   "assets/models/Golf.obj",
                  glm::vec3(-3, 0, 8),      glm::vec3(1.5f),   glm::vec3(0, 45, 0));
g_scene->addModel("roadblock_1", "assets/models/RoadBlockade_02.obj",
                  glm::vec3(-2, 0, 12),     glm::vec3(0.8f),   glm::vec3(0));
```

Tweaks now have intuitive units: "make the car a little bigger" → bump `1.7` to `1.9`.

---

### 10.3 Perspective projection: how `gluPerspective` actually controls perceived size

`gluPerspective(fovy, aspect, near, far)` defines the viewing frustum. Each parameter has a specific effect:

- **`fovy` — vertical field of view, in degrees.** Smaller (≈ 30°) is "telephoto": flattens depth, distant objects look only slightly smaller than near ones. Larger (≈ 90°) is "wide-angle": exaggerates depth, near objects loom huge. **45–55° is the natural-looking default for a third-person game.**
- **`aspect` — `viewportWidth / viewportHeight`.** Recompute it on every `reshape`, otherwise the image stretches.
- **`near` / `far` — clip distances from the eye.** Not about size; they decide what is visible and how depth precision is distributed. **Critical rule: keep `far / near` as small as the scene allows.** `(0.1, 1000)` (current code in [src/main.cpp:43](src/main.cpp#L43)) wastes ~99 % of the depth buffer near the camera and causes z-fighting; `(1.0, 200)` gives ~10× better resolution.

#### Why the current scene doesn't look "homogeneous"

Perspective foreshortening shrinks distant objects by `1/z`. At `fovy = 45°`, a 1-metre object 5 metres away occupies the same screen size as a 2-metre object 10 metres away. The current camera in [src/main.cpp:13](src/main.cpp#L13) sits at `cameraDistance = 100.0f` while the scaled models are well under one unit across — every model is squashed into a few pixels and any difference between them is invisible. Apparent size is governed by the ratio `(distance / object_size)` and `fovy`; pick units consistently or the scene will never look right.

#### Practical setup that pairs with the normalisation in 10.2 (units = metres)

```cpp
// reshape():
gluPerspective(50.0, (double)w / h, 0.5, 300.0);

// display() — orbit a ~2-metre subject:
float dist  = 8.0f;
float pitch = 12.0f;
float yaw   = cameraRotY;
float ex = dist * cosf(RAD(pitch)) * sinf(RAD(yaw));
float ey = dist * sinf(RAD(pitch)) + 1.0f;   // eye 1 m above ground
float ez = dist * cosf(RAD(pitch)) * cosf(RAD(yaw));
gluLookAt(ex, ey, ez,   0, 1.0f, 0,   0, 1, 0);
```

#### Debug ladder when the scene still looks wrong

Work top to bottom — don't skip steps:

1. Log `bbMin`, `bbMax`, `maxDim` for every loaded model. Surprises here explain ~80 % of "invisible / huge" cases.
2. Render a 1-metre reference cube at the origin in a contrasting colour. If the character isn't roughly the same height, normalisation isn't running.
3. Reset to canonical params: `fovy = 45`, camera distance = `5 × sceneRadius`, look at scene centre. If it looks right here, the bug is in your gameplay camera.
4. Only then arrange the actual scene.

---

## 11. References and Useful Links

### Learning Resources
- **LearnOpenGL.com**: https://learnopengl.com/ — Best free OpenGL tutorial series
- **Khronos OpenGL Docs**: https://www.khronos.org/opengl/ — Official reference
- **GLUT Documentation**: https://www.opengl.org/resources/libraries/glut/spec3/spec3.html

### Windows Libraries (MinGW Binaries)
- **Transmission Zero freeglut Windows**: https://www.transmissionzero.co.uk/software/freeglut-devel/ — Prebuilt MinGW binaries
- **GLEW**: http://glew.sourceforge.net/ — Download MinGW binaries
- **GLM**: https://github.com/g-truc/glm — Header-only; download zip
- **tinyobjloader**: https://github.com/tinyobjloader/tinyobjloader — Single header
- **stb_image**: https://github.com/nothings/stb — Single header
- **GLFW (Windows binaries)**: https://www.glfw.org/download.html — Modern windowing alternative

### Model Sources
- **Mixamo**: https://www.mixamo.com/ — Free rigged and animated characters
- **TurboSquid Free**: https://www.turbosquid.com/?max_price=0 — Free 3D models
- **Sketchfab Free**: https://sketchfab.com/?q=&type=models&downloadable=true — Community models
- **Poly Haven**: https://polyhaven.com/models — High-quality CC0 models
- **Quaternius**: https://quaternius.com/ — Low-poly game assets

### Game Development
- **OpenGL Game Development Series**: https://learnopengl.com/In-Practice/2D-Game/Breakout/Breakout
- **Game Programming Patterns**: https://gameprogrammingpatterns.com/ — Design patterns for games
- **Real-Time Rendering**: https://www.realtimerendering.com/ — Advanced graphics book

---

## Summary

This project is achievable in a few weeks of part-time work. Focus on **Phase 2 and 3** first to verify that model loading works, then expand to gameplay. Use **tinyobjloader** and the **fixed-function pipeline** to minimize complexity. Download a ready-made character from **Mixamo** to avoid rigging work. Build incrementally: render → textures → camera → movement → game logic.

**For Windows / Code::Blocks**:
1. Download prebuilt MinGW libraries (freeglut, GLEW) from the links above
2. Organize headers in `include/` and libraries in `lib/`
3. Create a Code::Blocks project; configure compiler, linker, and search directories
4. Copy DLLs next to the .exe
5. Build and run

Good luck!
