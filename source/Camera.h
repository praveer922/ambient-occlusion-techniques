#pragma once
#include <GL/freeglut.h>
#include <cmath>
#include "includes/cyMatrix.h"
#include "Util.h"
#include <iostream>

using namespace std;

class Camera {
public:
    Camera(cy::Vec3f worldSpacePosition = {-0.5f,3.0f,5.0f}) : position(worldSpacePosition), front(0.0f, 0.0f, -1.0f), up(0.0f, 1.0f, 0.0f), 
    yaw(-90.0f), pitch(0.0f), movementSpeed(2.5f), mouseSensitivity(0.1f), orthogonal_projection_on(false),
    perspectiveMatrix(cy::Matrix4f(1.0)), orthographicMatrix(cy::Matrix4f(1.0)){
        update();
    }

    void update() {
        front.x = cos(Util::degreesToRadians(yaw)) * cos(Util::degreesToRadians(pitch));
        front.y = sin(Util::degreesToRadians(pitch));
        front.z = sin(Util::degreesToRadians(yaw)) * cos(Util::degreesToRadians(pitch));
        front.Normalize();

        right = front.Cross(up);
        right.Normalize();
        upVector = right.Cross(front);
        upVector.Normalize();
    }

    void processMouseMovement(float xoffset, float yoffset, bool leftButtonPressed) {
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        if(leftButtonPressed) {
            yaw -= xoffset;
            pitch -= yoffset;
        } else {
            position -= front * yoffset;
        }

        // Prevent flipping when looking too far up or down
        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;

        update();
    }

    cy::Vec3f getPosition() const { return position; }
    cy::Vec3f getFront() const { return front; }
    cy::Matrix4f getLookAtMatrix() const { return cy::Matrix4f::View(position, position+front, upVector); }

    cy::Matrix4f& getProjectionMatrix() {
        if (orthogonal_projection_on) {
            return orthographicMatrix;
        } else {
            return perspectiveMatrix;
        }
    }

    void setPosition(cy::Vec3f &newPos) {
        position = newPos;
    }

    void setFrontDirection(cy::Vec3f frontDir) {
        front = frontDir;
    }

    void setPerspectiveMatrix(float fov_degrees, float aspect, float znear, float zfar) {
        perspectiveMatrix = cy::Matrix4f::Perspective(Util::degreesToRadians(fov_degrees), aspect, znear, zfar);

    }

    void setOrthographicMatrix(float near, float far, float scale) {
        float scale_factor = scale/(position.z);
        orthographicMatrix = Util::getOrthographicMatrix(-scale_factor, scale_factor, -scale_factor, scale_factor, near, far);

    }

private:
    // camera position in world space
    cy::Vec3f position;
    cy::Vec3f front;
    cy::Vec3f up; // world space up direction
    cy::Vec3f right;
    cy::Vec3f upVector; // camera's up vector (in world space)

    cy::Matrix4f perspectiveMatrix;
    cy::Matrix4f orthographicMatrix;

    float yaw;
    float pitch;

    float movementSpeed;
    float mouseSensitivity;

    bool orthogonal_projection_on;
};