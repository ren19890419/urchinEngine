# UrchinEngine
## Features
Game engine library in C++17: 3D, Physics, Sound, Network, IA, Map Editor

Video: <https://www.youtube.com/watch?v=lSbCx7u6RtE>

- 3D Engine:
  - Skeleton animate 3d models
  - Texture manager: TGA & PNG, anisotropy, mip-maps, normal mapping
  - Dynamic shadow (cached parallel split shadow maps, variance shadow map, blur)
  - Dynamic lighting with deferred shading: omni-directional & directional
  - Terrain multi-layered and grass
  - Ambient occlusion (SSAO+)
  - Anti-aliasing (FXAA)
  - Space partitioning (octree)
  - UI 2D: button, window, text (TTF), text input, bitmap, slider
  - Skybox
  - Fog

- Physics Engine:
  - Supported shapes: sphere, box, cylinder, capsule, cone, convex hull, compound shape, terrain
  - Supported body: rigid body
  - Algorithms:
    - Broad phase: AABBox Tree
    - Narrow phase: sphere-sphere, sphere-box, GJK/EPA, concave-convex
    - Constraint solver (iterative): external force, friction, restitution, inertia
  - Continuous collision detection (CCD)
  - Island
  - Ray cast
  - Character controller

- Sound Engine:
  - Ambience sound and 3D sound
  - Streaming
  - Sound trigger in function of character position

- IA Engine:
  - Automatic navigation mesh generator
  - Pathfinding A* with funnel algorithm
  - Character controller
  
- Network Engine:
  - HTTP request sender
  
- Map Editor:
  - Handle:
    - 3d objects with physics
    - Lights
    - Landscape: terrain, water, sky
    - Sound
    - AI agent
  - Debug visualization: hit box, light range, sound trigger, navigation mesh

## Build
- Pre-requisites: 
  - Linux: `sudo apt install qt5-default qtbase5-dev libglew-dev libopenal-dev libsndfile1-dev libfreetype6-dev libcppunit-dev libcurl4-gnutls-dev`
  - Windows: [here](./WIN_SETUP.md)
- Build:
  - Debug:
    ```
    git clone https://github.com/petitg1987/urchinEngine.git && cd ./urchinEngine
    cmake -DCMAKE_BUILD_TYPE=Debug -S ./ -B .build/debug/
    cmake --build .build/debug/ -- -j 12
    ```
  - Release:
    ```
    git clone https://github.com/petitg1987/urchinEngine.git && cd ./urchinEngine
    cmake -DCMAKE_BUILD_TYPE=Release -S ./ -B .build/release/
    cmake --build .build/release/ -- -j 12
    ```
- Execute tests:
    ```
    cd urchinEngine/test/
    ./testRunner
    ```

## Launch map editor
```
cd urchinEngine/mapEditor/
./urchinMapEditor
```

## CLion configuration
- Add CMake profiles (File > Settings -> Build,Execution,Deployment > CMake)
  - Profile **Debug**: 
    - Name: `Debug`
    - Build type: `Debug`
    - Generation path: `.build/debug`
  - Profile **Release**:
    - Name: `Release`
    - Build type: `Release`
    - Generation path: `.build/release`
- Add CMake applications
  - Application **testRunner**:
    - Target/executable: `testRunner`
  - Application **urchinMapEditor**:
    - Target/executable: `urchinMapEditor`
