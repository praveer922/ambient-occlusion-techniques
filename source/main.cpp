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
shared_ptr<PlaneObject> planeObj;
GLuint gbuffer;
GLuint viewSpacePosTexture;
int AOMode = 0;

void display() { 
    cy::Matrix4f view = camera.getLookAtMatrix();
    cy::Matrix4f proj = camera.getProjectionMatrix();

    if (AOMode <= 1) {
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
    } else if (AOMode > 1) {
        // 1. geometry pass: render scene's geometry/color data into gbuffer
        // -----------------------------------------------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, gbuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        for (int i=1;i<scene.size();i++) {
            shared_ptr<Object> modelObj = scene[i];
            modelObj->progs[2]->Bind();
            (*modelObj->progs[2])["model"] = modelObj->modelMatrix;
            (*modelObj->progs[2])["view"] = view;
            (*modelObj->progs[2])["projection"] = proj;
            glBindVertexArray(modelObj->VAO);
            glDrawElements(GL_TRIANGLES, modelObj->mesh.NF() * 3, GL_UNSIGNED_INT, 0);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        // render quad
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        planeObj->depthMapProg.Bind();
        glBindVertexArray(planeObj->VAO);
        glBindTexture(GL_TEXTURE_2D, viewSpacePosTexture);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);  
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
        (*modelObj->progs[1])["Ka"] = cy::Vec3f(modelObj->mesh.M(0).Ka[0],modelObj->mesh.M(0).Ka[1],modelObj->mesh.M(0).Ka[2]);
        (*modelObj->progs[1])["Kd"] = cy::Vec3f(modelObj->mesh.M(0).Kd[0], modelObj->mesh.M(0).Kd[1], modelObj->mesh.M(0).Kd[2]);
        (*modelObj->progs[1])["Ks"] = cy::Vec3f(modelObj->mesh.M(0).Ks[0],modelObj->mesh.M(0).Ks[1],modelObj->mesh.M(0).Ks[2]);
        (*modelObj->progs[1])["Ns"] = modelObj->mesh.M(0).Ns;
        (*modelObj->progs[1])["lightColor"] = cy::Vec3f(1.0f,1.0f,1.0f);
        (*modelObj->progs[1])["lightPos"] = lightPos;


        // ssao geometry shader
        modelObj->addProg("ssao_depthmap_vs.txt", "ssao_depthmap_fs.txt");
    }

    // set up framebuffer for rendering geometric information
    glGenFramebuffers(1, &gbuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gbuffer);
    glGenTextures(1, &viewSpacePosTexture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, viewSpacePosTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 800, 600, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, viewSpacePosTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // set up plane
    planeObj = make_shared<PlaneObject>(&PredefinedModels::quadVertices);
    planeObj->depthMapProg.BuildFiles("ssao_depthscreen_vs.txt", "ssao_depthscreen_fs.txt");
    planeObj->ssaoProg.BuildFiles("ssao_depthscreen_vs.txt", "ssao_fs.txt");

    glGenVertexArrays(1, &(planeObj->VAO)); 
    glBindVertexArray(planeObj->VAO);

    GLuint planeVBO;
    glGenBuffers(1, &planeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * planeObj->vertices->size(), planeObj->vertices->data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    planeObj->depthMapProg["viewSpacePos"] = 0;
    planeObj->ssaoProg["viewSpacePos"] = 0;


    glutMainLoop();

    return 0;
}


/* 

AOMode progs indexes:

0 -- no ambient light
1 -- constant ambient light
2 -- ssao+ geometry shader (for rendering geometric info into gbuffer)
3 -- ssao+ shader (uses Gbuffer to render SSAO texture)
4 -- ssao+ lighting pass (use ssao texture to do final render)

*/


/* 

Program controls:

o -- No Ambient Occlusion [done]
c -- constant ambient lighting [done]
s -- screen space ao (ssao)
h -- normal-based hemisphere AO (ssao+)
n -- neural network ambient occlusion

*/