#include <GLFW/glfw3.h>
#include <OpenGL/gl3.h>
#include <iostream>
#include <math.h>

// Vertex shader source code with lighting support
const char *vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aColor;
    layout (location = 2) in vec3 aNormal;

    out vec3 fragColor;
    out vec3 fragPos;
    out vec3 normal;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        fragPos = vec3(model * vec4(aPos, 1.0));
        normal = mat3(transpose(inverse(model))) * aNormal;
        fragColor = aColor;
    }
)";

// Fragment shader source code with lighting
const char *fragmentShaderSource = R"(
    #version 330 core
    in vec3 fragColor;
    in vec3 fragPos;
    in vec3 normal;

    out vec4 FragColor;

    uniform vec3 lightPos;
    uniform vec3 viewPos;

    void main()
    {
        // Ambient light
        float ambientStrength = 0.3;
        vec3 ambient = ambientStrength * fragColor;

        // Diffuse light
        vec3 norm = normalize(normal);
        vec3 lightDir = normalize(lightPos - fragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * fragColor;

        // Specular light
        float specularStrength = 0.5;
        vec3 viewDir = normalize(viewPos - fragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);

        // Combine lighting
        vec3 result = ambient + diffuse + specular;
        FragColor = vec4(result, 1.0);
    }
)";

void createRotationMatrixY(float angle, float matrix[16]) {
    float c = cos(angle);
    float s = sin(angle);

    matrix[0] = c;
    matrix[4] = 0.0f;
    matrix[8] = s;
    matrix[12] = 0.0f;
    matrix[1] = 0.0f;
    matrix[5] = 1.0f;
    matrix[9] = 0.0f;
    matrix[13] = 0.0f;
    matrix[2] = -s;
    matrix[6] = 0.0f;
    matrix[10] = c;
    matrix[14] = 0.0f;
    matrix[3] = 0.0f;
    matrix[7] = 0.0f;
    matrix[11] = 0.0f;
    matrix[15] = 1.0f;
}

void createPerspectiveMatrix(float fov, float aspect, float nearZ, float farZ, float matrix[16]) {
    float tanHalfFov = tan(fov / 2.0f);

    matrix[0] = 1.0f / (aspect * tanHalfFov);
    matrix[4] = 0.0f;
    matrix[8] = 0.0f;
    matrix[12] = 0.0f;
    matrix[1] = 0.0f;
    matrix[5] = 1.0f / tanHalfFov;
    matrix[9] = 0.0f;
    matrix[13] = 0.0f;
    matrix[2] = 0.0f;
    matrix[6] = 0.0f;
    matrix[10] = -(farZ + nearZ) / (farZ - nearZ);
    matrix[14] = -2.0f * farZ * nearZ / (farZ - nearZ);
    matrix[3] = 0.0f;
    matrix[7] = 0.0f;
    matrix[11] = -1.0f;
    matrix[15] = 0.0f;
}

void createViewMatrix(float eyeX, float eyeY, float eyeZ, float matrix[16]) {
    matrix[0] = 1.0f;
    matrix[4] = 0.0f;
    matrix[8] = 0.0f;
    matrix[12] = -eyeX;
    matrix[1] = 0.0f;
    matrix[5] = 1.0f;
    matrix[9] = 0.0f;
    matrix[13] = -eyeY;
    matrix[2] = 0.0f;
    matrix[6] = 0.0f;
    matrix[10] = 1.0f;
    matrix[14] = -eyeZ;
    matrix[3] = 0.0f;
    matrix[7] = 0.0f;
    matrix[11] = 0.0f;
    matrix[15] = 1.0f;
}

void createTranslationMatrix(float x, float y, float z, float matrix[16]) {
    matrix[0] = 1.0f;
    matrix[4] = 0.0f;
    matrix[8] = 0.0f;
    matrix[12] = x;
    matrix[1] = 0.0f;
    matrix[5] = 1.0f;
    matrix[9] = 0.0f;
    matrix[13] = y;
    matrix[2] = 0.0f;
    matrix[6] = 0.0f;
    matrix[10] = 1.0f;
    matrix[14] = z;
    matrix[3] = 0.0f;
    matrix[7] = 0.0f;
    matrix[11] = 0.0f;
    matrix[15] = 1.0f;
}

// Add this function to multiply two 4x4 matrices
void multiplyMatrices(float a[16], float b[16], float result[16]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result[i + j * 4] = 0;
            for (int k = 0; k < 4; k++) {
                result[i + j * 4] += a[i + k * 4] * b[k + j * 4];
            }
        }
    }
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required for macOS

    // Create window
    GLFWwindow *window = glfwCreateWindow(800, 600, "OpenGL Volumetric Triangles", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Define vertices for two tetrahedrons (position, color, normal)
    float vertices[] = {
            // Tetrahedron 1
            // Face 1
            -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, // Bottom left
            0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // Bottom right
            0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // Top

            // Face 2
            -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, -0.5f, -0.5f, -0.5f, // Bottom left
            0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, -0.5f, -0.5f, -0.5f, // Bottom right
            0.0f, 0.0f, -0.5f, 1.0f, 1.0f, 1.0f, -0.5f, -0.5f, -0.5f, // Apex

            // Face 3
            0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.0f, -0.5f, // Bottom right
            0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.0f, -0.5f, // Top
            0.0f, 0.0f, -0.5f, 1.0f, 1.0f, 1.0f, 0.5f, 0.0f, -0.5f, // Apex

            // Face 4
            0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, -0.5f, // Top
            -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.5f, -0.5f, // Bottom left
            0.0f, 0.0f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.5f, -0.5f, // Apex

            // Tetrahedron 2 (offset to be visible)
            // Face 1
            -0.3f, -0.3f, -0.7f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // Bottom left
            0.3f, -0.3f, -0.7f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, // Bottom right
            0.0f, 0.3f, -0.7f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // Top

            // Face 2
            -0.3f, -0.3f, -0.7f, 1.0f, 1.0f, 0.0f, -0.5f, -0.5f, -0.5f, // Bottom left
            0.3f, -0.3f, -0.7f, 0.0f, 1.0f, 1.0f, -0.5f, -0.5f, -0.5f, // Bottom right
            0.0f, 0.0f, -1.2f, 0.5f, 0.5f, 0.5f, -0.5f, -0.5f, -0.5f, // Apex

            // Face 3
            0.3f, -0.3f, -0.7f, 0.0f, 1.0f, 1.0f, 0.5f, 0.0f, -0.5f, // Bottom right
            0.0f, 0.3f, -0.7f, 1.0f, 0.0f, 1.0f, 0.5f, 0.0f, -0.5f, // Top
            0.0f, 0.0f, -1.2f, 0.5f, 0.5f, 0.5f, 0.5f, 0.0f, -0.5f, // Apex

            // Face 4
            0.0f, 0.3f, -0.7f, 1.0f, 0.0f, 1.0f, 0.0f, 0.5f, -0.5f, // Top
            -0.3f, -0.3f, -0.7f, 1.0f, 1.0f, 0.0f, 0.0f, 0.5f, -0.5f, // Bottom left
            0.0f, 0.0f, -1.2f, 0.5f, 0.5f, 0.5f, 0.0f, 0.5f, -0.5f // Apex
    };

    // Create and compile vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // Check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
    }

    // Create and compile fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
    }

    // Create shader program
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
    }

    // Delete shaders as they're linked into our program and no longer necessary
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Create VAO and VBO
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // Bind VAO first, then bind and set VBO, and then configure vertex attributes
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Normal attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *) (6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Get uniform locations
    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
    unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    unsigned int lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
    unsigned int viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        // Clear the color and depth buffer
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Activate shader program
        glUseProgram(shaderProgram);

        // Camera
        float eyeX = 0.0f;
        float eyeY = 1.0f;
        float eyeZ = 20.0f; // Fixed position in front of the objects

        // Lighting Pos
        float lightX = 0.0f;
        float lightY = 3.0f;
        float lightZ = 2.0f;
        glUniform3f(lightPosLoc, lightX, lightY, lightZ);

        // View Pos
        glUniform3f(viewPosLoc, eyeX, eyeY, eyeZ);

        // View Matrix
        float viewMatrix[16];
        createViewMatrix(eyeX, eyeY, eyeZ, viewMatrix);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, viewMatrix);

        // Projection Matrix
        float aspectRatio = 800.0f / 600.0f;
        float projMatrix[16];
        createPerspectiveMatrix(45.0f * (3.14159f / 180.0f), aspectRatio, 0.1f, 200.0f, projMatrix);
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projMatrix);

        float timeValue = glfwGetTime();

        // First Pyramid
        float translationMatrix1[16];

        // Translation T
        createTranslationMatrix(0.0f, 0.0f, -5.0f, translationMatrix1);

        // Rotation R
        float rotationMatrix1[16];
        createRotationMatrixY(timeValue, rotationMatrix1);

        // Transform = TR
        float transform[16];
        multiplyMatrices(translationMatrix1, rotationMatrix1, transform);

        // Set model matrix for first tetrahedron
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, transform);

        // Draw first Pyramid
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 12); // First 12 vertices (4 faces)

        // Create orbit motion for the second Pyramid
        float orbitRadius = 4.0f;
        float orbitX = sin(timeValue) * orbitRadius;
        float orbitZ = sin(timeValue) * orbitRadius;

        // Second Pyramid
        // Trans
        float translationMatrix2[16];
        createTranslationMatrix(orbitX, 0.0f, orbitZ, translationMatrix2);

        // Rot
        float rotationMatrix2[16];
        createRotationMatrixY(timeValue * 1.5f, rotationMatrix2);

        // A = TR
        float transform2[16];
        multiplyMatrices(translationMatrix2, rotationMatrix2, transform2);

        // Set transform
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, transform2);

        // Draw 2nd Pyramid
        glDrawArrays(GL_TRIANGLES, 12, 12); // Last 12 vertices (4 faces)

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up resources
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    // Terminate GLFW
    glfwTerminate();
    return 0;
}
