#include <fstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fmt/core.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

#define MAX_CIRCLE_SIZE 150.0f
#define MIN_CIRCLE_SIZE 5.0f
#define SKIN_POINT_SIZE 5.0f
#define LEFT_COLOR glm::vec3(1.0f, 0.0f, 0.0f)
#define RIGHT_COLOR glm::vec3(0.0f, 0.0f, 1.0f)
#define BALL_COLOR glm::vec3(0.0f, 1.0f, 0.0f)

float window_width = 800;
float window_height = 600;

glm::vec2 mouse_position = glm::vec2(0.0f);

int holded_circle_index = -1;

struct TouchingCircle
{
    float radius;
    glm::vec2 position;
};

struct CircleExternalTangentPoints
{
    glm::vec2 c1_p1;
    glm::vec2 c2_p1;
    glm::vec2 c1_p2;
    glm::vec2 c2_p2;
};

struct RadicalLine
{
    float a;
    float b;
    float c;
};

struct SeparatedPoints
{
    glm::vec2 left_point;
    glm::vec2 right_point;
};

class Circle
{
public:
    float radius;
    glm::vec2 position;
    std::vector<glm::vec2> vertices;
    glm::vec3 color;
    Circle(float r, glm::vec2 pos, glm::vec3 color = BALL_COLOR)
    {
        this->position = pos;
        this->radius = r;
        this->color = color;

        vertices = std::vector<glm::vec2>();

        auto alpha = 2 * glm::pi<float>() / 100.0f;

        for (int i = 0; i < 100; i++)
        {
            vertices.push_back(glm::vec2(0.0f));
            vertices.push_back(glm::vec2(glm::sin(alpha * (i + 1)), glm::cos(alpha * (i + 1))));
            vertices.push_back(glm::vec2(glm::sin(alpha * i), glm::cos(alpha * i)));
        }
    }
    std::vector<float> get_vertex_data()
    {
        std::vector<float> data;

        for (auto vertex : vertices)
        {
            data.push_back(vertex.x);
            data.push_back(vertex.y);
            data.push_back(color.x);
            data.push_back(color.y);
            data.push_back(color.z);
        }

        return data;
    }
};

class HermiteCurve
{
private:
    glm::vec2 p0;
    glm::vec2 p1;
    glm::vec2 v0;
    glm::vec2 v1;
    glm::vec3 color;
    glm::vec2 hermite(glm::vec2 p_cur, glm::vec2 p_next, glm::vec2 v_cur, glm::vec2 v_next, float t)
    {
        float h0 = 2 * glm::pow(t, 3) - 3 * glm::pow(t, 2) + 1;
        float h1 = -2 * glm::pow(t, 3) + 3 * glm::pow(t, 2);
        float h2 = glm::pow(t, 3) - 2 * glm::pow(t, 2) + t;
        float h3 = glm::pow(t, 3) - glm::pow(t, 2);

        return h0 * p_cur + h1 * p_next + h2 * v_cur + h3 * v_next;
    }
public:
    HermiteCurve(glm::vec2 p0, glm::vec2 p1, glm::vec2 v0, glm::vec2 v1, glm::vec3 color)
    {
        this->p0 = p0;
        this->p1 = p1;
        this->v0 = v0;
        this->v1 = v1;
        this->color = color;
    }
    std::vector<float> get_vertex_data(int segments)
    {
        std::vector<float> data;

        for (int i = 0; i <= segments; i++)
        {
            float t0 = (float)i / (float)segments;

            auto point = hermite(p0, p1, v0, v1, t0);

            data.push_back(point.x);
            data.push_back(point.y);
            data.push_back(color.x);
            data.push_back(color.y);
            data.push_back(color.z);
        }

        return data;
    }
};

std::vector<Circle> circles;
std::vector<Circle> point_circles;
std::vector<HermiteCurve> curves;

std::vector<glm::vec2> lines;

// Based on: https://math.stackexchange.com/questions/3100828/calculate-the-circle-that-touches-three-other-circles
TouchingCircle * find_touching_circle(Circle *c1, Circle *c2, Circle *c3, int s1, int s2, int s3)
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

    return new TouchingCircle{r, glm::vec2(x, y)};
}

CircleExternalTangentPoints * get_tangent_points(glm::vec2 c1_pos, float r1, glm::vec2 c2_pos, float r2)
{
    auto d = c2_pos - c1_pos;
    auto l = glm::length(d);
    auto u = d / l;
    auto v = glm::vec2(-d.y / l, d.x / l);

    auto p1 = c1_pos + r1 * ((r2 - r1) * u + l * v) / l;
    auto p2 = c2_pos + r2 * ((r2 - r1) * u + l * v) / l;
    auto p3 = c1_pos + r1 * ((r2 - r1) * u + l * -1 * v) / l;
    auto p4 = c2_pos + r2 * ((r2 - r1) * u + l * -1 * v) / l;

    return new CircleExternalTangentPoints{p1, p2, p3, p4};
}

bool get_if_circles_touch_externally_or_internally(glm::vec2 common_circle_pos, float common_circle_radius, glm::vec2 circle_pos, float circle_radius)
{
    auto distance = glm::distance(circle_pos, common_circle_pos);
    auto radius_diff = glm::abs(circle_radius - common_circle_radius);

    if (glm::abs(distance - radius_diff) < 0.1f)
    {
        // Circles touch internally
        return true;
    }

    // Circles touch externally (no need for further checks, since the circles are guaranteed to touch)
    return false;
}

std::tuple<glm::vec2, glm::vec2> find_curve_points_for_circle(int index)
{
    int s2_counter = 0;
    int s3_counter = 0;

    int s1 = 1;
    int s2 = 1;
    int s3 = 1;

    std::vector<glm::vec2> curve_points;

    for (auto i = 0; i < 8; i++)
    {
        auto touching_circle = find_touching_circle(&circles[index - 1], &circles[index], &circles[index + 1], s1, s2, s3);

        auto touching_point = touching_circle->position + glm::normalize(circles[index].position - touching_circle->position) * touching_circle->radius;

        auto control_orientation = get_if_circles_touch_externally_or_internally(touching_circle->position, touching_circle->radius, circles[index].position, circles[index].radius);

        auto all_same_orientation = true;

        for (auto circle : { &circles[index - 1], &circles[index + 1] })
        {
            auto orientation = get_if_circles_touch_externally_or_internally(touching_circle->position, touching_circle->radius, circle->position, circle->radius);

            if (orientation != control_orientation)
            {
                all_same_orientation = false;
                break;
            }
        }

        if (all_same_orientation)
        {
            curve_points.push_back(touching_point);
        }

        s2_counter++;
        s3_counter++;

        s1 *= -1;

        if (s2_counter % 2 == 0)
        {
            s2 *= -1;
        }

        if (s3_counter % 4 == 0)
        {
            s3 *= -1;
        }

        delete touching_circle;
    }

    return std::make_tuple(curve_points[0], curve_points[1]);
}

RadicalLine * get_radical_line(glm::vec2 c1_pos, float r1, glm::vec2 c2_pos, float r2)
{
    float a = 2 * (c2_pos.x - c1_pos.x);
    float b = 2 * (c2_pos.y - c1_pos.y);
    // float c = (glm::pow(c1_pos.x, 2) + glm::pow(c2_pos.x, 2)) - (glm::pow(c1_pos.y, 2) + glm::pow(c2_pos.y, 2)) - (glm::pow(r1, 2) + glm::pow(r2, 2));
    float c = (glm::pow(c1_pos.x, 2) - glm::pow(c2_pos.x, 2)) + (glm::pow(c1_pos.y, 2) - glm::pow(c2_pos.y, 2)) - (glm::pow(r1, 2) + glm::pow(r2, 2));

    return new RadicalLine{a, b, c};
}

glm::vec2 find_radical_center(glm::vec2 c1_pos, float r1, glm::vec2 c2_pos, float r2, glm::vec2 c3_pos, float r3)
{
    float x1 = c1_pos.x;
    float y1 = c1_pos.y;
    float x2 = c2_pos.x;
    float y2 = c2_pos.y;
    float x3 = c3_pos.x;
    float y3 = c3_pos.y;

    auto radical_line1 = get_radical_line(c1_pos, r1, c2_pos, r2);
    auto radical_line2 = get_radical_line(c2_pos, r1, c3_pos, r3);

    float a1 = radical_line1->a;
    float b1 = radical_line1->b;
    float c1 = radical_line1->c * -1;

    float a2 = radical_line2->a;
    float b2 = radical_line2->b;
    float c2 = radical_line2->c * -1;

    float d = a1 * b2 - a2 * b1;

    float x = (c1 * b2 - c2 * b1) / d;
    float y = (a1 * c2 - a2 * c1) / d;

    delete radical_line1;
    delete radical_line2;

    return glm::vec2(x, y);
}

glm::vec2 rotate_vector(glm::vec2 vec, float angle)
{
    float rad = glm::radians(angle);

    float cos = glm::cos(rad);
    float sin = glm::sin(rad);

    return glm::mat2x2(cos, -sin, sin, cos) * vec;
}

glm::vec2 flip_when_facing_opposite(glm::vec2 vec, glm::vec2 check_against)
{
    float dot = vec.x * check_against.x + vec.y * check_against.y;
    float det = vec.x * check_against.y - vec.y * check_against.x;

    float angle = glm::atan(det, dot);

    if (glm::abs(angle) > glm::radians(90.0f))
    {
        return -vec;
    }

    return vec;
}

std::tuple<glm::vec2, glm::vec2> calculate_tangents(glm::vec2 c1_pos, float r1, glm::vec2 c2_pos, float r2, glm::vec2 point1, glm::vec2 point2)
{
    auto radical_line = get_radical_line(c1_pos, r1, c2_pos, r2);

    float radical_distance_a = (glm::abs(radical_line->a * point1.x + radical_line->b * point1.y + radical_line->c)) / glm::sqrt(glm::pow(radical_line->a, 2) + glm::pow(radical_line->b, 2));
    float radical_distance_b = (glm::abs(radical_line->a * point2.x + radical_line->b * point2.y + radical_line->c)) / glm::sqrt(glm::pow(radical_line->a, 2) + glm::pow(radical_line->b, 2));

    fmt::println("c1: {}, {}, c2: {}, {}, a: {}, b: {}, c: {}", c1_pos.x, c1_pos.y, c2_pos.x, c2_pos.y, radical_line->a, radical_line->b, radical_line->c);

    auto p1_to_c1_vec = (c1_pos - point1) / glm::length(c1_pos - point1);
    auto p2_to_c2_vec = (c2_pos - point2) / glm::length(c2_pos - point2);

    delete radical_line;

    auto p1_to_p2_vec = point2 - point1;

    auto tangent1 = flip_when_facing_opposite(rotate_vector(p1_to_c1_vec, -90.0f) * 2.0f * radical_distance_a, p1_to_p2_vec);
    auto tangent2 = flip_when_facing_opposite(rotate_vector(p2_to_c2_vec, -90.0f) * 2.0f * radical_distance_b, p1_to_p2_vec);

    return std::make_tuple(tangent1, tangent2);
}

SeparatedPoints * separate_points(glm::vec2 point1, glm::vec2 point2, int index)
{
    auto radical_center = find_radical_center(
        circles[index].position, circles[index].radius,
        circles[index - 1].position, circles[index - 1].radius,
        circles[index + 1].position, circles[index + 1].radius);

    point_circles.push_back(Circle(SKIN_POINT_SIZE, radical_center, glm::vec3(0.0f, 1.0f, 1.0f)));

    glm::vec2 to_check = circles[index].position - circles[index - 1].position;
    glm::vec2 check_against = circles[index + 1].position - circles[index - 1].position;

    float dot = to_check.x * check_against.x + to_check.y * check_against.y;
    float det = to_check.x * check_against.y - to_check.y * check_against.x;

    float angle = glm::atan(det, dot);

    auto p1_radical_distance = glm::distance(radical_center, point1);
    auto p2_radical_distance = glm::distance(radical_center, point2);

    glm::vec2 left;
    glm::vec2 right;

    if (angle < 0)
    {
        if (p1_radical_distance < p2_radical_distance)
        {
            left = (point1);
            right = (point2);
        }
        else
    {
            left = (point2);
            right = (point1);
        }
    }
    else
    {
        if (p1_radical_distance < p2_radical_distance)
        {
            left = (point2);
            right = (point1);
        }
        else
    {
            left = (point1);
            right = (point2);
        }
    }

    return new SeparatedPoints{left, right };
}

void calculate_skin()
{
    if (circles.size() < 4)
    {
        return;
    }

    curves.clear();
    lines.clear();
    point_circles.clear();

    std::vector<glm::vec2> left_points;
    std::vector<glm::vec2> right_points;

    auto first_points = get_tangent_points(
        circles[0].position, circles[0].radius,
        circles[1].position, circles[1].radius);

    auto separate_points_first = separate_points(first_points->c1_p1, first_points->c1_p2, 0);
    left_points.push_back(separate_points_first->left_point);
    point_circles.push_back(Circle(SKIN_POINT_SIZE, separate_points_first->left_point, LEFT_COLOR));
    right_points.push_back(separate_points_first->right_point);
    point_circles.push_back(Circle(SKIN_POINT_SIZE, separate_points_first->right_point, RIGHT_COLOR));

    delete first_points;
    delete separate_points_first;

    for (auto i = 1; i < circles.size() - 1; i++)
    {
        auto points = find_curve_points_for_circle(i);

        auto separated_points = separate_points(std::get<0>(points), std::get<1>(points), i);

        left_points.push_back(separated_points->left_point);
        point_circles.push_back(Circle(SKIN_POINT_SIZE, separated_points->left_point, LEFT_COLOR));
        right_points.push_back(separated_points->right_point);
        point_circles.push_back(Circle(SKIN_POINT_SIZE, separated_points->right_point, RIGHT_COLOR));

        delete separated_points;
    }

    auto last_points = get_tangent_points(
        circles[circles.size() - 2].position, circles[circles.size() - 2].radius,
        circles[circles.size() - 1].position, circles[circles.size() - 1].radius);
    auto separate_points_last = separate_points(last_points->c2_p1, last_points->c2_p2, circles.size() - 2);
    left_points.push_back(separate_points_last->left_point);
    point_circles.push_back(Circle(SKIN_POINT_SIZE, separate_points_last->left_point, LEFT_COLOR));
    right_points.push_back(separate_points_last->right_point);
    point_circles.push_back(Circle(SKIN_POINT_SIZE, separate_points_last->right_point, RIGHT_COLOR));

    delete last_points;
    delete separate_points_last;

    for (auto i = 0; i < left_points.size() - 1; i++)
    {
        auto tanggents = calculate_tangents(
            circles[i].position, circles[i].radius,
            circles[i + 1].position, circles[i + 1].radius,
            left_points[i], left_points[i + 1]);

        // lines.push_back(left_points[i]);
        // lines.push_back(left_points[i] + std::get<0>(tanggents));
        //
        // lines.push_back(left_points[i + 1]);
        // lines.push_back(left_points[i + 1] + std::get<1>(tanggents));

        curves.push_back(HermiteCurve(
            left_points[i], left_points[i + 1],
            std::get<0>(tanggents), std::get<1>(tanggents),
            LEFT_COLOR));
    }

    for (auto i = 0; i < right_points.size() - 1; i++)
    {
        auto tanggents = calculate_tangents(
            circles[i].position, circles[i].radius,
            circles[i + 1].position, circles[i + 1].radius,
            right_points[i], right_points[i + 1]);

        // lines.push_back(right_points[i]);
        // lines.push_back(right_points[i] + std::get<0>(tanggents));
        //
        // lines.push_back(right_points[i + 1]);
        // lines.push_back(right_points[i + 1] + std::get<1>(tanggents));

        curves.push_back(HermiteCurve(
            right_points[i], right_points[i + 1],
            std::get<0>(tanggents), std::get<1>(tanggents),
            RIGHT_COLOR));
    }
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

        calculate_skin();
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
            calculate_skin();
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

        auto vertex_data = circle.get_vertex_data();

        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertex_data.size() * sizeof(vertex_data[0]), &vertex_data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glDrawArrays(GL_TRIANGLES, 0, vertex_data.size());
        glBindVertexArray(0);
    }
}

void render_curves(unsigned int vbo, unsigned int vao, std::vector<HermiteCurve> curves, GLint model_uniform)
{
    for (auto curve : curves)
    {
        auto model = glm::mat4(1.0f);

        auto vertex_data = curve.get_vertex_data(30);

        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));

        glLineWidth(3.0f);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertex_data.size() * sizeof(vertex_data[0]), &vertex_data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glDrawArrays(GL_LINE_STRIP, 0, vertex_data.size() / 5);
        glBindVertexArray(0);
    }
}

void render_lines(unsigned int vbo, unsigned int vao, std::vector<glm::vec2> lines, GLint model_uniform)
{
    std::vector<float> vertex_data;

    for (auto line_point : lines)
    {
        vertex_data.push_back(line_point.x);
        vertex_data.push_back(line_point.y);
        vertex_data.push_back(0.0f);
        vertex_data.push_back(0.0f);
        vertex_data.push_back(0.0f);
    }

    auto model = glm::mat4(1.0f);

    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));

    glLineWidth(1.0f);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertex_data.size() * sizeof(vertex_data[0]), &vertex_data[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glDrawArrays(GL_LINES, 0, vertex_data.size() / 5);
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

    glViewport(0, 0, window_width, window_height);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    auto shader_program = get_shader_program();

    auto model_uniform = glGetUniformLocation(shader_program, "model");
    auto projection_uniform = glGetUniformLocation(shader_program, "projection");

    unsigned int circle_vbo;
    unsigned int circle_vao;

    unsigned int hermite_vbo;
    unsigned int hermite_vao;

    unsigned int line_vbo;
    unsigned int line_vao;

    glGenBuffers(1, &circle_vbo);
    glGenVertexArrays(1, &circle_vao);

    glBindVertexArray(circle_vao);
    glBindBuffer(GL_ARRAY_BUFFER, circle_vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &hermite_vbo);
    glGenVertexArrays(1, &hermite_vao);

    glBindVertexArray(hermite_vao);
    glBindBuffer(GL_ARRAY_BUFFER, hermite_vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &line_vbo);
    glGenVertexArrays(1, &line_vao);

    glBindVertexArray(line_vao);
    glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    circles = std::vector<Circle>();
    point_circles = std::vector<Circle>();
    curves = std::vector<HermiteCurve>();

    lines = std::vector<glm::vec2>();

    while(!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader_program);

        auto projection = glm::ortho(0.0f, window_width, window_height, 0.0f, -1.0f, 1.0f);

        glUniformMatrix4fv(projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));

        render_circles(circle_vbo, circle_vao, circles, model_uniform);
        render_circles(circle_vbo, circle_vao, point_circles, model_uniform);
        render_curves(hermite_vbo, hermite_vao, curves, model_uniform);
        render_lines(line_vbo, line_vao, lines, model_uniform);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
