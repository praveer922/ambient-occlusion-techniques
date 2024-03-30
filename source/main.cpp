#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include "includes/cyTriMesh.h"
#include "includes/cyMatrix.h"
#include "includes/cyGL.h"
#include "includes/lodepng.h"
#include "Camera.h"
#include "Object.h"
#include "Init.h"
#include <memory>
#include <iostream>

using namespace std;

Camera camera;

vector<shared_ptr<Object>> scene;
shared_ptr<LightCubeObject> lightCubeObj;
int AOMode = 0;

void display() { 
    cy::Matrix4f view = camera.getLookAtMatrix();
    cy::Matrix4f proj = camera.getProjectionMatrix();
    // draw actual scene
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (shared_ptr<Object> modelObj : scene) {
        modelObj->prog.Bind();
        modelObj->prog["model"] = modelObj->modelMatrix;
        modelObj->prog["view"] = view;
        modelObj->prog["projection"] = proj;
        modelObj->prog["cameraWorldSpacePos"] = camera.getPosition();
        modelObj->prog["AOMode"] = AOMode;
        glBindVertexArray(modelObj->VAO);
        glDrawElements(GL_TRIANGLES, modelObj->mesh.NF() * 3, GL_UNSIGNED_INT, 0);
    }
    glutSwapBuffers();
}


int main(int argc, char** argv) {

    Init::initGL("Ambient Occlusion Techniques", argc, argv);
    Init::setCallbacks(display);

    shared_ptr<Object> backWall = Init::initUntexturedModel("back_wall.obj", "vs.txt", "no_ambient_fs.txt");
    scene.push_back(backWall);

    shared_ptr<Object> areaLight = Init::initUntexturedModel("area_light.obj", "vs.txt", "light_fs.txt");
    scene.push_back(areaLight);

    cy::Vec3f lightPos = (areaLight->mesh.GetBoundMin() + areaLight->mesh.GetBoundMax()) * 0.5f;

    shared_ptr<Object> ceiling = Init::initUntexturedModel("ceiling.obj", "vs.txt", "no_ambient_fs.txt");
    scene.push_back(ceiling);

    shared_ptr<Object> floor = Init::initUntexturedModel("floor.obj", "vs.txt", "no_ambient_fs.txt");
    scene.push_back(floor);

    shared_ptr<Object> leftWall = Init::initUntexturedModel("left_wall.obj", "vs.txt", "no_ambient_fs.txt");
    scene.push_back(leftWall);

    shared_ptr<Object> rightWall = Init::initUntexturedModel("right_wall.obj", "vs.txt", "no_ambient_fs.txt");
    scene.push_back(rightWall);

    shared_ptr<Object> shortBox = Init::initUntexturedModel("short_box.obj", "vs.txt", "no_ambient_fs.txt");
    scene.push_back(shortBox);

    shared_ptr<Object> tallBox = Init::initUntexturedModel("tall_box.obj", "vs.txt", "no_ambient_fs.txt");
    scene.push_back(tallBox);
    
    // init cameras
    Init::initCamera(&camera);

    // initialize some uniforms
    for (shared_ptr<Object> modelObj : scene) {
        modelObj->prog["ambientStr"] = 0.1f;
        modelObj->prog["Ka"] = cy::Vec3f(modelObj->mesh.M(0).Ka[0],modelObj->mesh.M(0).Ka[1],modelObj->mesh.M(0).Ka[2]);
        modelObj->prog["Kd"] = cy::Vec3f(modelObj->mesh.M(0).Kd[0], modelObj->mesh.M(0).Kd[1], modelObj->mesh.M(0).Kd[2]);
        modelObj->prog["Ks"] = cy::Vec3f(modelObj->mesh.M(0).Ks[0],modelObj->mesh.M(0).Ks[1],modelObj->mesh.M(0).Ks[2]);
        modelObj->prog["Ns"] = modelObj->mesh.M(0).Ns;
        modelObj->prog["lightColor"] = cy::Vec3f(1.0f,1.0f,1.0f);
        modelObj->prog["lightPos"] = lightPos;
    }

    glutMainLoop();

    return 0;
}