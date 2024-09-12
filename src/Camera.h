#ifndef CAMERA_H_
#define CAMERA_H_

#include <glm/ext/matrix_float4x4.hpp>

class Camera
{
public:
    Camera();
    Camera(float fovy, float aspect, float zNear, float zFar);
    ~Camera();

    glm::mat4 getTransform() const;
    glm::vec3 getEye() const;
    void lookAt(glm::vec3 center, float r, float theta, float phi);
    void drag(float dx, float dy);
    void setAspect(float aspect);
    void setRadius(float radius);
    void addRadius(float dr);
    void addFar(float dzFar);
    void setCenter(glm::vec3 center);
    void moveCenter(glm::vec3 dc);
private:
    void updateTransform();
    glm::mat4 perspective(float fovy, float aspect, float zNear, float zFar);
    void calcView(glm::vec3 center, float r, float theta, float phi);

    float fovy;
    float aspect;
    float zNear;
    float zFar;
    glm::vec3 center;
    glm::vec3 eye;
    float r;
    float theta;
    float phi;
    glm::mat4 currentTransform;
    glm::mat4 v;
    glm::mat4 p;
};

#endif
