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
int AOMode = 1;

void display() { 
    cy::Matrix4f view = camera.getLookAtMatrix();
    cy::Matrix4f proj = camera.getProjectionMatrix();
    // draw actual scene
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw light
    scene[0]->progs[0]->Bind();
    (*scene[0]->progs[0])["model"] = scene[0]->modelMatrix;
    (*scene[0]->progs[0])["view"] = view;
    (*scene[0]->progs[0])["projection"] = proj;
    (*scene[0]->progs[0])["cameraWorldSpacePos"] = camera.getPosition();
    glBindVertexArray(scene[0]->VAO);
    glDrawElements(GL_TRIANGLES, scene[0]->mesh.NF() * 3, GL_UNSIGNED_INT, 0);


    // draw all other models
    for (int i=1;i<scene.size();i++) {
        shared_ptr<Object> modelObj = scene[i];
        modelObj->progs[AOMode]->Bind();
        (*modelObj->progs[AOMode])["model"] = modelObj->modelMatrix;
        (*modelObj->progs[AOMode])["view"] = view;
        (*modelObj->progs[AOMode])["projection"] = proj;
        (*modelObj->progs[AOMode])["cameraWorldSpacePos"] = camera.getPosition();
        glBindVertexArray(modelObj->VAO);
        glDrawElements(GL_TRIANGLES, modelObj->mesh.NF() * 3, GL_UNSIGNED_INT, 0);
    }
    glutSwapBuffers();
}


int main(int argc, char** argv) {

    Init::initGL("Ambient Occlusion Techniques", argc, argv);
    Init::setCallbacks(display);

    shared_ptr<Object> areaLight = Init::initUntexturedModel("area_light.obj");
    scene.push_back(areaLight);

    shared_ptr<Object> backWall = Init::initUntexturedModel("back_wall.obj");
    scene.push_back(backWall);

    shared_ptr<Object> ceiling = Init::initUntexturedModel("ceiling.obj");
    scene.push_back(ceiling);

    shared_ptr<Object> floor = Init::initUntexturedModel("floor.obj");
    scene.push_back(floor);

    shared_ptr<Object> leftWall = Init::initUntexturedModel("left_wall.obj");
    scene.push_back(leftWall);

    shared_ptr<Object> rightWall = Init::initUntexturedModel("right_wall.obj");
    scene.push_back(rightWall);

    shared_ptr<Object> shortBox = Init::initUntexturedModel("short_box.obj");
    scene.push_back(shortBox);

    shared_ptr<Object> tallBox = Init::initUntexturedModel("tall_box.obj");
    scene.push_back(tallBox);
    
    // init cameras
    Init::initCamera(&camera);

    // initialize some uniforms
    cy::Vec3f lightPos = (areaLight->mesh.GetBoundMin() + areaLight->mesh.GetBoundMax()) * 0.5f;
    scene[0]->addProg("vs.txt", "light_fs.txt"); // shader program for the light itself
    
    for (int i=1;i<scene.size();i++) {
        shared_ptr<Object> modelObj = scene[i];

        // build no ambient light shader
        modelObj->addProg("vs.txt","no_ambient_fs.txt");
        (*modelObj->progs[0])["Ka"] = cy::Vec3f(modelObj->mesh.M(0).Ka[0],modelObj->mesh.M(0).Ka[1],modelObj->mesh.M(0).Ka[2]);
        (*modelObj->progs[0])["Kd"] = cy::Vec3f(modelObj->mesh.M(0).Kd[0], modelObj->mesh.M(0).Kd[1], modelObj->mesh.M(0).Kd[2]);
        (*modelObj->progs[0])["Ks"] = cy::Vec3f(modelObj->mesh.M(0).Ks[0],modelObj->mesh.M(0).Ks[1],modelObj->mesh.M(0).Ks[2]);
        (*modelObj->progs[0])["Ns"] = modelObj->mesh.M(0).Ns;
        (*modelObj->progs[0])["lightColor"] = cy::Vec3f(1.0f,1.0f,1.0f);
        (*modelObj->progs[0])["lightPos"] = lightPos;

        // build constant ambient light shader
        modelObj->addProg("vs.txt","constant_ambient_fs.txt");
        modelObj->progs[1]->Bind();
        (*modelObj->progs[1])["Ka"] = cy::Vec3f(modelObj->mesh.M(0).Ka[0],modelObj->mesh.M(0).Ka[1],modelObj->mesh.M(0).Ka[2]);
        (*modelObj->progs[1])["Kd"] = cy::Vec3f(modelObj->mesh.M(0).Kd[0], modelObj->mesh.M(0).Kd[1], modelObj->mesh.M(0).Kd[2]);
        (*modelObj->progs[1])["Ks"] = cy::Vec3f(modelObj->mesh.M(0).Ks[0],modelObj->mesh.M(0).Ks[1],modelObj->mesh.M(0).Ks[2]);
        (*modelObj->progs[1])["Ns"] = modelObj->mesh.M(0).Ns;
        (*modelObj->progs[1])["lightColor"] = cy::Vec3f(1.0f,1.0f,1.0f);
        (*modelObj->progs[1])["lightPos"] = lightPos;
    }



    glutMainLoop();

    return 0;
}