#include <fstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fmt/core.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

float window_width = 800;
float window_height = 600;

struct Circle
{
    float radius;
    glm::vec2 position;
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    window_width = width;
    window_height = height;
    glViewport(0, 0, width, height);
}

GLFWwindow* initialize()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    auto window = glfwCreateWindow(window_width, window_height, "Circle skinning", NULL, NULL);

    if (window == nullptr)
    {
        fmt::println("Failed to create window");
        return nullptr;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        fmt::println("Failed to initialize GLAD");
        return nullptr;
    }

    return window;
}

std::string read_shader(std::string path)
{
    std::string result;

    std::string line;
    std::ifstream file(path);

    while (std::getline(file, line))
    {
        result += line;
        result += "\n";
    }

    return result;
}

unsigned int get_shader_program()
{
    auto vertex_shader = read_shader("src/vertex.glsl");
    auto fragment_shader = read_shader("src/fragment.glsl");

    const char *vertex_shader_source = vertex_shader.c_str();
    const char *fragment_shader_source = fragment_shader.c_str();

    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertex_shader_source, nullptr);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragment_shader_source, nullptr);
    glCompileShader(fragmentShader);

    unsigned int shader_program = glCreateProgram();
    glAttachShader(shader_program, vertexShader);
    glAttachShader(shader_program, fragmentShader);

    glLinkProgram(shader_program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shader_program;
}

void render_circles(unsigned int vbo, unsigned int vao, std::vector<Circle> circles)
{
    auto vertices = std::vector<glm::vec3>();

    auto alpha = 2 * glm::pi<float>() / 100.0f;

    for (auto circle : circles)
    {
        for (int i = 0; i < 100; i++)
        {
            vertices.push_back(glm::vec3(circle.position.x, circle.position.y, 1));
            vertices.push_back(glm::vec3(circle.position.x + circle.radius * glm::sin(alpha * (i + 1)), circle.position.y +  circle.radius * glm::cos(alpha * (i + 1)), 1));
            vertices.push_back(glm::vec3(circle.position.x + circle.radius * glm::sin(alpha * i), circle.position.y + circle.radius * glm::cos(alpha * i), 1));
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), &vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    glBindVertexArray(0);
}

int main()
{
    auto window = initialize();

    if (window == nullptr)
    {
        glfwTerminate();
        return 1;
    }

    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    auto shader_program = get_shader_program();

    auto model_uniform = glGetUniformLocation(shader_program, "model");
    auto projection_uniform = glGetUniformLocation(shader_program, "projection");

    auto model = glm::mat4(1);

    unsigned int circle_vbo;
    unsigned int circle_vao;

    glGenBuffers(1, &circle_vbo);
    glGenVertexArrays(1, &circle_vao);

    glBindVertexArray(circle_vao);
    glBindBuffer(GL_ARRAY_BUFFER, circle_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0); 

    std::vector<Circle> circles = {
        Circle
        {
            .radius = .1,
            .position = glm::vec2(0, 0),
        },
        Circle
        {
            .radius = .2,
            .position = glm::vec2(2, 2),
        }
    };

    while(!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader_program);

        auto projection = glm::ortho(0.0f, window_width, window_height, 0.0f, -1.0f, 1.0f);

        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));

        render_circles(circle_vbo, circle_vao, circles);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
