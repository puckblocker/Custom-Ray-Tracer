// Program: This program aims to create a whitted ray tracer with simply lighting
// and shading using the CPU with OpenGL and the accompanying GLM library
// Author: Seth Hird
#include "config.h"
#include "rayData.h"
#include "viewport.h"
#include "intersection.h"
#include "lighting.h"
#include "renderer.h"

using namespace std;

// VERTEX SHADER (Sets vertices)
const char *vertexShaderSource = "#version 330 core\n"
                                 "layout (location = 0) in vec2 aPos;\n" // gets inputs for attribute position aPos(bridge to rest of code look to glVertexAttribPointer with 0)
                                 "layout (location = 1) in vec2 aTexCoord;\n"
                                 "out vec2 TexCoord;\n" // output to fragment shader
                                 "void main()\n"
                                 "{\n"
                                 "   gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);\n" // sets vertex position
                                 "   TexCoord = aTexCoord;\n"                         // passes texture coord further down pipeline
                                 "}\0";

// FRAGMENT SHADER (Sets color)
const char *fragmentShaderSource = "#version 330 core\n"
                                   "out vec4 FragColor;\n"              // final color output
                                   "in vec2 TexCoord;\n"                // vertex shader input
                                   "uniform sampler2D screenTexture;\n" // represents the image
                                   "void main()\n"
                                   "{\n"
                                   "   FragColor = texture(screenTexture, TexCoord);\n" // looks at color at the coords
                                   "}\n\0";

int main()
{
    // Variables
    GLFWwindow *window; //  create window
    Renderer renderer;
    renderer.loadScene("scene.txt");
    const int resWidth = 640;
    const int resHeight = 640;
    // Create Pixel Buffer to Heap
    float *pixelBuffer = new float[resWidth * resHeight * 3]; // holds total pixel count * total color count (RGB) to give each pixel it's own RGB variables

    // ERROR CHECKERS + SETUP
    // GLFW error checker
    if (!glfwInit())
    {
        cout << "GLFW failed to start.\n";
        return -1;
    }

    // Set window stats
    window = glfwCreateWindow(640, 640, "My Window", NULL, NULL); // sets window stats like resolution and full screen, etc
    glfwMakeContextCurrent(window);                               // Sets window to the context we'll be rendering to

    // Glad error checker
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) // tells glad to run through system with the location of all function definitions
    {
        glfwTerminate();
        return -1;
    }

    // Create Reference to Store Vertex Shader / Compile Shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);     // create shader and get reference value
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL); // feed vertex shader the source code from before main
    glCompileShader(vertexShader);                              // compile source code

    // Create Reference to Store Fragment Shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);     // create shader and get reference value
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL); // feed fragment shader the source code from before main
    glCompileShader(fragmentShader);                                // compile source code

    // Wrap Up Shaders Into Shader Program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader); // attach shaders to shader program
    glLinkProgram(shaderProgram);                  // wraps up shader program
    glDeleteShader(vertexShader);                  // delete shaders (they are alreday implemented)
    glDeleteShader(fragmentShader);

    // Setup Quad Geometry (Two Triangles)
    GLfloat quadVertices[] = {
        // Triangle Coords & Texture Coords (Screen Coverage (0-1))
        // Triangle 1
        -1.0f, 1.0f, 0.0f, 1.0f,  // top left
        -1.0f, -1.0f, 0.0f, 0.0f, // bottom left
        1.0f, -1.0f, 1.0f, 0.0f,  // bottom right
        // Triangle 2
        1.0f, -1.0f, 1.0f, 0.0f, // bottom right
        1.0f, 1.0f, 1.0f, 1.0f,  // top right
        -1.0f, 1.0f, 0.0f, 1.0f  // top left
    };

    // CREATE VERTICES BUFFER & VERTICES ARRAY (slow to send things between CPU to GPU, this sends the data in batches)
    GLuint VAO, VBO; // reference to store vertex data (VBO Vertex Buffer Object)
                     // stores pointers to VBOs and tells OpenGL how to interpret them (VAO Vertex Array Object)
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO); // creates buffer object with 1 3D object

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);                                                // binds object making the object the current object (changes to binded object change current object)
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW); // stores vertices in VBO

    // Position
    int stride = 4 * sizeof(float);                                     // how many bytes OpenGl needs to skip to get to next vertex in buffer
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void *)0); // tells openGL how to read VBO (void* 0 is because vertices begin at start of array)
    glEnableVertexAttribArray(0);                                       // enables ^
    // Texture Coords
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void *)(2 * sizeof(float))); // different void to represent offset since vertices are 4 bytes apart
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0); // combines VAO & VBO to prevent errors <
    glBindVertexArray(0);             // <

    // SETUP TEXTURE
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Allocate the memory on the GPU (NULL because we haven't uploaded data yet)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, resWidth, resHeight, 0, GL_RGB, GL_FLOAT, NULL); // creates a 2D texture imaghe (allocates GPU memory) / stores vertices in the texture

    // CALL RENDERER
    renderer.render(pixelBuffer, resWidth, resHeight);

    // OPEN WINDOW
    while (!glfwWindowShouldClose(window)) // keeps window up until closed by user
    {
        glfwPollEvents();                    // keeps event queue from overflowing (events are constantly being made)
        glClearColor(.75f, .5f, .75f, 1.0f); // set color that will be used to clear the screen
        glClear(GL_COLOR_BUFFER_BIT);        // clears screen with constant to tell which buffer to clear (color buffer)

        // renderer.render(pixelBuffer, resWidth, resHeight);

        // DISPLAY RAY TRACER
        // Create Image / Upload to GPU
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0.0f, 0.0f, resWidth, resHeight, GL_RGB, GL_FLOAT, pixelBuffer); // creates 2D texture sub image (copies data into existing memory)

        // Display
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6); // starting indices of triangles

        glfwSwapBuffers(window); // keeps display updated / swaps buffer
    }

    // Terminate
    delete[] pixelBuffer; // clean up heap
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate(); // terminate window
    return 0;        // terminate program
}