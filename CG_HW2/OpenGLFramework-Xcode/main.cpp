#include <fstream>
#include <iostream>
#include <math.h>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "textfile.h"
#include "Matrices.h"
#include "Vectors.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
//
#include <unordered_map>
#include <functional>

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

using namespace std;

// Default window size
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;
int cur_WINDOW_WIDTH = WINDOW_WIDTH;
int cur_WINDOW_HEIGHT = WINDOW_HEIGHT;

bool mouse_pressed = false;
//int starting_press_x = -1;
//int starting_press_y = -1;

enum TransMode
{
    GeoTranslation = 0,
    GeoRotation = 1,
    GeoScaling = 2,
    LightEdit = 3,
    ShininessEdit = 4,
};

//vector<string> filenames; // .obj filename list

struct PhongMaterial
{
    Vector3 Ka;
    Vector3 Kd;
    Vector3 Ks;
};
struct UniformPhongMaterial
{
    GLint iLocKa;
    GLint iLocKs;
    GLint iLocKd;
};

typedef struct
{
    GLuint vao;
    GLuint vbo;
    GLuint vboTex;
    GLuint ebo;
    GLuint p_color;
    int vertex_count;
    GLuint p_normal;
    PhongMaterial material;
    int indexCount;
    GLuint m_texture;
} Shape;

struct model
{
    Vector3 position = Vector3(0, 0, 0);
    Vector3 scale = Vector3(1, 1, 1);
    Vector3 rotation = Vector3(0, 0, 0);    // Euler form

    vector<Shape> shapes;
};
vector<model> models;

struct camera
{
    Vector3 position;
    Vector3 center;
    Vector3 up_vector;
};
camera main_camera;

struct project_setting
{
    GLfloat nearClip, farClip;
    GLfloat fovy;
    GLfloat aspect;
    GLfloat left, right, top, bottom;
};
project_setting proj;

TransMode cur_trans_mode = TransMode::GeoTranslation;

Matrix4 view_matrix;
Matrix4 project_matrix;

int cur_idx = 0; // represent which model should be rendered now

#define numOfLightSources 3
struct LightInfo
{
    /*
    GLfloat position[3];
    GLfloat ambient[3];
    GLfloat diffuse[3];
    */
    Vector3 position;
    Vector3 ambient;
    Vector3 diffuse;
    Vector3 specular;
    GLfloat constantAttenuation;
    GLfloat linearAttenuation;
    GLfloat quadraticAttenuation;
};
LightInfo lightSources[numOfLightSources];


struct UniformLightInfo
{
    GLint iLocPosition;
    GLint iLocAmbient;
    GLint iLocDiffuse;
    GLint iLocSpecular;
    GLint iLocConstantAttenuation;
    GLint iLocLinearAttenuation;
    GLint iLocQuadraticAttenuation;
};
    
struct SpotLightInfo
{
    Vector3 spotDirection;
    GLfloat spotExponent;
    GLfloat spotCutOff;
};
SpotLightInfo spotLightInfo;

struct UniformSpotLight
{
    GLint iLocSpotDirection;
    GLint iLocSpotExponent;
    GLint iLocSpotCutOff;
};
    
struct Uniform
{
    GLint iLocMVP;
    GLint iLocM;
    GLint iLocCameraPosition;
    UniformLightInfo iLocLightInfo;
    UniformPhongMaterial iLocMaterial;
    UniformSpotLight iLocSpotLight;
    GLint iLocShininess;
    GLint iLocCurLightMode;
    GLint iLocIsPerPixLighting;
};
Uniform uniform;

int curLightMode = 0;
GLfloat shininess;

static GLvoid Normalize(GLfloat v[3])
{
    GLfloat l;

    l = (GLfloat)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    v[0] /= l;
    v[1] /= l;
    v[2] /= l;
}

static GLvoid Cross(GLfloat u[3], GLfloat v[3], GLfloat n[3])
{

    n[0] = u[1] * v[2] - u[2] * v[1];
    n[1] = u[2] * v[0] - u[0] * v[2];
    n[2] = u[0] * v[1] - u[1] * v[0];
}


// [TODO] given a translation vector then output a Matrix4 (Translation Matrix)
Matrix4 translate(Vector3 vec)
{
    Matrix4 mat;

    mat = Matrix4(
        1, 0, 0, vec.x,
        0, 1, 0, vec.y,
        0, 0, 1, vec.z,
        0, 0, 0,     1
    );

    return mat;
}

// [TODO] given a scaling vector then output a Matrix4 (Scaling Matrix)
Matrix4 scaling(Vector3 vec)
{
    Matrix4 mat;

    mat = Matrix4(
        vec.x, 0, 0, 0,
        0, vec.y, 0, 0,
        0, 0, vec.z, 0,
        0 ,0 ,0 ,    1
    );

    return mat;
}


// [TODO] given a float value then ouput a rotation matrix alone axis-X (rotate alone axis-X)
Matrix4 rotateX(GLfloat val)
{
    Matrix4 mat;

    GLfloat c = cos(val);
    GLfloat s = sin(val);
        
    mat = Matrix4(
        1, 0,  0, 0,
        0, c, -s, 0,
        0, s,  c, 0,
        0, 0,  0, 1
    );

    return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Y (rotate alone axis-Y)
Matrix4 rotateY(GLfloat val)
{
    Matrix4 mat;

    GLfloat c = cos(val);
    GLfloat s = sin(val);
       
   mat = Matrix4(
        c, 0, s, 0,
        0, 1, 0, 0,
       -s, 0, c, 0,
        0, 0, 0, 1
   );
    
    return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Z (rotate alone axis-Z)
Matrix4 rotateZ(GLfloat val)
{
    Matrix4 mat;

    GLfloat c = cos(val);
    GLfloat s = sin(val);

    mat = Matrix4(
        c, -s, 0, 0,
        s,  c, 0, 0,
        0,  0, 1, 0,
        0,  0, 0, 1
    );

    return mat;
}

Matrix4 rotate(Vector3 vec)
{
    return rotateX(vec.x)*rotateY(vec.y)*rotateZ(vec.z);
}

// [TODO] compute viewing matrix accroding to the setting of main_camera
void setViewingMatrix()
{
    Vector3 forward = main_camera.center - main_camera.position;
    GLfloat forward_arr[3] = {forward.x, forward.y, forward.z};
    Normalize(forward_arr);

    Vector3 up = main_camera.up_vector;
    GLfloat up_arr[3] = {up.x, up.y, up.z};

    GLfloat right_arr[3];
    Cross(forward_arr, up_arr, right_arr);
    Normalize(right_arr);

    GLfloat new_up_arr[3];
    Cross(right_arr, forward_arr, new_up_arr);

    Matrix4 R = Matrix4(
        right_arr[0], new_up_arr[0], -forward_arr[0], 0,
        right_arr[1], new_up_arr[1], -forward_arr[1], 0,
        right_arr[2], new_up_arr[2], -forward_arr[2], 0,
                 0,             0,               0, 1
    );

    Matrix4 T = Matrix4(
        1, 0, 0, -main_camera.position.x,
        0, 1, 0, -main_camera.position.y,
        0, 0, 1, -main_camera.position.z,
        0, 0, 0,                       1
    );

    view_matrix = R * T;

}

constexpr double PI = 3.14159265359;

inline float degree2radian(double degree)
{
    return static_cast<float> (degree * PI / 180.0);
}


// [TODO] compute persepective projection matrix
void setPerspective()
{
    GLfloat fovyRadians = degree2radian(proj.fovy);
    GLfloat f = 1.0 / tan(fovyRadians / 2);
    GLfloat z = (proj.farClip + proj.nearClip) / (proj.nearClip - proj.farClip);
    GLfloat t_z = (2 * proj.farClip * proj.nearClip) / (proj.nearClip - proj.farClip);

    project_matrix = Matrix4(
        f, 0,  0,   0,
        0, f,  0,   0,
        0, 0,  z,   t_z,
        0, 0, -1,   0
    );
    
    // Ensure orthographic projection matrix maintains the correct aspect ratio
    project_matrix[0] = f / (proj.aspect / 2);
    /*
    if (proj.aspect >= 1)
        project_matrix[0] = f / (proj.aspect / 2);
    else
        project_matrix[5] = f * (proj.aspect / 2);
     */
}

void setGLMatrix(GLfloat* glm, Matrix4& m) {
    glm[0] = m[0];  glm[4] = m[1];   glm[8] = m[2];    glm[12] = m[3];
    glm[1] = m[4];  glm[5] = m[5];   glm[9] = m[6];    glm[13] = m[7];
    glm[2] = m[8];  glm[6] = m[9];   glm[10] = m[10];   glm[14] = m[11];
    glm[3] = m[12];  glm[7] = m[13];  glm[11] = m[14];   glm[15] = m[15];
}

// Vertex buffers
//GLuint VAO, VBO;

// Call back function for window reshape
void ChangeSize(GLFWwindow* window, int width, int height)
{
    //glViewport(0, 0, width, height);
    // [TODO] change your aspect ratio
    if (width == 0 || height == 0)
        return;

    // Update aspect ratio
    proj.aspect = static_cast<float>(width) / height;

    // Update perspective projection matrix
    //setPerspective();
    float f = cos(degree2radian(proj.fovy) / 2) / sin(degree2radian(proj.fovy) / 2);
    project_matrix[0] = f / (proj.aspect / 2);

    /*
    float f = cos(degree2radian(proj.fovy) / 2) / sin(degree2radian(proj.fovy) / 2);
    if (proj.aspect >= 1)
        project_matrix[0] = f / (proj.aspect / 2);
    else
        project_matrix[5] = f * (proj.aspect / 2);
     */
    cur_WINDOW_WIDTH = width;
    cur_WINDOW_HEIGHT = height;
    
}

void transVec3ToShader(GLint iLoc, const Vector3 &v)
{
    glUniform3f(iLoc, v.x, v.y, v.z);
}

// Render function for display rendering
void RenderScene(void) {
    // clear canvas
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    // Cache the current model
    auto& currentModel = models.at(cur_idx);
    
    Matrix4 T, R, S;
    // [TODO] update translation, rotation and scaling
    T = translate(currentModel.position);
    R = rotate(currentModel.rotation);
    S = scaling(currentModel.scale);
    
    Matrix4 MVP;
    GLfloat mvp[16];
    GLfloat m[16];
    Matrix4 M;
    // [TODO] multiply all the matrix
    // MVP
    MVP = project_matrix * view_matrix * T * R * S ;
    M = T * R * S;
    
    // row-major ---> column-major
    setGLMatrix(mvp, MVP);
    setGLMatrix(m, M);

    // Use uniform to send mvp to vertex shader
    glUniformMatrix4fv(uniform.iLocMVP, 1, GL_FALSE, mvp);
    glUniformMatrix4fv(uniform.iLocM, 1, GL_FALSE, m);
    transVec3ToShader(uniform.iLocCameraPosition, main_camera.position);

    // Cache the light information
    auto& currentLightInfo = lightSources[curLightMode];

    transVec3ToShader(uniform.iLocLightInfo.iLocPosition, currentLightInfo.position);
    transVec3ToShader(uniform.iLocLightInfo.iLocAmbient, currentLightInfo.ambient);
    transVec3ToShader(uniform.iLocLightInfo.iLocDiffuse, currentLightInfo.diffuse);
    transVec3ToShader(uniform.iLocLightInfo.iLocSpecular, currentLightInfo.specular);
    glUniform1f(uniform.iLocLightInfo.iLocConstantAttenuation, currentLightInfo.constantAttenuation);
    glUniform1f(uniform.iLocLightInfo.iLocLinearAttenuation, currentLightInfo.linearAttenuation);
    glUniform1f(uniform.iLocLightInfo.iLocQuadraticAttenuation, currentLightInfo.quadraticAttenuation);

    transVec3ToShader(uniform.iLocSpotLight.iLocSpotDirection, spotLightInfo.spotDirection);
    glUniform1f(uniform.iLocSpotLight.iLocSpotExponent, spotLightInfo.spotExponent);
    glUniform1f(uniform.iLocSpotLight.iLocSpotCutOff, spotLightInfo.spotCutOff);

    glUniform1i(uniform.iLocCurLightMode, curLightMode);
    glUniform1f(uniform.iLocShininess, shininess);

    for (auto& shape : currentModel.shapes)
    {
        transVec3ToShader(uniform.iLocMaterial.iLocKa, shape.material.Ka);
        transVec3ToShader(uniform.iLocMaterial.iLocKd, shape.material.Kd);
        transVec3ToShader(uniform.iLocMaterial.iLocKs, shape.material.Ks);

        /* draw left */
        glUniform1i(uniform.iLocIsPerPixLighting, 0);
        glViewport(0, 0, cur_WINDOW_WIDTH / 2, cur_WINDOW_HEIGHT);

        glBindVertexArray(shape.vao);
        glDrawArrays(GL_TRIANGLES, 0, shape.vertex_count);

        /* draw right */
        glUniform1i(uniform.iLocIsPerPixLighting, 1);
        glViewport(cur_WINDOW_WIDTH / 2, 0, cur_WINDOW_WIDTH / 2, cur_WINDOW_HEIGHT);

        glBindVertexArray(shape.vao);
        glDrawArrays(GL_TRIANGLES, 0, shape.vertex_count);
    }
}


void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    // [TODO] Call back function for keyboard
    if (action == GLFW_PRESS)
    {
    // Define an unordered map to map key codes to actions
        std::unordered_map<int, std::function<void()>> key_actions = {
            { GLFW_KEY_Z, []() { cur_idx = (cur_idx == 0) ? static_cast<int>(models.size()) - 1 : cur_idx - 1;} },
            { GLFW_KEY_X, []() { cur_idx = (cur_idx == static_cast<int>(models.size()) - 1) ? 0 : cur_idx + 1; } },
            { GLFW_KEY_P, setPerspective },
            { GLFW_KEY_T, []() { cur_trans_mode = TransMode::GeoTranslation; } },
            { GLFW_KEY_S, []() { cur_trans_mode = TransMode::GeoScaling; } },
            { GLFW_KEY_R, []() { cur_trans_mode = TransMode::GeoRotation; } },
            // L -> switch among directional / point / spot light
            // curLightMode value go circular to be 0, 1, 2
            { GLFW_KEY_L, []() { curLightMode = (curLightMode == 2) ? 0 : curLightMode + 1; } },
            { GLFW_KEY_K, []() { cur_trans_mode = TransMode::LightEdit; } },
            { GLFW_KEY_J, []() { cur_trans_mode = TransMode::ShininessEdit; } }
        };
        // Call the action associated with the key code, if it exists
        auto it = key_actions.find(key);
        if (it != key_actions.end()) {
            it->second();
        }
    }
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    // [TODO] scroll up positive, otherwise it would be negtive
    float diff = static_cast<float>(yoffset);

    switch (cur_trans_mode)
    {
        case TransMode::GeoTranslation:
            models.at(cur_idx).position.z += diff / 10;
            break;

        case TransMode::GeoScaling:
            models.at(cur_idx).scale.z += diff / 10;
            break;

        case TransMode::GeoRotation:
            models.at(cur_idx).rotation.z += degree2radian(diff);
            break;

        case TransMode::LightEdit:
            if (curLightMode == 0 || curLightMode == 1)
            {
                lightSources[curLightMode].diffuse = Vector3(0.1f, 0.1f, 0.1f) * diff;
            }
            else if (curLightMode == 2)
            {
                spotLightInfo.spotCutOff -= diff;
                if (spotLightInfo.spotCutOff < 0)
                    spotLightInfo.spotCutOff = 0;
                else if (spotLightInfo.spotCutOff > 90)
                    spotLightInfo.spotCutOff = 90;
            }
            break;

        case TransMode::ShininessEdit:
            shininess += diff * 5;
            break;
            
        default:
            break;
    }
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    // [TODO] mouse press callback function
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
            mouse_pressed = true;
        else if (action == GLFW_RELEASE)
            mouse_pressed = false;
    }
}

double lastMouseX = 0, lastMouseY = 0;

static void cursor_pos_callback(GLFWwindow *window, double xpos, double ypos)
{
    // [TODO] cursor position callback function
    if (mouse_pressed)
    {
        const float diff_x = static_cast<float>(xpos - lastMouseX);
        const float diff_y = static_cast<float>(ypos - lastMouseY);
        switch (cur_trans_mode)
        {
            case TransMode::GeoTranslation:
                models.at(cur_idx).position.x += diff_x / 200;
                models.at(cur_idx).position.y -= diff_y / 200;
                break;
            case TransMode::GeoScaling:
                models.at(cur_idx).scale.x -= diff_x / 200;
                models.at(cur_idx).scale.y -= diff_y / 200;
                break;
            case TransMode::GeoRotation:
                models.at(cur_idx).rotation.x -= degree2radian(diff_y / 2);
                models.at(cur_idx).rotation.y -= degree2radian(diff_x / 2);
                break;
            case TransMode::LightEdit:
                lightSources[curLightMode].position.x += diff_x / 200;
                lightSources[curLightMode].position.y -= diff_y / 200;
                break;
            default:
                break;
        }
    }
    lastMouseX = xpos;
    lastMouseY = ypos;
}

void setShaders()
{
    GLuint v, f, p;
    char *vs = NULL;
    char *fs = NULL;

    v = glCreateShader(GL_VERTEX_SHADER);
    f = glCreateShader(GL_FRAGMENT_SHADER);

    vs = textFileRead("shader.vs");
    fs = textFileRead("shader.fs");

    glShaderSource(v, 1, (const GLchar**)&vs, NULL);
    glShaderSource(f, 1, (const GLchar**)&fs, NULL);

    free(vs);
    free(fs);

    GLint success;
    char infoLog[1000];
    // compile vertex shader
    glCompileShader(v);
    // check for shader compile errors
    glGetShaderiv(v, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(v, 1000, NULL, infoLog);
        std::cout << "ERROR: VERTEX SHADER COMPILATION FAILED\n" << infoLog << std::endl;
    }

    // compile fragment shader
    glCompileShader(f);
    // check for shader compile errors
    glGetShaderiv(f, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(f, 1000, NULL, infoLog);
        std::cout << "ERROR: FRAGMENT SHADER COMPILATION FAILED\n" << infoLog << std::endl;
    }

    // create program object
    p = glCreateProgram();

    // attach shaders to program object
    glAttachShader(p,f);
    glAttachShader(p,v);

    // link program
    glLinkProgram(p);
    // check for linking errors
    glGetProgramiv(p, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(p, 1000, NULL, infoLog);
        std::cout << "ERROR: SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(v);
    glDeleteShader(f);

    uniform.iLocMVP = glGetUniformLocation(p, "MVP");
    uniform.iLocM = glGetUniformLocation(p, "M");
    uniform.iLocCameraPosition = glGetUniformLocation(p, "cameraPosition");

    uniform.iLocMaterial.iLocKa = glGetUniformLocation(p, "material.Ka");
    uniform.iLocMaterial.iLocKd = glGetUniformLocation(p, "material.Kd");
    uniform.iLocMaterial.iLocKs = glGetUniformLocation(p, "material.Ks");

    uniform.iLocLightInfo.iLocPosition = glGetUniformLocation(p, "lightSources.position");
    uniform.iLocLightInfo.iLocAmbient = glGetUniformLocation(p, "lightSources.ambient");
    uniform.iLocLightInfo.iLocDiffuse = glGetUniformLocation(p, "lightSources.diffuse");
    uniform.iLocLightInfo.iLocSpecular = glGetUniformLocation(p, "lightSources.specular");
    uniform.iLocLightInfo.iLocConstantAttenuation =  glGetUniformLocation(p, "lightSources.constAttenuation");
    uniform.iLocLightInfo.iLocLinearAttenuation =    glGetUniformLocation(p, "lightSources.linearAttenuation");
    uniform.iLocLightInfo.iLocQuadraticAttenuation = glGetUniformLocation(p, "lightSources.quadraticAttenuation");

    uniform.iLocSpotLight.iLocSpotDirection = glGetUniformLocation(p, "spotLightInfo.spotDirection");
    uniform.iLocSpotLight.iLocSpotExponent = glGetUniformLocation(p, "spotLightInfo.spotExponent");
    uniform.iLocSpotLight.iLocSpotCutOff = glGetUniformLocation(p, "spotLightInfo.spotCutOff");

    uniform.iLocCurLightMode = glGetUniformLocation(p, "curLightMode");
    uniform.iLocShininess = glGetUniformLocation(p, "shininess");
    uniform.iLocIsPerPixLighting = glGetUniformLocation(p, "isPerPixLighting");

    if (success)
        glUseProgram(p);
    else
    {
        system("pause");
        exit(123);
    }
}

void normalization(tinyobj::attrib_t* attrib, vector<GLfloat>& vertices, vector<GLfloat>& colors, vector<GLfloat>& normals, tinyobj::shape_t* shape)
{
    vector<float> xVector, yVector, zVector;
    float minX = 10000, maxX = -10000, minY = 10000, maxY = -10000, minZ = 10000, maxZ = -10000;

    // find out min and max value of X, Y and Z axis
    for (int i = 0; i < attrib->vertices.size(); i++)
    {
        //maxs = max(maxs, attrib->vertices.at(i));
        if (i % 3 == 0)
        {

            xVector.push_back(attrib->vertices.at(i));

            if (attrib->vertices.at(i) < minX)
            {
                minX = attrib->vertices.at(i);
            }

            if (attrib->vertices.at(i) > maxX)
            {
                maxX = attrib->vertices.at(i);
            }
        }
        else if (i % 3 == 1)
        {
            yVector.push_back(attrib->vertices.at(i));

            if (attrib->vertices.at(i) < minY)
            {
                minY = attrib->vertices.at(i);
            }

            if (attrib->vertices.at(i) > maxY)
            {
                maxY = attrib->vertices.at(i);
            }
        }
        else if (i % 3 == 2)
        {
            zVector.push_back(attrib->vertices.at(i));

            if (attrib->vertices.at(i) < minZ)
            {
                minZ = attrib->vertices.at(i);
            }

            if (attrib->vertices.at(i) > maxZ)
            {
                maxZ = attrib->vertices.at(i);
            }
        }
    }

    float offsetX = (maxX + minX) / 2;
    float offsetY = (maxY + minY) / 2;
    float offsetZ = (maxZ + minZ) / 2;

    for (int i = 0; i < attrib->vertices.size(); i++)
    {
        if (offsetX != 0 && i % 3 == 0)
        {
            attrib->vertices.at(i) = attrib->vertices.at(i) - offsetX;
        }
        else if (offsetY != 0 && i % 3 == 1)
        {
            attrib->vertices.at(i) = attrib->vertices.at(i) - offsetY;
        }
        else if (offsetZ != 0 && i % 3 == 2)
        {
            attrib->vertices.at(i) = attrib->vertices.at(i) - offsetZ;
        }
    }

    float greatestAxis = maxX - minX;
    float distanceOfYAxis = maxY - minY;
    float distanceOfZAxis = maxZ - minZ;

    if (distanceOfYAxis > greatestAxis)
    {
        greatestAxis = distanceOfYAxis;
    }

    if (distanceOfZAxis > greatestAxis)
    {
        greatestAxis = distanceOfZAxis;
    }

    float scale = greatestAxis / 2;

    for (int i = 0; i < attrib->vertices.size(); i++)
    {
        //std::cout << i << " = " << (double)(attrib.vertices.at(i) / greatestAxis) << std::endl;
        attrib->vertices.at(i) = attrib->vertices.at(i) / scale;
    }
    size_t index_offset = 0;
    for (size_t f = 0; f < shape->mesh.num_face_vertices.size(); f++) {
        int fv = shape->mesh.num_face_vertices[f];

        // Loop over vertices in the face.
        for (size_t v = 0; v < fv; v++) {
            // access to vertex
            tinyobj::index_t idx = shape->mesh.indices[index_offset + v];
            vertices.push_back(attrib->vertices[3 * idx.vertex_index + 0]);
            vertices.push_back(attrib->vertices[3 * idx.vertex_index + 1]);
            vertices.push_back(attrib->vertices[3 * idx.vertex_index + 2]);
            // Optional: vertex colors
            colors.push_back(attrib->colors[3 * idx.vertex_index + 0]);
            colors.push_back(attrib->colors[3 * idx.vertex_index + 1]);
            colors.push_back(attrib->colors[3 * idx.vertex_index + 2]);
            // Optional: vertex normals
            if (idx.normal_index >= 0) {
                normals.push_back(attrib->normals[3 * idx.normal_index + 0]);
                normals.push_back(attrib->normals[3 * idx.normal_index + 1]);
                normals.push_back(attrib->normals[3 * idx.normal_index + 2]);
            }
        }
        index_offset += fv;
    }
}

string GetBaseDir(const string& filepath) {
    if (filepath.find_last_of("/\\") != std::string::npos)
        return filepath.substr(0, filepath.find_last_of("/\\"));
    return "";
}

void LoadModels(string model_path)
{
    vector<tinyobj::shape_t> shapes;
    vector<tinyobj::material_t> materials;
    tinyobj::attrib_t attrib;
    vector<GLfloat> vertices;
    vector<GLfloat> colors;
    vector<GLfloat> normals;

    string err;
    string warn;

    string base_dir = GetBaseDir(model_path); // handle .mtl with relative path

#ifdef _WIN32
    base_dir += "\\";
#else
    base_dir += "/";
#endif

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str(), base_dir.c_str());

    if (!warn.empty()) {
        cout << warn << std::endl;
    }

    if (!err.empty()) {
        cerr << err << std::endl;
    }

    if (!ret) {
        exit(1);
    }

    printf("Load Models Success ! Shapes size %d Material size %d\n", int(shapes.size()), int(materials.size()));
    model tmp_model;

    vector<PhongMaterial> allMaterial;
    for (int i = 0; i < materials.size(); i++)
    {
        PhongMaterial material;
        material.Ka = Vector3(materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2]);
        material.Kd = Vector3(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
        material.Ks = Vector3(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);
        allMaterial.push_back(material);
    }

    for (int i = 0; i < shapes.size(); i++)
    {

        vertices.clear();
        colors.clear();
        normals.clear();
        normalization(&attrib, vertices, colors, normals, &shapes[i]);
        // printf("Vertices size: %d", vertices.size() / 3);

        Shape tmp_shape;
        glGenVertexArrays(1, &tmp_shape.vao);
        glBindVertexArray(tmp_shape.vao);

        glGenBuffers(1, &tmp_shape.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GL_FLOAT), &vertices.at(0), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        tmp_shape.vertex_count = vertices.size() / 3;

        glGenBuffers(1, &tmp_shape.p_color);
        glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_color);
        glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GL_FLOAT), &colors.at(0), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glGenBuffers(1, &tmp_shape.p_normal);
        glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_normal);
        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GL_FLOAT), &normals.at(0), GL_STATIC_DRAW);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        // not support per face material, use material of first face
        if (allMaterial.size() > 0)
            tmp_shape.material = allMaterial[shapes[i].mesh.material_ids[0]];
        tmp_model.shapes.push_back(tmp_shape);
    }
    shapes.clear();
    materials.clear();
    models.push_back(tmp_model);
}

void initParameter()
{
    // [TODO] Setup some parameters if you need
    proj.left = -1;
    proj.right = 1;
    proj.top = 1;
    proj.bottom = -1;
    proj.nearClip = 0.001;
    proj.farClip = 100.0;
    proj.fovy = 80;
    proj.aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

    main_camera.position = Vector3(0.0f, 0.0f, 2.0f);
    main_camera.center = Vector3(0.0f, 0.0f, 0.0f);
    main_camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);
    
    /* directional light */
    lightSources[0].position = Vector3(1.0f, 1.0f, 1.0f);
    lightSources[0].ambient = Vector3(0.15f, 0.15f, 0.15f);
    lightSources[0].diffuse = Vector3(1.0f, 1.0f, 1.0f);
    lightSources[0].specular = Vector3(1.0f, 1.0f, 1.0f);
    lightSources[0].constantAttenuation = 0.0f;
    lightSources[0].linearAttenuation = 0.0f;
    lightSources[0].quadraticAttenuation = 0.0f;

    /* position light */
    lightSources[1].position = Vector3(0.0f, 2.0f, 1.0f);
    lightSources[1].ambient = Vector3(0.15f, 0.15f, 0.15f);
    lightSources[1].diffuse = Vector3(1.0f, 1.0f, 1.0f);
    lightSources[1].specular = Vector3(1.0f, 1.0f, 1.0f);
    lightSources[1].constantAttenuation = 0.01f;
    lightSources[1].linearAttenuation = 0.8f;
    lightSources[1].quadraticAttenuation = 0.1f;

    /* spot light */
    lightSources[2].position = Vector3(0.0f, 0.0f, 2.0f);
    lightSources[2].ambient = Vector3(0.15f, 0.15f, 0.15f);
    lightSources[2].diffuse = Vector3(1.0f, 1.0f, 1.0f);
    lightSources[2].specular = Vector3(1.0f, 1.0f, 1.0f);
    lightSources[2].constantAttenuation = 0.05f;
    lightSources[2].linearAttenuation = 0.3f;
    lightSources[2].quadraticAttenuation = 0.6f;
    spotLightInfo.spotDirection = Vector3(0.0f, 0.0f, -1.0f);
    spotLightInfo.spotExponent = 50.0f;
    spotLightInfo.spotCutOff = 30.0f;

    shininess = 64.0f;

    setViewingMatrix();
    setPerspective();    //set default projection matrix as perspective matrix
}

void setupRC()
{
    // setup shaders
    setShaders();
    initParameter();

    // OpenGL States and Values
    glClearColor(0.2, 0.2, 0.2, 1.0);
    vector<string> model_list{ "../NormalModels/bunny5KN.obj", "../NormalModels/dragon10KN.obj", "../NormalModels/lucy25KN.obj", "../NormalModels/teapot4KN.obj", "../NormalModels/dolphinN.obj"};
    // [TODO] Load five model at here
    //LoadModels(model_list[cur_idx]);
    for (const auto &model_path : model_list)
        LoadModels(model_path);
}

void glPrintContextInfo(bool printExtension)
{
    cout << "GL_VENDOR = " << (const char*)glGetString(GL_VENDOR) << endl;
    cout << "GL_RENDERER = " << (const char*)glGetString(GL_RENDERER) << endl;
    cout << "GL_VERSION = " << (const char*)glGetString(GL_VERSION) << endl;
    cout << "GL_SHADING_LANGUAGE_VERSION = " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
    if (printExtension)
    {
        GLint numExt;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
        cout << "GL_EXTENSIONS =" << endl;
        for (GLint i = 0; i < numExt; i++)
        {
            cout << "\t" << (const char*)glGetStringi(GL_EXTENSIONS, i) << endl;
        }
    }
}


int main(int argc, char **argv)
{
    // initial glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fix compilation on OS X
#endif

    
    // create window
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "108011173 HW2", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    
    // load OpenGL function pointer
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    // register glfw callback functions
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);

    glfwSetFramebufferSizeCallback(window, ChangeSize);
    glEnable(GL_DEPTH_TEST);
    // Setup render context
    setupRC();

    // main loop
    while (!glfwWindowShouldClose(window))
    {
        // render
        RenderScene();
        
        // swap buffer from back to front
        glfwSwapBuffers(window);
        
        // Poll input event
        glfwPollEvents();
    }
    
    // just for compatibiliy purposes
    return 0;
}
