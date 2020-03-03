#include "Im3D.h"
#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include <vector>

#include <GL/gl3w.h>

namespace
{
    struct DrawVert
    {
        Vec3 pos;
        Vec3 col;
    };

    enum class DrawType : uint8_t
    {
        Points,
        Lines,
        LineList,
        Triangles
    };

    struct DrawCmd
    {
        DrawType type;
        bool isDeferredDraw; // If true, uses the commands vertex + elements arrays to draw from, assuming they've already been uploaded.
        unsigned int offset;
        unsigned int count;

        GLuint vertexArray;
        GLuint elementsArray;
    };

    constexpr Vec3 DefaultColor{ 1.f, 1.f, 1.f };

    GLuint vertShaderHandle;
    GLuint fragShaderHandle;
    GLuint shaderHandle{ 0 };
    GLuint attribLocationVtxPos;
    GLuint attribLocationVtxCol;
    GLuint uniformLocationCamFromWorld;
    GLuint uniformLocationClipFromCamera;


    static bool CheckShader(GLuint handle, const char* desc)
    {
        GLint status = 0, log_length = 0;
        glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
        glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
        if ((GLboolean)status == GL_FALSE)
            fprintf(stderr, "ERROR: Im3D: failed to compile %s!\n", desc);
        if (log_length > 1)
        {
            ImVector<char> buf;
            buf.resize((int)(log_length + 1));
            glGetShaderInfoLog(handle, log_length, NULL, (GLchar*)buf.begin());
            fprintf(stderr, "%s\n", buf.begin());
        }
        return (GLboolean)status == GL_TRUE;
    }

    static bool CheckProgram(GLuint handle, const char* desc)
    {
        GLint status = 0, log_length = 0;
        glGetProgramiv(handle, GL_LINK_STATUS, &status);
        glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length);
        if ((GLboolean)status == GL_FALSE)
            fprintf(stderr, "ERROR: Im3D: failed to link %s!\n", desc);
        if (log_length > 1)
        {
            ImVector<char> buf;
            buf.resize((int)(log_length + 1));
            glGetProgramInfoLog(handle, log_length, NULL, (GLchar*)buf.begin());
            fprintf(stderr, "%s\n", buf.begin());
        }
        return (GLboolean)status == GL_TRUE;
    }

    void InitializeShaders()
    {
        constexpr char* vertShaderSource = R"%%(
            #version 130
            uniform mat4 cameraFromWorld;
            uniform mat4 clipFromCamera;
            in vec3 Position;
            in vec3 Color;
            out vec4 FragColor;
            void main()
            {
                FragColor = vec4(Color, 1.0);
                gl_Position = clipFromCamera*cameraFromWorld*vec4(Position.xyz, 1);
            }
)%%";

        constexpr char* fragShaderSource = R"%%(
        #version 130
        in vec4 FragColor;
        out vec4 OutColor;
        void main()
        {
            OutColor = FragColor;
        }
)%%";

        vertShaderHandle = glCreateShader(GL_VERTEX_SHADER);
        fragShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);

        glShaderSource(vertShaderHandle, 1, &vertShaderSource, nullptr);
        glShaderSource(fragShaderHandle, 1, &fragShaderSource, nullptr);
        glCompileShader(vertShaderHandle);
        CheckShader(vertShaderHandle, "vertex shader");
        glCompileShader(fragShaderHandle);
        CheckShader(fragShaderHandle, "frament shader");

        shaderHandle = glCreateProgram();
        glAttachShader(shaderHandle, vertShaderHandle);
        glAttachShader(shaderHandle, fragShaderHandle);
        glLinkProgram(shaderHandle);
        CheckProgram(shaderHandle, "shader program");


        attribLocationVtxPos = glGetAttribLocation(shaderHandle, "Position");
        attribLocationVtxCol = glGetAttribLocation(shaderHandle, "Color");
        uniformLocationCamFromWorld = glGetUniformLocation(shaderHandle, "cameraFromWorld");
        uniformLocationClipFromCamera = glGetUniformLocation(shaderHandle, "clipFromCamera");
    }


    struct Quaternion
    {
        float x, y, z, w;

        Quaternion() = default;
        Quaternion(float a, float b, float c, float s) : x{ a }, y{ b }, z{ c }, w{ s } {}
        Quaternion(const Vec3& v, float s = 0) : x{ v.x }, y{ v.y }, z{ v.z }, w{ s }{}
        Quaternion(const Quaternion&) = default;
        Quaternion(Quaternion&&) = default;

        Quaternion& operator=(const Quaternion&) = default;
        Quaternion& operator=(Quaternion&&) = default;

        Vec3 GetVectorPart() const { return Vec3{ x, y, z }; }

        static Quaternion FromAxisAngle(const Vec3& axis, float angle)
        {
            float s = sinf(angle * 0.5f);
            return Quaternion(axis.x * s, axis.y * s, axis.z * s, cosf(angle * 0.5f));
        }

        void Normalize()
        {
            float denom = 1.f / sqrtf(x * x + y * y + z * z + w * w);
            x *= denom;
            y *= denom;
            z *= denom;
            w *= denom;
        }
    };

    Quaternion operator*(const Quaternion& q1, const Quaternion& q2)
    {
        return Quaternion(
            q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
            q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x,
            q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w,
            q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z
        );
    }

    Vec3 Cross(const Vec3& a, const Vec3& b)
    {
        return Vec3{
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    float Dot(const Vec3& a, const Vec3& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    Vec3 operator*(const Vec3& v, float s)
    {
        return Vec3{ v.x * s, v.y * s, v.z * s };

    }
    Vec3 operator*(float s, const Vec3& v)
    {
        return Vec3{ v.x * s, v.y * s, v.z * s };
    }

    Vec3 operator+(const Vec3& a, const Vec3& b)
    {
        return Vec3{ a.x + b.x, a.y + b.y, a.z + b.z };
    }
    
    Vec3 operator-(const Vec3& a, const Vec3& b)
    {
        return Vec3{ a.x - b.x, a.y - b.y, a.z - b.z };
    }

    float Length(const Vec3& a)
    {
        return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
    }

    Vec3 Normalized(const Vec3& a)
    {
        return (1.f / Length(a)) * a;
    }

    Vec3 Rotate(const Vec3& v, const Quaternion& q)
    {
        const Vec3 b = q.GetVectorPart();
        float b2 = b.x * b.x + b.y * b.y + b.z * b.z;
        return v * (q.w * q.w - b2) + b * (Dot(v, b) * 2.f) + Cross(b, v) * (2.f * q.w);
    }

    // Fills a matrix in row-major order.
    void FillTransformMatrix(const Quaternion& q, const Vec3& t, float m[16])
    {
        float x2 = q.x * q.x;
        float y2 = q.y * q.y;
        float z2 = q.z * q.z;
        float xy = q.x * q.y;
        float xz = q.x * q.z;
        float yz = q.y * q.z;
        float wx = q.w * q.x;
        float wy = q.w * q.y;
        float wz = q.w * q.z;

        // TODO: Pretty sure this rotation needs to be transposed.
        m[0] = 1.f - 2.f * (y2 + z2); m[1] = 2.f * (xy - wz);         m[2] = 2.f * (xz + wy);        m[3] = 0;
        m[4] = 2.f * (xy + wz);       m[5] = 1.f - 2.f * (x2 + z2);   m[6] = 2.f * (yz - wx);        m[7] = 0;
        m[8] = 2.f * (xz - wy);       m[9] = 2.f * (yz + wx);         m[10] = 1.f - 2.f * (x2 + y2); m[11] = 0;
        m[12] = t.x; m[13] = t.y; m[14] = t.z; m[15] = 1;
    }

    void FillTransformMatrix(const Vec3& cameraPos, const Vec3& up, const Vec3& target, float m[16])
    {
        Vec3 zaxis = Normalized(cameraPos - target);
        Vec3 xaxis = Normalized(Cross(up, zaxis));
        Vec3 yaxis = Normalized(Cross(zaxis, xaxis));

        m[0] = xaxis.x; m[1] = yaxis.x; m[2] = zaxis.x; m[3] = 0;
        m[4] = xaxis.y; m[5] = yaxis.y; m[6] = zaxis.y; m[7] = 0;
        m[8] = xaxis.z; m[9] = yaxis.z; m[10] = zaxis.z; m[11] = 0;
        m[12] = Dot(xaxis, cameraPos); m[13] = Dot(yaxis, cameraPos); m[14] = Dot(zaxis, cameraPos); m[15] = 1;
    }

    void FillProjectionMatrix(float n, float f, float r, float t, float m[16])
    {
        m[0] = n / r; m[1] = 0; m[2] = 0; m[3] = 0;
        m[4] = 0; m[5] = n / t; m[6] = 0; m[7] = 0;
        m[8] = 0; m[9] = 0; m[10] = -(n + f) / (n - f); m[11] = 1;
        m[12] = 0; m[13] = 0; m[14] = 2 * n * f / (n - f); m[15] = 0;
    }

    Vec3 GetArcballVector(float x, float y)
    {
        Vec3 p{ x, y, 0 };
        float OP_squared = p.x * p.x + p.y * p.y;
        if (OP_squared < 1)
        {
            p.z = sqrtf(1 - OP_squared);
        }
        else
        {
            float norm = sqrtf(OP_squared);
            p.x /= norm;
            p.y /= norm;
        }
        return p;
    }
}


struct View3d::Impl
{
    ImVec2 framebufferSize;
    GLuint colorTexture;
    GLuint depthbuffer;
    GLuint framebuffer;

    ImVec4 backgroundColor{ 0,0,0,0 };

    std::vector<DrawVert> vertexBuffer;
    std::vector<unsigned int> indexBuffer;
    std::vector<DrawCmd> drawCommands;

    GLuint vertexArray;
    GLuint elementsArray;

    Vec3 cameraTarget{ 0.f,0.f,0.f };
    Vec3 cameraPosition{ 0.f,0.f,-10.f };
    Vec3 cameraUp{ 0,1,0 };

    float nearPlane = 0.1;
    float farPlane = 100;
    float horizontalFovDegrees = 30.f;
    float cameraRotateSpeed = 2.f;
    float cameraZoomSpeed = 5.f;

    bool Initialize(const ImVec2& fbSize)
    {
        InitializeShaders();//TODO: Make this only happen on creation of the first 3d view.

        framebufferSize = fbSize;

        //Backup framebuffer state.
        GLuint prevTexture;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&prevTexture);
        GLuint prevRenderbuffer;
        glGetIntegerv(GL_RENDERBUFFER_BINDING, (GLint*)&prevRenderbuffer);
        GLuint prevFramebuffer;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&prevFramebuffer);

        // Generate framebuffers and textures necessary.
        glGenTextures(1, &colorTexture);
        glBindTexture(GL_TEXTURE_2D, colorTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, framebufferSize.x, framebufferSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glGenRenderbuffers(1, &depthbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, depthbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, framebufferSize.x, framebufferSize.y);

        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbuffer);
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            fprintf(stderr, "Failed to create framebuffer!");
            return false;
        }

        //Restore framebuffer state
        glBindFramebuffer(GL_FRAMEBUFFER, prevFramebuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, prevRenderbuffer);
        glBindTexture(GL_TEXTURE_2D, prevTexture);

        glGenBuffers(1, &vertexArray);
        glGenBuffers(1, &elementsArray);

        return true;
    }
};


View3d::View3d(const ImVec2& framebufferSize) : impl(std::make_unique<View3d::Impl>())
{
    impl->Initialize(framebufferSize);
}

View3d::~View3d() = default;

void View3d::SetBackgroundColor(float r, float g, float b, float a)
{
    impl->backgroundColor = ImVec4(r, g, b, a);
}


void View3d::DrawPoints(const Vec3 * points, int numPoints)
{
    size_t startingIndex = impl->vertexBuffer.size();
    impl->vertexBuffer.resize(startingIndex + numPoints);

    DrawCmd cmd;
    cmd.type = DrawType::Points;
    cmd.isDeferredDraw = false;
    cmd.count = numPoints;
    cmd.offset = startingIndex;

    for (int i = 0; i < numPoints; ++i)
    {
        impl->vertexBuffer[startingIndex + i].pos = points[i];
        impl->vertexBuffer[startingIndex + i].col = DefaultColor;
    }

    impl->drawCommands.emplace_back(std::move(cmd));
}

void View3d::DrawLine(const Vec3 start, const Vec3 end)
{
    size_t startingIndex = impl->vertexBuffer.size();
    impl->vertexBuffer.resize(startingIndex + 2);
    impl->vertexBuffer[startingIndex].pos = start;
    impl->vertexBuffer[startingIndex].col = DefaultColor;
    impl->vertexBuffer[startingIndex + 1].pos = end;
    impl->vertexBuffer[startingIndex + 1].col = DefaultColor;

    DrawCmd cmd;
    cmd.type = DrawType::Lines;
    cmd.isDeferredDraw = false;
    cmd.count = 2;
    cmd.offset = startingIndex;
    impl->drawCommands.emplace_back(std::move(cmd));
}

void View3d::DrawViewBall()
{
    Vec3 center = Vec3{ 0,0,0 } - impl->cameraTarget;
    // Compute radius based on camera parameters - want a fixed size on screen.
    // TODO: This needs to consider fov, etc.

    float radius = 0.1f * 10;

    // Want to draw a circle of lines around each axis, centered at the center.
    int numSegmentsPerCircle = 32;
    float angleStep = 2.0 * IM_PI / numSegmentsPerCircle;
    int numPointsPerCircle = numSegmentsPerCircle + 1;

    auto& verts = impl->vertexBuffer;
    size_t startingIndex = verts.size();
    verts.resize(startingIndex + 3 * (numPointsPerCircle));

    for (int pointIndex = 0; pointIndex < numPointsPerCircle; ++pointIndex)
    {
        float theta = angleStep * pointIndex;
        auto& xPoint = verts[startingIndex + pointIndex];
        xPoint.pos = center + Vec3{ 0,radius * sinf(theta),radius * cosf(theta) };
        xPoint.col = Vec3{ 1,0,0 };

        auto& yPoint = verts[startingIndex + pointIndex + numPointsPerCircle];
        yPoint.pos = center + Vec3{ radius * cosf(theta), 0, radius * sinf(theta) };
        yPoint.col = Vec3{ 0, 1,0 };

        auto& zPoint = verts[startingIndex + pointIndex + 2 * numPointsPerCircle];
        zPoint.pos = center + Vec3{ radius * cosf(theta), radius * sinf(theta), 0 };
        zPoint.col = Vec3{ 0, 0, 1 };

    }

    // This code is silly - should loop, or just use a line with indices...
    {
        DrawCmd cmd;
        cmd.type = DrawType::LineList;
        cmd.isDeferredDraw = false;
        cmd.count = numPointsPerCircle;
        cmd.offset = startingIndex;
        impl->drawCommands.emplace_back(std::move(cmd));
    }
    {
        DrawCmd cmd;
        cmd.type = DrawType::LineList;
        cmd.isDeferredDraw = false;
        cmd.count = numPointsPerCircle;
        cmd.offset = startingIndex + numPointsPerCircle;
        impl->drawCommands.emplace_back(std::move(cmd));
    }
    {
        DrawCmd cmd;
        cmd.type = DrawType::LineList;
        cmd.isDeferredDraw = false;
        cmd.count = numPointsPerCircle;
        cmd.offset = startingIndex + 2 * numPointsPerCircle;
        impl->drawCommands.emplace_back(std::move(cmd));
    }
}

void View3d::Render()
{

    glBindFramebuffer(GL_FRAMEBUFFER, impl->framebuffer);

    glClearColor(impl->backgroundColor.x, impl->backgroundColor.y, impl->backgroundColor.z, impl->backgroundColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Setup Opengl state;
    //TODO: Backup previous state.
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    glUseProgram(shaderHandle);
    glEnableVertexAttribArray(attribLocationVtxPos);
    glEnableVertexAttribArray(attribLocationVtxCol);

    glVertexAttribPointer(attribLocationVtxPos, 3, GL_FLOAT, GL_FALSE, sizeof(DrawVert), (GLvoid*)IM_OFFSETOF(DrawVert, pos));
    glVertexAttribPointer(attribLocationVtxCol, 3, GL_FLOAT, GL_FALSE, sizeof(DrawVert), (GLvoid*)IM_OFFSETOF(DrawVert, col));

    glPointSize(5);
    glViewport(0, 0, impl->framebufferSize.x, impl->framebufferSize.y);

    // Camera setup

    float cameraFromWorld[16];

    FillTransformMatrix(impl->cameraPosition, impl->cameraUp, impl->cameraTarget, cameraFromWorld);
    glUniformMatrix4fv(uniformLocationCamFromWorld, 1, GL_FALSE, cameraFromWorld);

    float clipFromCamera[16];

    float right = tanf(0.5f * 3.141592f / 180.f * impl->horizontalFovDegrees) * impl->nearPlane;
    float heightByWidth = impl->framebufferSize.y / impl->framebufferSize.x;


    FillProjectionMatrix(impl->nearPlane, impl->farPlane, right, right * heightByWidth, clipFromCamera);
    glUniformMatrix4fv(uniformLocationClipFromCamera, 1, GL_FALSE, clipFromCamera);

    //Copy over the buffer data.
    glBindBuffer(GL_ARRAY_BUFFER, impl->vertexArray);
    glBufferData(GL_ARRAY_BUFFER, impl->vertexBuffer.size() * sizeof(DrawVert), impl->vertexBuffer.data(), GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, impl->elementsArray);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, impl->indexBuffer.size() * sizeof(unsigned int), impl->indexBuffer.data(), GL_STREAM_DRAW);

    for (auto& cmd : impl->drawCommands)
    {
        //PERF: Avoid binding when unnecessary
        if (cmd.isDeferredDraw)
        {
            glBindBuffer(GL_ARRAY_BUFFER, cmd.vertexArray);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cmd.elementsArray);
        }
        else
        {
            glBindBuffer(GL_ARRAY_BUFFER, impl->vertexArray);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, impl->elementsArray);
        }

        GLenum drawMode;
        bool hasIndices = true;
        switch (cmd.type)
        {
        case DrawType::Points:
            drawMode = GL_POINTS;
            hasIndices = false;
            break;
        case DrawType::Lines:
            drawMode = GL_LINES;
            hasIndices = false;
            break;
        case DrawType::LineList:
            drawMode = GL_LINE_STRIP;
            hasIndices = false;
            break;
        case DrawType::Triangles:
            drawMode = GL_TRIANGLES;
            break;
        default:
            drawMode = GL_POINTS;
            break;
        }


        if (hasIndices)
        {
            glDrawElements(drawMode, cmd.count, GL_UNSIGNED_INT, (GLvoid*)cmd.offset);
        }
        else
        {
            glDrawArrays(drawMode, cmd.offset, cmd.count);
        }
    }

    impl->drawCommands.clear();
    impl->vertexBuffer.clear();
    impl->indexBuffer.clear();



    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void View3d::Image(const ImVec2 & size)
{

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    // Default to using texture ID as ID. User can still push string/integer prefixes.
    ImGui::PushID((void*)(intptr_t)impl->colorTexture);
    const ImGuiID id = window->GetID("#image");
    ImGui::PopID();

    const ImVec2 padding = style.FramePadding;
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size + padding * 2);
    const ImRect image_bb(window->DC.CursorPos + padding, window->DC.CursorPos + padding + size);
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id))
        return;

    bool hovered, leftHeld, rightHeld;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &leftHeld, ImGuiButtonFlags_MouseButtonLeft);

    // Render
    const ImVec2 uv_min = ImVec2(0, 0);
    const ImVec2 uv_max = ImVec2(1, 1);
    window->DrawList->AddImage((ImTextureID)impl->colorTexture, image_bb.Min, image_bb.Max, uv_min, uv_max, IM_COL32_WHITE);

    // Handle camera controls.

    bool leftClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    bool rightClicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
    
    static ImVec2 lastClickedPos;
    static Vec3 lastClickedUp;
    static Vec3 lastClickedCamPos;
    static Vec3 lastClickedTarget;
    if (leftClicked || rightClicked)
    {
        lastClickedPos = ImGui::GetMousePos();
        lastClickedUp = impl->cameraUp;
        lastClickedCamPos = impl->cameraPosition;
        lastClickedTarget = impl->cameraTarget;
        
    }

    if (leftHeld)
    {
        // Update camera information here.
        ImVec2 currentPos = ImGui::GetMousePos();
        //Convert mouse position to [-1,1] in the framebuffer coordinates.
        ImVec2 size = image_bb.GetSize();
        //Clicked position from [-1,1]
        ImVec2 lastClickedCam(2 * (lastClickedPos.x - image_bb.Min.x) / size.x - 1, 2 * (lastClickedPos.y - image_bb.Min.y) / size.y - 1);
        ImVec2 currentPosCam(2 * (currentPos.x - image_bb.Min.x) / size.x - 1, 2 * (currentPos.y - image_bb.Min.y) / size.y - 1);
        Vec3 movementDir{ currentPosCam.x - lastClickedCam.x, currentPosCam.y - lastClickedCam.y, 0.f };
        float angle = Length(movementDir);

        if (!isnan(angle) && angle != 0)
        {
            angle *= impl->cameraRotateSpeed;
            Vec3 eye = lastClickedCamPos - lastClickedTarget;

            Vec3 eyeDir = Normalized(eye);
            Vec3 camUp = Normalized(lastClickedUp);
            Vec3 camSideways = Normalized(Cross(camUp, eyeDir));
            movementDir = movementDir.y * camUp + movementDir.x * camSideways;
            Vec3 axis = Normalized(Cross(movementDir, eye));
            
            Quaternion q = Quaternion::FromAxisAngle(axis, angle);
            impl->cameraUp = Rotate(lastClickedUp, q);
            impl->cameraPosition = Rotate(eye, q) + impl->cameraTarget;
        }
    }

    auto& io = ImGui::GetIO();

    // Zoom
    if (hovered)
    {
        float zoomDelta = io.MouseWheel;

        Vec3 eye = impl->cameraPosition - impl->cameraTarget;
        // Scale it down by some percentage each zoomDelta of 1.
        eye = (1.0f - (impl->cameraZoomSpeed*zoomDelta*0.01f)) * eye;
        impl->cameraPosition = eye + impl->cameraTarget;
        ImGui::Text("Mouse Wheel %f", zoomDelta);
    }


    //Pan camera.
    if (hovered && io.MouseDown[ImGuiMouseButton_Right])
    {
        // Pan by moving the target and camera position, keeping the eye vector constant. 
        // We pan perpendicularly to the eye direction.
        Vec3 eye = lastClickedCamPos - lastClickedTarget;
        Vec3 upDir = Normalized(impl->cameraUp);
        Vec3 rightDir = Normalized(Cross(upDir, eye));
        // Update camera information here.
        ImVec2 currentPos = ImGui::GetMousePos();
        //Convert mouse position to [-1,1] in the framebuffer coordinates.
        ImVec2 size = image_bb.GetSize();
        //Clicked position from [-1,1]
        ImVec2 lastClickedCam(2 * (lastClickedPos.x - image_bb.Min.x) / size.x - 1, 2 * (lastClickedPos.y - image_bb.Min.y) / size.y - 1);
        ImVec2 currentPosCam(2 * (currentPos.x - image_bb.Min.x) / size.x - 1, 2 * (currentPos.y - image_bb.Min.y) / size.y - 1);
        Vec3 movementDir{ currentPosCam.x - lastClickedCam.x, currentPosCam.y - lastClickedCam.y, 0.f };
        impl->cameraPosition = lastClickedCamPos + upDir * movementDir.y;
        impl->cameraPosition = impl->cameraPosition + rightDir * movementDir.x;
        impl->cameraTarget = impl->cameraPosition - eye;

        ImGui::Text("Held!");
    }

    ImGui::SliderFloat3("Camera Target", (float*)(&impl->cameraTarget), -1, 1);
    ImGui::SliderFloat("Camera near", &impl->nearPlane, 0, 1);
    ImGui::SliderFloat("Camera far", &impl->farPlane, 0, 1000);
    ImGui::SliderFloat("Camera FOV", &impl->horizontalFovDegrees, 0, 90);
    ImGui::SliderFloat("Camera Speed (Rot)", &impl->cameraRotateSpeed, 0, 5);
    ImGui::SliderFloat("Camera Speed (Zoom)", &impl->cameraZoomSpeed, 0, 5);

}