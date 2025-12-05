#include <fstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fmt/core.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

#define MAX_CIRCLE_SIZE 150.0f
#define MIN_CIRCLE_SIZE 5.0f

float window_width = 800;
float window_height = 600;

glm::vec2 mouse_position = glm::vec2(0.0f);

int holded_circle_index = -1;

class Circle
{
public:
    float radius;
    glm::vec2 position;
    std::vector<glm::vec2> vertices;
    Circle(float r, glm::vec2 pos)
    {
        this->position = pos;
        this->radius = r;

        vertices = std::vector<glm::vec2>();

        auto alpha = 2 * glm::pi<float>() / 100.0f;

        for (int i = 0; i < 100; i++)
        {
            vertices.push_back(glm::vec2(0.0f));
            vertices.push_back(glm::vec2(glm::sin(alpha * (i + 1)), glm::cos(alpha * (i + 1))));
            vertices.push_back(glm::vec2(glm::sin(alpha * i), glm::cos(alpha * i)));
        }
    }
};

std::vector<Circle> circles;

// Based on: https://math.stackexchange.com/questions/3100828/calculate-the-circle-that-touches-three-other-circles
Circle * find_touching_circle(Circle *c1, Circle *c2, Circle *c3, int s1, int s2, int s3)
{
    float r1 = s1 * c1->radius;
    float r2 = s2 * c2->radius;
    float r3 = s3 * c3->radius;

    float x1 = c1->position.x;
    float y1 = c1->position.y;
    float x2 = c2->position.x;
    float y2 = c2->position.y;
    float x3 = c3->position.x;
    float y3 = c3->position.y;

    fmt::println("r1={}, r2={}, r3={}", r1, r2, r3);

    float k_a = -glm::pow(r1, 2) + glm::pow(r2, 2) + glm::pow(x1, 2) - glm::pow(x2, 2) + glm::pow(y1, 2) - glm::pow(y2, 2);
    float k_b = -glm::pow(r1, 2) + glm::pow(r3, 2) + glm::pow(x1, 2) - glm::pow(x3, 2) + glm::pow(y1, 2) - glm::pow(y3, 2);

    float d = x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2);
    float a0 = (k_a * (y1 - y3) + k_b * (y2 - y1)) / (2 * d);
    float b0 = -(k_a * (x1 - x3) + k_b * (x2 - x1)) / (2 * d);

    float a1 = -(r1 * (y2 - y3) + r2 * (y3 - y1) + r3 * (y1 - y2)) / d;
    float b1 = (r1 * (x2 - x3) + r2 * (x3 - x1) + r3 * (x1 - x2)) / d;

    // float C0 = glm::pow(a0 - x1, 2) + glm::pow(b0 - y1, 2) - glm::pow(r1, 2);
    float C0 = glm::pow(a0, 2) - 2 * a0 * x1 + glm::pow(b0, 2) - 2 * b0 * y1 - glm::pow(r1, 2) + glm::pow(x1, 2) + glm::pow(y1, 2);
    // float C1 = a1 * (a0 - x1) + b1 * (b0 - y1) - r1;
    float C1 = a0 * a1 - a1 * x1 + b0 * b1 - b1 * y1 - r1;
    float C2 = glm::pow(a1, 2) + glm::pow(b1, 2) - 1;

    float r = (-glm::sqrt(glm::pow(C1, 2) - C0 * C2) - C1)/ C2;

    float x = a0 + a1 * r;
    float y = b0 + b1 * r;

    return new Circle(r, glm::vec2(x, y));
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    window_width = width;
    window_height = height;
    glViewport(0, 0, width, height);
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    mouse_position = glm::vec2(xpos, ypos);

    if (holded_circle_index != -1)
    {
        circles[holded_circle_index].position = mouse_position;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        for (auto i = 0; i < circles.size(); i++)
        {
            auto circle = circles[i];

            if (glm::distance(mouse_position, circle.position) <= circle.radius)
            {
                holded_circle_index = i;
                break;
            }
        }

        if (holded_circle_index == -1)
        {
            circles.push_back(Circle(50.0f, mouse_position));
        }
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        holded_circle_index = -1;
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        for (auto i = 0; i < circles.size(); i++)
        {
            auto circle = circles[i];

            if (glm::distance(mouse_position, circle.position) <= circle.radius)
            {
                circles.erase(circles.begin() + i);
                break;
            }
        }
        return;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (holded_circle_index != -1)
    {
        circles[holded_circle_index].radius += yoffset;

        if (circles[holded_circle_index].radius < MIN_CIRCLE_SIZE)
        {
            circles[holded_circle_index].radius = MIN_CIRCLE_SIZE;
        }
        else if (circles[holded_circle_index].radius > MAX_CIRCLE_SIZE)
        {
            circles[holded_circle_index].radius = MAX_CIRCLE_SIZE;
        }
    }
}

GLFWwindow* initialize()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
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

void render_circles(unsigned int vbo, unsigned int vao, std::vector<Circle> circles, GLint model_uniform)
{
    for (auto circle : circles)
    {
        auto model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(circle.position, 0.0f));
        model = glm::scale(model, glm::vec3(circle.radius, circle.radius, 1.0f));

        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, circle.vertices.size() * sizeof(circle.vertices[0]), &circle.vertices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glDrawArrays(GL_TRIANGLES, 0, circle.vertices.size());
        glBindVertexArray(0);
    }
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
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    auto shader_program = get_shader_program();

    auto model_uniform = glGetUniformLocation(shader_program, "model");
    auto projection_uniform = glGetUniformLocation(shader_program, "projection");

    unsigned int circle_vbo;
    unsigned int circle_vao;

    glGenBuffers(1, &circle_vbo);
    glGenVertexArrays(1, &circle_vao);

    glBindVertexArray(circle_vao);
    glBindBuffer(GL_ARRAY_BUFFER, circle_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    circles = std::vector<Circle>();

    while(!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader_program);

        auto projection = glm::ortho(0.0f, window_width, window_height, 0.0f, -1.0f, 1.0f);

        glUniformMatrix4fv(projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));

        render_circles(circle_vbo, circle_vao, circles, model_uniform);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
