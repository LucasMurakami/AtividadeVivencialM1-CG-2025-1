#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>

// Shader sources
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform bool selected;
    
    out vec3 Normal;
    out vec3 FragPos;
    out vec3 SelColor;
    
    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        SelColor = selected ? vec3(0.9, 0.6, 0.1) : vec3(1.0, 1.0, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    
    in vec3 Normal;
    in vec3 FragPos;
    in vec3 SelColor;
    
    uniform vec3 lightPos;
    uniform vec3 lightColor;
    
    void main() {
        // Ambient
        float ambientStrength = 0.3;
        vec3 ambient = ambientStrength * lightColor;
        
        // Diffuse
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;
        
        vec3 result = (ambient + diffuse) * SelColor;
        FragColor = vec4(result, 1.0);
    }
)";

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Camera settings
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

class OBJ {
public:
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<GLuint> indices;
    
    GLuint VAO, VBO, EBO, NBO;
    
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    
    std::string name;
    
    OBJ(const std::string& objFilePath) {
        name = objFilePath;
        loadOBJ(objFilePath);
        setupMesh();
    }
    
    ~OBJ() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteBuffers(1, &NBO);
    }
    
    void loadOBJ(const std::string& filePath);
    void setupMesh();
    void draw(GLuint shaderProgram, bool selected, bool wireframeMode); // Updated parameter list
};

void OBJ::loadOBJ(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return;
    }
    
    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec3> temp_normals;
    std::vector<GLuint> vertexIndices, normalIndices;
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        
        if (prefix == "v") {
            glm::vec3 vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            temp_vertices.push_back(vertex);
        } 
        else if (prefix == "vn") {
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            temp_normals.push_back(normal);
        } 
        else if (prefix == "f") {
            std::string vertexData;
            unsigned int vertexIndex, textureIndex, normalIndex;
            
            for (int i = 0; i < 3; i++) {
                iss >> vertexData;
                std::replace(vertexData.begin(), vertexData.end(), '/', ' ');
                std::istringstream vdss(vertexData);
                vdss >> vertexIndex;
                
                // Skip texture coordinate if present
                if (vertexData.find(' ') != std::string::npos) {
                    vdss >> textureIndex;
                    if (vertexData.find(' ', vertexData.find(' ') + 1) != std::string::npos) {
                        vdss >> normalIndex;
                        normalIndices.push_back(normalIndex - 1);
                    }
                }
                
                vertexIndices.push_back(vertexIndex - 1);
            }
        }
    }
    
    // Process indices
    for (unsigned int i = 0; i < vertexIndices.size(); i++) {
        GLuint vertexIndex = vertexIndices[i];
        vertices.push_back(temp_vertices[vertexIndex]);
        
        if (i < normalIndices.size()) {
            GLuint normalIndex = normalIndices[i];
            if (normalIndex < temp_normals.size()) {
                normals.push_back(temp_normals[normalIndex]);
            } else {
                normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
            }
        } else {
            normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
        }
        
        indices.push_back(i);
    }
    
    std::cout << "Loaded " << vertices.size() << " vertices from " << filePath << std::endl;
}

void OBJ::setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &NBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, NBO);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);
    
    glBindVertexArray(0);
}

void OBJ::draw(GLuint shaderProgram, bool selected, bool wireframeMode) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, scale);
    
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(glGetUniformLocation(shaderProgram, "selected"), selected ? 1 : 0);
    
    // Set polygon mode based on wireframe state if this object is selected
    if (selected && wireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // Reset polygon mode to fill after drawing this object
    if (selected && wireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

// Function declarations
GLuint createShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource);
void processInput(GLFWwindow* window, float deltaTime);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void displayHelp();

// Global variables
std::vector<OBJ*> objects;
int selectedObjectIndex = 0;
bool transformMode = false;  // false = translate, true = rotate
bool transformMode2 = false; // false = translate/rotate, true = scale
bool wireframeMode = false;  // false = solid, true = wireframe

// Transformation speed
float rotationSpeed = 50.0f;   // degrees per second
float translationSpeed = 2.0f; // units per second
float scaleSpeed = 1.0f;       // scale per second

void displayHelp() {
    std::cout << "==== 3D Object Viewer Controls ====\n";
    std::cout << "ESC - Exit application\n";
    std::cout << "TAB - Switch between objects\n\n";
    
    std::cout << "== Transformation Modes ==\n";
    std::cout << "1 - Rotation mode\n";
    std::cout << "2 - Translation mode\n";
    std::cout << "3 - Scale mode\n";
    std::cout << "4 - Toggle wireframe mode\n\n";
    
    std::cout << "== Controls (in respective modes) ==\n";
    std::cout << "W/S or Up/Down - Y-axis movement/rotation/scale\n";
    std::cout << "A/D or Left/Right - X-axis movement/rotation/scale\n";
    std::cout << "Q/E - Z-axis movement/rotation/scale\n";
    std::cout << "H - Show this help\n";
    std::cout << "===============================\n\n";
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OBJ Viewer", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    GLuint shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    
    glEnable(GL_DEPTH_TEST);
    
    try {
        objects.push_back(new OBJ("../assets/Suzanne.obj"));
        objects[0]->position = glm::vec3(-1.5f, 0.0f, 0.0f);
        
        objects.push_back(new OBJ("../assets/Suzanne.obj"));
        objects[1]->position = glm::vec3(1.5f, 0.0f, 0.0f);
    }
    catch(const std::exception& e) {
        std::cerr << "Error loading OBJ: " << e.what() << std::endl;
    }
    
    if (objects.empty()) {
        std::cout << "No objects loaded. Exiting." << std::endl;
        glfwTerminate();
        return -1;
    }
    
    // Show help at startup
    displayHelp();
    
    // Set light parameters
    glUseProgram(shaderProgram);
    glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 5.0f, 5.0f, 5.0f);
    glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
    
    float lastFrame = 0.0f;
    
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        processInput(window, deltaTime);
        
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glUseProgram(shaderProgram);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        
        for (size_t i = 0; i < objects.size(); i++) {
            objects[i]->draw(shaderProgram, i == selectedObjectIndex, wireframeMode); // Pass wireframeMode here
        }
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    for (auto obj : objects) {
        delete obj;
    }
    objects.clear();
    
    glfwTerminate();
    return 0;
}

GLuint createShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

void processInput(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
        displayHelp();
    }
    
    if (objects.empty() || selectedObjectIndex >= objects.size()) {
        return;
    }
    
    OBJ* selectedObj = objects[selectedObjectIndex];
    
    if (transformMode2) { // Scale mode
        float scaleChange = scaleSpeed * deltaTime;
        
        // Remove the uniform vs non-uniform condition, just keep the non-uniform scaling
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            selectedObj->scale.y += scaleChange;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            selectedObj->scale.y -= scaleChange;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            selectedObj->scale.x -= scaleChange;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            selectedObj->scale.x += scaleChange;
        }
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            selectedObj->scale.z += scaleChange;
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            selectedObj->scale.z -= scaleChange;
        }
        
        selectedObj->scale = glm::max(selectedObj->scale, glm::vec3(0.1f));
    } else if (transformMode) { // Rotation mode
        float rotChange = rotationSpeed * deltaTime;
        
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            selectedObj->rotation.x += rotChange;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            selectedObj->rotation.x -= rotChange;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            selectedObj->rotation.y += rotChange;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            selectedObj->rotation.y -= rotChange;
        }
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            selectedObj->rotation.z += rotChange;
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            selectedObj->rotation.z -= rotChange;
        }
        
        selectedObj->rotation.x = fmod(selectedObj->rotation.x, 360.0f);
        selectedObj->rotation.y = fmod(selectedObj->rotation.y, 360.0f);
        selectedObj->rotation.z = fmod(selectedObj->rotation.z, 360.0f);
    } else { // Translation mode
        float moveSpeed = translationSpeed * deltaTime;
        
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || 
            glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            selectedObj->position.y += moveSpeed;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || 
            glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            selectedObj->position.y -= moveSpeed;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || 
            glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            selectedObj->position.x -= moveSpeed;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || 
            glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            selectedObj->position.x += moveSpeed;
        }
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            selectedObj->position.z -= moveSpeed;
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            selectedObj->position.z += moveSpeed;
        }
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) {
        return;
    }
    
    switch (key) {
        case GLFW_KEY_TAB:
            if (!objects.empty()) {
                selectedObjectIndex = (selectedObjectIndex + 1) % objects.size();
                std::cout << "Selected object: " << selectedObjectIndex + 1 << "/" << objects.size()
                          << " (" << objects[selectedObjectIndex]->name << ")" << std::endl;
            }
            break;
        case GLFW_KEY_1:
            transformMode = true;
            transformMode2 = false;
            std::cout << "Mode: Rotation" << std::endl;
            break;
        case GLFW_KEY_2:
            transformMode = false;
            transformMode2 = false;
            std::cout << "Mode: Translation" << std::endl;
            break;
        case GLFW_KEY_3:
            transformMode = false;
            transformMode2 = true;
            std::cout << "Mode: Scale" << std::endl;  // Remove mention of uniform vs non-uniform
            break;
        case GLFW_KEY_4:
            // Toggle wireframe mode
            wireframeMode = !wireframeMode;
            std::cout << "Wireframe mode: " << (wireframeMode ? "ON" : "OFF") << std::endl;
            break;
        case GLFW_KEY_H:
            displayHelp();
            break;
    }
}