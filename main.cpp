// main.cpp
// This file requires C++11 (or later).

// --- Include system and third-party headers ---
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <btBulletDynamicsCommon.h>

#include <iostream>
#include <vector>
#include <cassert>

// --- Include Dear ImGui headers ---
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// --- Shader source code using raw string literals with a delimiter ---
const char* vertexShaderSource = R"SHADER(
#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 uMVP;
void main()
{
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)SHADER";

const char* fragmentShaderSource = R"SHADER(
#version 330 core
out vec4 FragColor;
uniform vec3 uColor;
void main()
{
    FragColor = vec4(uColor, 1.0);
}
)SHADER";

// --- Global variables ---
GLFWwindow* window = nullptr;
int windowWidth = 1280, windowHeight = 720;

// Camera parameters and control variables
glm::vec3 cameraPos   = glm::vec3(0.0f, 10.0f, 20.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, -0.3f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraSpeed = 10.0f; // units per second
float yaw   = -90.0f;
float pitch = 0.0f;
bool firstMouse = true;
float lastX = windowWidth / 2.0f;
float lastY = windowHeight / 2.0f;

// Input mode toggle:
// false = FPS mode (camera control, cursor hidden)
// true  = GUI/Interaction mode (cursor visible)
bool guiInputMode = false; // Change to 'true' for default GUI mode.

// Shader program ID
GLuint shaderProgram = 0;

// Bullet Physics globals
btDiscreteDynamicsWorld* dynamicsWorld = nullptr;
btRigidBody* pickedBody = nullptr;
btPoint2PointConstraint* pickConstraint = nullptr;

// Matrices for rendering
glm::mat4 projectionMatrix;
glm::mat4 viewMatrix;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// --- Utility Functions ---

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    if (!shader) {
        std::cerr << "Error creating shader!" << std::endl;
        exit(EXIT_FAILURE);
    }
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        char buffer[512];
        glGetShaderInfoLog(shader, 512, nullptr, buffer);
        std::cerr << "Shader Compile Error: " << buffer << std::endl;
    }
    return shader;
}

GLuint createShaderProgram(const char* vertexSrc, const char* fragmentSrc) {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSrc);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        char buffer[512];
        glGetProgramInfoLog(program, 512, nullptr, buffer);
        std::cerr << "Program Linking Error: " << buffer << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

// --- Cube Mesh Data ---
float cubeVertices[] = {
    // Back face
   -0.5f, -0.5f, -0.5f,    0.5f, -0.5f, -0.5f,    0.5f,  0.5f, -0.5f,
    0.5f,  0.5f, -0.5f,   -0.5f,  0.5f, -0.5f,   -0.5f, -0.5f, -0.5f,
    // Front face
   -0.5f, -0.5f,  0.5f,    0.5f, -0.5f,  0.5f,    0.5f,  0.5f,  0.5f,
    0.5f,  0.5f,  0.5f,   -0.5f,  0.5f,  0.5f,   -0.5f, -0.5f,  0.5f,
    // Left face
   -0.5f,  0.5f,  0.5f,   -0.5f,  0.5f, -0.5f,   -0.5f, -0.5f, -0.5f,
   -0.5f, -0.5f, -0.5f,   -0.5f, -0.5f,  0.5f,   -0.5f,  0.5f,  0.5f,
    // Right face
    0.5f,  0.5f,  0.5f,    0.5f,  0.5f, -0.5f,    0.5f, -0.5f, -0.5f,
    0.5f, -0.5f, -0.5f,    0.5f, -0.5f,  0.5f,    0.5f,  0.5f,  0.5f,
    // Bottom face
   -0.5f, -0.5f, -0.5f,    0.5f, -0.5f, -0.5f,    0.5f, -0.5f,  0.5f,
    0.5f, -0.5f,  0.5f,   -0.5f, -0.5f,  0.5f,   -0.5f, -0.5f, -0.5f,
    // Top face
   -0.5f,  0.5f, -0.5f,    0.5f,  0.5f, -0.5f,    0.5f,  0.5f,  0.5f,
    0.5f,  0.5f,  0.5f,   -0.5f,  0.5f,  0.5f,   -0.5f,  0.5f, -0.5f
};

GLuint cubeVAO, cubeVBO;

void setupCubeMesh() {
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

// --- UV Sphere Mesh Data ---
struct Mesh {
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;
    size_t indexCount;
};

Mesh createSphereMesh(unsigned int sectorCount = 20, unsigned int stackCount = 20, float radius = 0.5f) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    float x, y, z, xy;
    float sectorStep = 2 * glm::pi<float>() / sectorCount;
    float stackStep = glm::pi<float>() / stackCount;
    for (unsigned int i = 0; i <= stackCount; ++i) {
        float stackAngle = glm::pi<float>() / 2 - i * stackStep;
        xy = radius * cosf(stackAngle);
        z = radius * sinf(stackAngle);
        for (unsigned int j = 0; j <= sectorCount; ++j) {
            float sectorAngle = j * sectorStep;
            x = xy * cosf(sectorAngle);
            y = xy * sinf(sectorAngle);
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }
    for (unsigned int i = 0; i < stackCount; ++i) {
        unsigned int k1 = i * (sectorCount + 1);
        unsigned int k2 = k1 + sectorCount + 1;
        for (unsigned int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }
            if (i != (stackCount - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
    Mesh sphere;
    sphere.indexCount = indices.size();
    glGenVertexArrays(1, &sphere.VAO);
    glGenBuffers(1, &sphere.VBO);
    glGenBuffers(1, &sphere.EBO);
    glBindVertexArray(sphere.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphere.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    return sphere;
}

Mesh sphereMesh;

// --- Bullet Physics Setup ---
btDiscreteDynamicsWorld* initPhysics() {
    auto* collisionConfiguration = new btDefaultCollisionConfiguration();
    auto* dispatcher = new btCollisionDispatcher(collisionConfiguration);
    auto* overlappingPairCache = new btDbvtBroadphase();
    auto* solver = new btSequentialImpulseConstraintSolver;
    btDiscreteDynamicsWorld* world = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);
    world->setGravity(btVector3(0, -9.81f, 0));
    return world;
}

btRigidBody* createRigidBody(btCollisionShape* shape, float mass, const btTransform& transform) {
    bool isDynamic = (mass != 0.f);
    btVector3 localInertia(0, 0, 0);
    if (isDynamic)
        shape->calculateLocalInertia(mass, localInertia);
    auto* motionState = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, shape, localInertia);
    btRigidBody* body = new btRigidBody(rbInfo);
    dynamicsWorld->addRigidBody(body);
    return body;
}

// --- screenPosToWorldRay ---
// Convert screen (mouse) coordinates into a world-space ray direction.
glm::vec3 screenPosToWorldRay(double mouseX, double mouseY) {
    float x = (2.0f * static_cast<float>(mouseX)) / windowWidth - 1.0f;
    float y = 1.0f - (2.0f * static_cast<float>(mouseY)) / windowHeight;
    glm::vec3 rayNDS(x, y, 1.0f);
    glm::vec4 rayClip(rayNDS.x, rayNDS.y, -1.0f, 1.0f);
    glm::vec4 rayEye = glm::inverse(projectionMatrix) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
    glm::vec4 rayWorld = glm::inverse(viewMatrix) * rayEye;
    return glm::normalize(glm::vec3(rayWorld));
}

// --- Combined Cursor Position Callback ---
// When in FPS mode, update camera look.
// When in GUI mode, update picking (if any).
void combinedCursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (!guiInputMode) {
        // FPS mode: update camera look.
        if (firstMouse) {
            lastX = static_cast<float>(xpos);
            lastY = static_cast<float>(ypos);
            firstMouse = false;
        }
        float xoffset = static_cast<float>(xpos) - lastX;
        float yoffset = lastY - static_cast<float>(ypos);
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        float sensitivity = 0.1f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;
        yaw   += xoffset;
        pitch += yoffset;
        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);
    } else {
        // GUI mode: update picking constraint pivot if exists.
        if (pickConstraint) {
            glm::vec3 rayDir = screenPosToWorldRay(xpos, ypos);
            glm::vec3 newPivot = cameraPos + rayDir * 10.0f;
            pickConstraint->setPivotB(btVector3(newPivot.x, newPivot.y, newPivot.z));
        }
    }
}

// --- Mouse Button Callback ---
// In GUI mode, process object picking.
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (!guiInputMode)
        return;
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            double mouseX, mouseY;
            glfwGetCursorPos(window, &mouseX, &mouseY);
            glm::vec3 rayDir = screenPosToWorldRay(mouseX, mouseY);
            glm::vec3 rayFrom = cameraPos;
            glm::vec3 rayTo = cameraPos + rayDir * 1000.0f;
            btVector3 btRayFrom(rayFrom.x, rayFrom.y, rayFrom.z);
            btVector3 btRayTo(rayTo.x, rayTo.y, rayTo.z);
            btCollisionWorld::ClosestRayResultCallback rayCallback(btRayFrom, btRayTo);
            dynamicsWorld->rayTest(btRayFrom, btRayTo, rayCallback);
            if (rayCallback.hasHit()) {
                btRigidBody* body = const_cast<btRigidBody*>(btRigidBody::upcast(rayCallback.m_collisionObject));
                if (body && !(body->isStaticObject() || body->isKinematicObject())) {
                    pickedBody = body;
                    pickedBody->setActivationState(DISABLE_DEACTIVATION);
                    btVector3 pickPos = rayCallback.m_hitPointWorld;
                    btVector3 localPivot = body->getCenterOfMassTransform().inverse() * pickPos;
                    pickConstraint = new btPoint2PointConstraint(*body, localPivot);
                    dynamicsWorld->addConstraint(pickConstraint, true);
                }
            }
        } else if (action == GLFW_RELEASE) {
            if (pickConstraint) {
                dynamicsWorld->removeConstraint(pickConstraint);
                delete pickConstraint;
                pickConstraint = nullptr;
                if (pickedBody) {
                    pickedBody->forceActivationState(ACTIVE_TAG);
                    pickedBody->setDeactivationTime(0.f);
                    pickedBody = nullptr;
                }
            }
        }
    }
}

// --- Input Handling for Camera Movement ---
void processInput(GLFWwindow* window) {
    if (guiInputMode)
        return; // Don't move camera in GUI mode.
    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    float velocity = cameraSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraFront * velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraFront * velocity;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
}

// --- Key Callback ---
// Pressing ESC toggles to GUI mode (shows the cursor).
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        guiInputMode = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

// --- Rendering Helper Functions ---
void drawCube(const glm::mat4& model, const glm::vec3& color) {
    glUseProgram(shaderProgram);
    glm::mat4 mvp = projectionMatrix * viewMatrix * model;
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uMVP"), 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform3fv(glGetUniformLocation(shaderProgram, "uColor"), 1, glm::value_ptr(color));
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void drawSphere(const glm::mat4& model, const glm::vec3& color, const Mesh& mesh) {
    glUseProgram(shaderProgram);
    glm::mat4 mvp = projectionMatrix * viewMatrix * model;
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uMVP"), 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform3fv(glGetUniformLocation(shaderProgram, "uColor"), 1, glm::value_ptr(color));
    glBindVertexArray(mesh.VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indexCount), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// --- GUI Variables ---
bool showDemoWindow = false;
bool addBox = false;
bool addSphere = false;
bool deleteObjects = false;

// Global vectors for dynamic bodies and collision shapes.
std::vector<btRigidBody*> globalDynamicBodies;
std::vector<btCollisionShape*> collisionShapes;

// --- Main Function ---
int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed!" << std::endl;
        return -1;
    }
    // Set up OpenGL context (3.3 core)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    // Create window
    window = glfwCreateWindow(windowWidth, windowHeight, "Minimal Game Engine with GUI", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    // Set callbacks
    glfwSetCursorPosCallback(window, combinedCursorPosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetKeyCallback(window, keyCallback);
    // Start in FPS mode if guiInputMode is false; otherwise, in GUI mode.
    if (guiInputMode)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        firstMouse = true;
    }
    // Initialize GLAD
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        return -1;
    }
    glViewport(0, 0, windowWidth, windowHeight);
    glEnable(GL_DEPTH_TEST);
    // Create shader program
    shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    // Setup meshes
    setupCubeMesh();
    sphereMesh = createSphereMesh();
    // Initialize Bullet Physics
    dynamicsWorld = initPhysics();
    // Create a static ground plane
    btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), 0);
    btTransform groundTransform;
    groundTransform.setIdentity();
    groundTransform.setOrigin(btVector3(0, 0, 0));
    btRigidBody* groundBody = createRigidBody(groundShape, 0.f, groundTransform);
    collisionShapes.push_back(groundShape);
    // Create collision shapes for dynamic bodies
    btCollisionShape* boxShape = new btBoxShape(btVector3(1, 1, 1));
    collisionShapes.push_back(boxShape);
    btCollisionShape* sphereShape = new btSphereShape(0.5f);
    collisionShapes.push_back(sphereShape);
    // Create some initial dynamic boxes and spheres
    for (int i = 0; i < 5; ++i) {
        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(btVector3(-5 + i * 2.5, 5, 0));
        btRigidBody* body = createRigidBody(boxShape, 1.0f, transform);
        globalDynamicBodies.push_back(body);
    }
    for (int i = 0; i < 5; ++i) {
        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(btVector3(-5 + i * 2.5, 8, 3));
        btRigidBody* body = createRigidBody(sphereShape, 1.0f, transform);
        globalDynamicBodies.push_back(body);
    }
    // Setup camera matrices
    projectionMatrix = glm::perspective(glm::radians(45.0f),
                                        static_cast<float>(windowWidth) / windowHeight,
                                        0.1f, 1000.0f);
    viewMatrix = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    // --- Initialize Dear ImGui ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Process camera movement (only in FPS mode)
        processInput(window);
        // Step physics simulation
        dynamicsWorld->stepSimulation(1.f / 60.f);
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // Build GUI: Main Menu Bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Mode")) {
                if (ImGui::MenuItem("FPS Camera Mode", NULL, !guiInputMode)) {
                    guiInputMode = false;
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    firstMouse = true;
                }
                if (ImGui::MenuItem("GUI/Interaction Mode", NULL, guiInputMode)) {
                    guiInputMode = true;
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Add Object")) {
                if (ImGui::MenuItem("Add Box"))
                    addBox = true;
                if (ImGui::MenuItem("Add Sphere"))
                    addSphere = true;
                if (ImGui::MenuItem("Delete Objects"))
                    deleteObjects = true;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Options")) {
                ImGui::MenuItem("Demo Window", NULL, &showDemoWindow);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
        if (showDemoWindow)
            ImGui::ShowDemoWindow(&showDemoWindow);
        // Simple editor window
        ImGui::Begin("Scene Editor");
        ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", cameraPos.x, cameraPos.y, cameraPos.z);
        if (ImGui::Button("Reset Camera")) {
            cameraPos = glm::vec3(0.0f, 10.0f, 20.0f);
            yaw = -90.0f;
            pitch = 0.0f;
        }
        ImGui::End();
        // Handle adding objects via GUI
        if (addBox) {
            btTransform transform;
            transform.setIdentity();
            transform.setOrigin(btVector3(cameraPos.x, cameraPos.y, cameraPos.z - 5));
            btRigidBody* body = createRigidBody(boxShape, 1.0f, transform);
            globalDynamicBodies.push_back(body);
            addBox = false;
        }
        if (addSphere) {
            btTransform transform;
            transform.setIdentity();
            transform.setOrigin(btVector3(cameraPos.x, cameraPos.y, cameraPos.z - 5));
            btRigidBody* body = createRigidBody(sphereShape, 1.0f, transform);
            globalDynamicBodies.push_back(body);
            addSphere = false;
        }
        if (deleteObjects) {
            globalDynamicBodies.clear();
            deleteObjects = false;
            addBox = false;
            addSphere = false;
        }
        // Render scene
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        viewMatrix = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        // Draw ground
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0, -0.05f, 0));
            model = glm::scale(model, glm::vec3(50, 0.1f, 50));
            drawCube(model, glm::vec3(0.3f, 0.8f, 0.3f));
        }
        // Draw dynamic objects
        for (btRigidBody* body : globalDynamicBodies) {
            btTransform trans;
            body->getMotionState()->getWorldTransform(trans);
            btScalar m[16];
            trans.getOpenGLMatrix(m);
            glm::mat4 model = glm::make_mat4(m);
            btCollisionShape* shape = body->getCollisionShape();
            if (shape->getShapeType() == BOX_SHAPE_PROXYTYPE) {
                model = glm::scale(model, glm::vec3(2.0f));
                drawCube(model, glm::vec3(0.8f, 0.3f, 0.3f));
            } else if (shape->getShapeType() == SPHERE_SHAPE_PROXYTYPE) {
                model = glm::scale(model, glm::vec3(1.0f));
                drawSphere(model, glm::vec3(0.3f, 0.3f, 0.8f), sphereMesh);
            }
        }
        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // Cleanup Bullet objects
    for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; --i) {
        btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        if (body && body->getMotionState())
            delete body->getMotionState();
        dynamicsWorld->removeCollisionObject(obj);
        delete obj;
    }
    delete groundShape;
    delete boxShape;
    delete sphereShape;
    delete dynamicsWorld;
    // Cleanup OpenGL resources
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &sphereMesh.VAO);
    glDeleteBuffers(1, &sphereMesh.VBO);
    glDeleteBuffers(1, &sphereMesh.EBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
