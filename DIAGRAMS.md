# Visual Architecture Diagrams

## Class Structure Overview

```
┌────────────────────────────────────────────────────────────────┐
│                     Graphics Namespace                         │
└────────────────────────────────────────────────────────────────┘
                              │
                ┌─────────────┴─────────────┐
                │                           │
        ┌───────▼────────┐         ┌────────▼─────────┐
        │     Mesh       │         │   Renderable     │
        │  (Data Layer)  │         │(Presentation)    │
        └────────────────┘         └──────────────────┘
                │                           │
                │                           │
    ┌───────────┴───────────┐      ┌────────┴────────┐
    │                       │      │                 │
    ▼                       ▼      ▼                 ▼
┌────────┐            ┌────────┐ ┌────────┐    ┌────────┐
│ Vec3   │            │ Vec2   │ │Texture │    │Shader  │
│struct  │            │struct  │ │ class  │    │ class  │
└────────┘            └────────┘ └────────┘    └────────┘
```

---

## Mesh Class Detailed Structure

```
┌─────────────────────────────────────────────────────────────┐
│                         Mesh                                │
├─────────────────────────────────────────────────────────────┤
│ Member Data (Private):                                      │
│  • std::vector<Vec3> vertices_                              │
│  • std::vector<Index> indices_                              │
│  • std::vector<Vec3> colors_                                │
│  • std::vector<Vec2> texCoords_                             │
│  • std::vector<Vec3> normals_                               │
├─────────────────────────────────────────────────────────────┤
│ Public Interface:                                           │
│  Construction:                                              │
│   ├─ Mesh()                                                 │
│   └─ Mesh(vertexCount, indexCount)                         │
│                                                             │
│  Data Modification:                                         │
│   ├─ addVertex(...)                                         │
│   ├─ addIndex(...)                                          │
│   ├─ addTriangle(i0, i1, i2)                                │
│   └─ reserve(vCount, iCount)                                │
│                                                             │
│  Data Access:                                               │
│   ├─ getVertices() const / non-const                        │
│   ├─ getIndices() const / non-const                         │
│   ├─ getColors() const / non-const                          │
│   ├─ getTexCoords() const / non-const                       │
│   └─ getNormals() const / non-const                         │
│                                                             │
│  Utilities:                                                 │
│   ├─ isValid() const                                        │
│   ├─ validate() const                                       │
│   ├─ computeFlatNormals()                                   │
│   ├─ computeSmoothNormals()                                 │
│   ├─ getVertexCount() const                                 │
│   ├─ getIndexCount() const                                  │
│   └─ getTriangleCount() const                               │
├─────────────────────────────────────────────────────────────┤
│ Semantics:                                                  │
│  • Value type (copyable, moveable)                          │
│  • Rule of Zero (compiler-generated special members)        │
│  • No GPU dependencies                                      │
│  • Thread-safe for reading (const operations)               │
└─────────────────────────────────────────────────────────────┘
```

---

## Renderable Class Detailed Structure

```
┌──────────────────────────────────────────────────────────────┐
│                       Renderable                             │
├──────────────────────────────────────────────────────────────┤
│ Member Data (Private):                                       │
│  Shared Resources:                                           │
│   • shared_ptr<Mesh> mesh_                                   │
│   • shared_ptr<Shader> shader_                               │
│   • shared_ptr<Texture> texture_                             │
│   • shared_ptr<Material> material_                           │
│   • unordered_map<string, TexturePtr> additionalTextures_   │
│                                                              │
│  Unique Resources:                                           │
│   • unique_ptr<VertexBuffer> vertexBuffer_                   │
│   • unique_ptr<IndexBuffer> indexBuffer_                     │
│                                                              │
│  State Flags:                                                │
│   • bool gpuDataValid_                                       │
│   • bool enabled_                                            │
│   • bool castsShadows_                                       │
│   • bool receivesShadows_                                    │
├──────────────────────────────────────────────────────────────┤
│ Public Interface:                                            │
│  Construction:                                               │
│   ├─ Renderable()                                            │
│   ├─ Renderable(meshPtr)                                     │
│   └─ Renderable(meshPtr, shaderPtr, texturePtr)             │
│                                                              │
│  Resource Management:                                        │
│   ├─ setMesh(meshPtr)                                        │
│   ├─ setShader(shaderPtr)                                    │
│   ├─ setTexture(texturePtr)                                  │
│   ├─ setTexture(name, texturePtr)                            │
│   ├─ setMaterial(materialPtr)                                │
│   └─ getMesh/Shader/Texture() const                          │
│                                                              │
│  GPU Operations:                                             │
│   ├─ uploadToGPU()                                           │
│   ├─ invalidateGPUData()                                     │
│   ├─ releaseGPUResources()                                   │
│   └─ isUploadedToGPU() const                                 │
│                                                              │
│  Rendering:                                                  │
│   ├─ render() const                                          │
│   ├─ render(shader) const                                    │
│   ├─ isReadyToRender() const                                 │
│   ├─ bind() const                                            │
│   └─ unbind() const                                          │
│                                                              │
│  State Management:                                           │
│   ├─ setEnabled(bool)                                        │
│   ├─ setCastsShadows(bool)                                   │
│   ├─ setReceivesShadows(bool)                                │
│   └─ isEnabled/castsShadows/receivesShadows() const         │
├──────────────────────────────────────────────────────────────┤
│ Semantics:                                                   │
│  • Reference type (move-only)                                │
│  • Deleted copy constructor/assignment                       │
│  • RAII for GPU resources                                    │
│  • Not thread-safe (use from render thread)                  │
└──────────────────────────────────────────────────────────────┘
```

---

## Memory Ownership Model

```
Application Code
    │
    ├─ Creates shared_ptr<Mesh>
    │       │
    │       ├─→ Renderable 1 (references)
    │       ├─→ Renderable 2 (references)
    │       └─→ Renderable 3 (references)
    │
    ├─ Creates shared_ptr<Shader>
    │       │
    │       ├─→ Renderable 1 (references)
    │       ├─→ Renderable 2 (references)
    │       └─→ Renderable 4 (references)
    │
    └─ Creates shared_ptr<Texture>
            │
            ├─→ Renderable 1 (references)
            └─→ Renderable 3 (references)

Each Renderable owns:
    • unique_ptr<VertexBuffer>  (exclusive)
    • unique_ptr<IndexBuffer>   (exclusive)

Reference Count Examples:
    Mesh: refcount = 3 (3 Renderables reference it)
    Shader: refcount = 3 (3 Renderables reference it)
    Texture: refcount = 2 (2 Renderables reference it)
```

---

## Data Flow Diagram

```
┌──────────────┐
│ Create Mesh  │ ◄─── CPU Memory
└──────┬───────┘
       │
       ▼
┌──────────────────────┐
│ Fill with geometry:  │
│  • vertices          │
│  • indices           │
│  • colors            │
│  • texcoords         │
│  • normals           │
└──────┬───────────────┘
       │
       ▼
┌────────────────────┐
│ validate() ────────┼─── Check consistency
└──────┬─────────────┘
       │
       ▼
┌──────────────────────┐
│ Create Renderable    │
│  with shared_ptr     │
└──────┬───────────────┘
       │
       ▼
┌──────────────────────┐
│ Set shader/textures  │
└──────┬───────────────┘
       │
       ▼
┌──────────────────────┐
│ uploadToGPU() ───────┼─── CPU → GPU Transfer
└──────┬───────────────┘
       │                    ┌──────────────┐
       ├────────────────────┤ VertexBuffer │ ◄─── GPU Memory
       │                    └──────────────┘
       │                    ┌──────────────┐
       └────────────────────┤ IndexBuffer  │ ◄─── GPU Memory
                            └──────────────┘
       │
       ▼
┌──────────────────────┐
│ Render Loop:         │
│  if ready:           │
│    render()          │ ◄─── Draw calls to GPU
└──────────────────────┘
```

---

## Instancing Pattern Visualization

```
Single Mesh (CPU Memory)
┌────────────────────────┐
│  Sphere Geometry       │
│  • 1000 vertices       │
│  • 2000 triangles      │
│  • Memory: ~40KB       │
└───────────┬────────────┘
            │ shared_ptr (refcount = 100)
            │
    ┌───────┴────────┬────────┬────────┬─── ... ───┐
    ▼                ▼        ▼        ▼            ▼
┌─────────┐    ┌─────────┐ ┌─────────┐         ┌─────────┐
│Renderable│    │Renderable│ │Renderable│   ...  │Renderable│
│    #1    │    │    #2    │ │    #3    │         │   #100   │
├─────────┤    ├─────────┤ ├─────────┤         ├─────────┤
│ Texture: │    │ Texture: │ │ Texture: │         │ Texture: │
│  Wood    │    │  Metal   │ │  Stone   │         │  Glass   │
│ Position:│    │ Position:│ │ Position:│         │ Position:│
│ (0,0,0)  │    │ (5,0,0)  │ (10,0,0) │         │(495,0,0) │
└─────────┘    └─────────┘ └─────────┘         └─────────┘
    │              │            │                    │
    └──────────────┴────────────┴────────────────────┘
                   │
                   ▼
         Each has unique GPU buffers
         (but could use GPU instancing)

Memory Analysis:
    CPU Mesh: 40 KB (shared)
    Per Renderable overhead: ~100 bytes (pointers + state)
    Total CPU overhead: 40KB + (100 × 100 bytes) = ~50 KB

Without sharing:
    100 separate meshes: 40KB × 100 = 4000 KB (4 MB)

Savings: ~98.75% reduction in CPU memory!
```

---

## Mesh Modification and Re-upload Flow

```
┌─────────────────┐
│ Initial State   │
│ CPU: Mesh       │
│ GPU: Buffers    │
│ gpuDataValid_=  │
│      true       │
└────────┬────────┘
         │
         ▼
┌─────────────────────┐
│ Modify Mesh Data    │
│ mesh->getVertices() │
│      [i].y += 1.0   │
└────────┬────────────┘
         │
         ▼
┌─────────────────────┐
│ invalidateGPUData() │
│ gpuDataValid_ =     │
│      false          │
└────────┬────────────┘
         │
         ▼
┌─────────────────────┐
│ uploadToGPU()       │
│ • Upload vertices   │
│ • Upload indices    │
│ • Set valid = true  │
└────────┬────────────┘
         │
         ▼
┌─────────────────────┐
│ render()            │
│ • Use updated GPU   │
│   buffers           │
└─────────────────────┘
```

---

## Rendering Pipeline Integration

```
┌─────────────────────────────────────────────────────────────┐
│                    Render Loop (Application)                │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  For each Renderable in scene:      │
        └─────────────┬───────────────────────┘
                      │
                      ├─► isEnabled()? ────────► Skip if false
                      │
                      ├─► isReadyToRender()? ──► Skip if false
                      │
                      ▼
        ┌─────────────────────────────┐
        │  Set per-object uniforms:   │
        │  • Model matrix             │
        │  • Material properties      │
        └─────────────┬───────────────┘
                      │
                      ▼
        ┌─────────────────────────────┐
        │  renderable.render()        │
        └─────────────┬───────────────┘
                      │
        ┌─────────────┴─────────────┐
        │                           │
        ▼                           ▼
┌───────────────┐         ┌──────────────────┐
│ bind():       │         │ GPU Draw Call:   │
│ • Bind shader │         │ • glDrawElements │
│ • Bind texture│    ┌────│ • vkCmdDraw      │
│ • Bind buffers│    │    │ • DrawIndexed    │
└───────┬───────┘    │    └──────────────────┘
        │            │
        ▼            │
┌───────────────┐    │
│ GPU Renders   │◄───┘
│ Triangles     │
└───────┬───────┘
        │
        ▼
┌───────────────┐
│ unbind():     │
│ • Cleanup     │
│ • Reset state │
└───────────────┘
```

---

## Class Hierarchy and Extensions

```
┌────────────────────────────────────────────────────────┐
│                    Base Classes                        │
└────────────────────────────────────────────────────────┘
         │                              │
         ▼                              ▼
    ┌────────┐                    ┌──────────┐
    │  Mesh  │                    │Renderable│
    └───┬────┘                    └────┬─────┘
        │                              │
        │ Could extend with:           │ Could extend with:
        │                              │
        ├─► SkinnedMesh               ├─► AnimatedRenderable
        │   • Bone weights            │   • Animation state
        │   • Bone indices            │   • Keyframe data
        │                              │
        ├─► ProceduralMesh            ├─► LODRenderable
        │   • Generation params       │   • Multiple LOD levels
        │   • Regenerate()            │   • Distance selection
        │                              │
        └─► MorphTargetMesh           └─► InstancedRenderable
            • Multiple shapes             • Instance buffer
            • Blend weights               • Draw instanced


┌────────────────────────────────────────────────────────┐
│                 Supporting Classes                      │
└────────────────────────────────────────────────────────┘

    ┌──────────┐         ┌──────────┐         ┌──────────┐
    │  Shader  │         │ Texture  │         │ Material │
    └──────────┘         └──────────┘         └──────────┘
         │                    │                     │
         │                    │                     │
    Implements:           Implements:          Contains:
    • compile()           • load()             • Textures
    • bind()              • bind()             • Parameters
    • setUniform()        • getSize()          • Shader ref


    ┌────────────┐       ┌────────────┐
    │VertexBuffer│       │IndexBuffer │
    └────────────┘       └────────────┘
         │                    │
         │                    │
    Platform-specific:    Platform-specific:
    • OpenGL: VBO         • OpenGL: IBO
    • Vulkan: VkBuffer    • Vulkan: VkBuffer
    • D3D12: ID3DResource • D3D12: ID3DResource
```

---

## Thread Safety Model

```
┌─────────────────────────────────────────────────────────┐
│                      Thread Safety                       │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  Mesh (Value Type):                                      │
│  ┌────────────────────────────────────────────┐         │
│  │  ✓ Safe: Multiple threads reading          │         │
│  │  ✗ Unsafe: Concurrent reading + writing    │         │
│  │  ✗ Unsafe: Multiple threads writing        │         │
│  │                                             │         │
│  │  Solution: Const access is thread-safe     │         │
│  │            Mutable access requires sync    │         │
│  └────────────────────────────────────────────┘         │
│                                                          │
│  Renderable (Reference Type):                           │
│  ┌────────────────────────────────────────────┐         │
│  │  ✗ Unsafe: Not thread-safe by design       │         │
│  │  ✗ Unsafe: GPU context affinity            │         │
│  │                                             │         │
│  │  Solution: Use from render thread only     │         │
│  │            Or implement explicit locking   │         │
│  └────────────────────────────────────────────┘         │
│                                                          │
│  Typical Multi-threaded Pattern:                        │
│  ┌────────────────────────────────────────────┐         │
│  │  Worker Threads:                            │         │
│  │    • Create/modify Meshes                   │         │
│  │    • Compute normals                        │         │
│  │    • Load from disk                         │         │
│  │                                             │         │
│  │  Render Thread:                             │         │
│  │    • Create Renderables                     │         │
│  │    • Upload to GPU                          │         │
│  │    • Issue draw calls                       │         │
│  │                                             │         │
│  │  Communication:                             │         │
│  │    • Queue of meshes to upload              │         │
│  │    • Mutex-protected                        │         │
│  └────────────────────────────────────────────┘         │
└─────────────────────────────────────────────────────────┘
```

---

## Lifecycle Diagram

```
Mesh Lifecycle:
──────────────
┌──────┐     ┌──────┐     ┌──────┐     ┌──────┐     ┌──────┐
│Create│────►│Build │────►│Valid?│────►│ Use  │────►│Destroy│
└──────┘     └──────┘     └───┬──┘     └──────┘     └──────┘
                              │                          ▲
                              │ No                       │
                              └──────────────────────────┘
                              Report error / Fix


Renderable Lifecycle:
─────────────────────
┌──────┐  ┌────┐  ┌──────┐  ┌────┐  ┌──────┐  ┌──────┐
│Create│─►│Set │─►│Upload│─►│Use │─►│Modify│─►│Destroy│
│      │  │Res.│  │ GPU  │  │    │  │Mesh  │  │       │
└──────┘  └────┘  └──────┘  └─┬──┘  └───┬──┘  └───▲───┘
                               │         │         │
                               │         ▼         │
                               │    ┌────────┐     │
                               │    │Re-up │────┘
                               │    │ load    │
                               │    └────────┘
                               │
                               └─► Render loop ─┐
                                      ▲          │
                                      └──────────┘

Resource Sharing Example:
─────────────────────────
Time ─────────────────────────────────────►

Mesh:     [────────────────────────────]
          Create         ...        Destroy
                                    (when last ref dies)

Render1:    [──────────────────]
            Create    ...    Destroy

Render2:      [────────────────────]
              Create    ...    Destroy

Render3:          [──────────]
                  Create ... Destroy

Mesh refcount:
   0 ─► 1 ─► 2 ─► 3 ─► 2 ─► 1 ─► 0
       │    │    │    │    │    └─► Mesh deleted
       │    │    │    │    └──────► Render1 destroyed
       │    │    │    └───────────► Render3 destroyed
       │    │    └────────────────► Render3 created
       │    └─────────────────────► Render2 created
       └──────────────────────────► Render1 created
```

---

## Performance Optimization Patterns

```
┌──────────────────────────────────────────────────────┐
│           Optimization Strategy Matrix                │
├──────────────────────────────────────────────────────┤
│                                                       │
│  Static Geometry (never changes):                    │
│  ─────────────────────────────────                   │
│    Mesh ─────► Upload once ─────► Render many times │
│           uploadToGPU()                              │
│                                                       │
│  Dynamic Geometry (changes frequently):              │
│  ──────────────────────────────────────              │
│    Mesh ──┬─► Modify CPU data                        │
│           │                                           │
│           ├─► Batch modifications                    │
│           │                                           │
│           └─► uploadToGPU() once per frame           │
│                                                       │
│  Instanced Geometry (same mesh, many objects):       │
│  ─────────────────────────────────────────────       │
│    1 Mesh ──┬─► Renderable 1 (wood texture)          │
│             ├─► Renderable 2 (metal texture)         │
│             ├─► Renderable 3 (stone texture)         │
│             └─► ... (share geometry data)            │
│                                                       │
│    Memory: 40KB vs 4MB (100x instances)              │
│                                                       │
│  GPU Instancing (advanced):                          │
│  ───────────────────────────                         │
│    1 Mesh ──► 1 Renderable                           │
│               └─► Instance buffer (positions, etc.)  │
│                   └─► Draw instanced (1 draw call)   │
│                                                       │
└──────────────────────────────────────────────────────┘
```

---

These diagrams provide visual representations of the architecture's key concepts, data flows, and relationships. Use them as reference when implementing or extending the system.
