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
#include "SDL2/SDL.h"

using namespace std;

Camera camera;
vector<shared_ptr<Object>> scene;
shared_ptr<PlaneObject> planeObj;
GLuint gbuffer;
GLuint viewSpacePosTexture, gNormal;
unsigned int ssaoFBO, ssaoBlurFBO;
unsigned int ssaoColorBuffer, ssaoColorBufferBlur;

int AOMode = 0;
bool AO_ONLY_MODE = false;
float sample_sphere_radius = 0.5;

enum {
  FILTER_NUM  = 4,
  FILTER_SIZE = 32,
};

static GLuint F_tex[FILTER_NUM];


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
        glBindVertexArray(scene[0]->VAO);
        glDrawElements(GL_TRIANGLES, scene[0]->mesh.NF() * 3, GL_UNSIGNED_INT, 0);


        // draw all other models
        for (int i=1;i<scene.size();i++) {
            shared_ptr<Object> modelObj = scene[i];
            modelObj->progs[AOMode]->Bind();
            (*modelObj->progs[AOMode])["model"] = modelObj->modelMatrix;
            (*modelObj->progs[AOMode])["view"] = view;
            (*modelObj->progs[AOMode])["projection"] = proj;
            glBindVertexArray(modelObj->VAO);
            glDrawElements(GL_TRIANGLES, modelObj->mesh.NF() * 3, GL_UNSIGNED_INT, 0);
        }
    } else if (AOMode == 2) { //SSAO
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

        // 2. generate SSAO texture
        // ------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        planeObj->ssaoTexture.Bind();
        planeObj->ssaoTexture["projection"] = proj;
        planeObj->ssaoTexture["radius"] = sample_sphere_radius;
        glBindVertexArray(planeObj->VAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, viewSpacePosTexture);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (!AO_ONLY_MODE) {
            // 3. final render with AO applied
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
            // draw light
            scene[0]->progs[0]->Bind();
            (*scene[0]->progs[0])["model"] = scene[0]->modelMatrix;
            (*scene[0]->progs[0])["view"] = view;
            (*scene[0]->progs[0])["projection"] = proj;
            glBindVertexArray(scene[0]->VAO);
            glDrawElements(GL_TRIANGLES, scene[0]->mesh.NF() * 3, GL_UNSIGNED_INT, 0);


            // draw all other models
            for (int i=1;i<scene.size();i++) {
                shared_ptr<Object> modelObj = scene[i];
                modelObj->progs[3]->Bind();
                (*modelObj->progs[3])["model"] = modelObj->modelMatrix;
                (*modelObj->progs[3])["view"] = view;
                (*modelObj->progs[3])["projection"] = proj;
                glBindVertexArray(modelObj->VAO);
                glDrawElements(GL_TRIANGLES, modelObj->mesh.NF() * 3, GL_UNSIGNED_INT, 0);
            }
        } else {
            planeObj->debugScreen.Bind();
            glBindVertexArray(planeObj->VAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, viewSpacePosTexture);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
    } else if (AOMode == 3) { // SSAO+ 
        // 1. geometry pass: render scene's geometry/normal data into gbuffer
        // -----------------------------------------------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, gbuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        for (int i=1;i<scene.size();i++) {
            shared_ptr<Object> modelObj = scene[i];
            modelObj->progs[4]->Bind();
            (*modelObj->progs[4])["model"] = modelObj->modelMatrix;
            (*modelObj->progs[4])["view"] = view;
            (*modelObj->progs[4])["projection"] = proj;
            glBindVertexArray(modelObj->VAO);
            glDrawElements(GL_TRIANGLES, modelObj->mesh.NF() * 3, GL_UNSIGNED_INT, 0);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        // 2. generate SSAO+ texture
        // ------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        planeObj->ssaoPlusTexture.Bind();
        planeObj->ssaoPlusTexture["projection"] = proj;
        planeObj->ssaoPlusTexture["radius"] = sample_sphere_radius;
        glBindVertexArray(planeObj->VAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, viewSpacePosTexture);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

          // 3. final render with AO applied
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (!AO_ONLY_MODE) {
            // 3. final render with AO applied
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
            // draw light
            scene[0]->progs[0]->Bind();
            (*scene[0]->progs[0])["model"] = scene[0]->modelMatrix;
            (*scene[0]->progs[0])["view"] = view;
            (*scene[0]->progs[0])["projection"] = proj;
            glBindVertexArray(scene[0]->VAO);
            glDrawElements(GL_TRIANGLES, scene[0]->mesh.NF() * 3, GL_UNSIGNED_INT, 0);


            // draw all other models
            for (int i=1;i<scene.size();i++) {
                shared_ptr<Object> modelObj = scene[i];
                modelObj->progs[3]->Bind();
                (*modelObj->progs[3])["model"] = modelObj->modelMatrix;
                (*modelObj->progs[3])["view"] = view;
                (*modelObj->progs[3])["projection"] = proj;
                glBindVertexArray(modelObj->VAO);
                glDrawElements(GL_TRIANGLES, modelObj->mesh.NF() * 3, GL_UNSIGNED_INT, 0);
            }
        } else {
            planeObj->debugScreen.Bind();
            glBindVertexArray(planeObj->VAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, viewSpacePosTexture);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, gNormal);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

    } else if (AOMode == 4) { // NNAO
        // 1. geometry pass: render scene's geometry/normal data into gbuffer
        // -----------------------------------------------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, gbuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        for (int i=1;i<scene.size();i++) {
            shared_ptr<Object> modelObj = scene[i];
            modelObj->progs[5]->Bind();
            (*modelObj->progs[5])["model"] = modelObj->modelMatrix;
            (*modelObj->progs[5])["view"] = view;
            (*modelObj->progs[5])["projection"] = proj;
            glBindVertexArray(modelObj->VAO);
            glDrawElements(GL_TRIANGLES, modelObj->mesh.NF() * 3, GL_UNSIGNED_INT, 0);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 2. generate NNAO texture
        // ------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        planeObj->nnaoTexture.Bind();
        planeObj->nnaoTexture["cam_proj"] = proj;
        planeObj->nnaoTexture["radius"] = sample_sphere_radius;
        glBindVertexArray(planeObj->VAO);
        // set gbuffer
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, viewSpacePosTexture);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        // set filters
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, F_tex[0]);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, F_tex[1]);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, F_tex[2]);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, F_tex[3]);

        //draw to texture
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        // 3. blur ssao texture
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        planeObj->ssaoBlurTexture.Bind();
        glBindVertexArray(planeObj->VAO);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        // 4. final render with AO applied
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (!AO_ONLY_MODE) {
            // 3. final render with AO applied
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
            // draw light
            scene[0]->progs[0]->Bind();
            (*scene[0]->progs[0])["model"] = scene[0]->modelMatrix;
            (*scene[0]->progs[0])["view"] = view;
            (*scene[0]->progs[0])["projection"] = proj;
            glBindVertexArray(scene[0]->VAO);
            glDrawElements(GL_TRIANGLES, scene[0]->mesh.NF() * 3, GL_UNSIGNED_INT, 0);


            // draw all other models
            for (int i=1;i<scene.size();i++) {
                shared_ptr<Object> modelObj = scene[i];
                modelObj->progs[3]->Bind();
                (*modelObj->progs[3])["model"] = modelObj->modelMatrix;
                (*modelObj->progs[3])["view"] = view;
                (*modelObj->progs[3])["projection"] = proj;
                glBindVertexArray(modelObj->VAO);
                glDrawElements(GL_TRIANGLES, modelObj->mesh.NF() * 3, GL_UNSIGNED_INT, 0);
            }
        } else {
            // render AO only screen
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            planeObj->debugScreen.Bind();
            glBindVertexArray(planeObj->VAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, viewSpacePosTexture);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, gNormal);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

    }

    glutSwapBuffers();
}

static void load_tga(const char *filename, uint16_t *width, uint16_t *height, unsigned char **data) {
  
    SDL_RWops* file = SDL_RWFromFile(filename, "rb");
  
	if (file == NULL) {
		printf("Cannot open file %s", filename);
        exit(1);
	}
	
    char depth;
	SDL_RWseek(file, 12, SEEK_SET);
	SDL_RWread(file, width, sizeof(uint16_t), 1);
	SDL_RWseek(file, 14, SEEK_SET);
	SDL_RWread(file, height, sizeof(uint16_t), 1);
	SDL_RWseek(file, 16, SEEK_SET);
	SDL_RWread(file, &depth, sizeof(char), 1);
  
	if (depth != 32) {
		printf("tga file doesn't have four channels");
        exit(1);
    }
  
    *data = (unsigned char*)malloc(sizeof(unsigned char) * 4 * (*width) * (*height));
  
	SDL_RWseek(file, 18, SEEK_SET);
	SDL_RWread(file, *data, sizeof(unsigned char) * 4 * (*width) * (*height), 1);
    SDL_RWclose(file);
    
    for(int x = 0; x < *width; x++)
    for(int y = 0; y < *height; y++) {
        unsigned char t0, t1, t2, t3;
        t0 = (*data)[x * 4 + y * (*width) * 4 + 2]; t1 = (*data)[x * 4 + y * (*width) * 4 + 1];
        t2 = (*data)[x * 4 + y * (*width) * 4 + 0]; t3 = (*data)[x * 4 + y * (*width) * 4 + 3];
        (*data)[x * 4 + y * (*width) * 4 + 0] = t0; (*data)[x * 4 + y * (*width) * 4 + 1] = t1; 
        (*data)[x * 4 + y * (*width) * 4 + 2] = t2; (*data)[x * 4 + y * (*width) * 4 + 3] = t3;
    }
    
    for (int x = 0; x < *width;      x++)
    for (int y = 0; y < *height / 2; y++) {
        unsigned char t0, t1, t2, t3, b0, b1, b2, b3;
        t0 = (*data)[x * 4 + y * (*width) * 4 + 0]; t1 = (*data)[x * 4 + y * (*width) * 4 + 1];
        t2 = (*data)[x * 4 + y * (*width) * 4 + 2]; t3 = (*data)[x * 4 + y * (*width) * 4 + 3];
        b0 = (*data)[x * 4 + ((*height-1) - y) * (*width) * 4 + 0]; b1 = (*data)[x * 4 + ((*height-1) - y) * (*width) * 4 + 1];
        b2 = (*data)[x * 4 + ((*height-1) - y) * (*width) * 4 + 2]; b3 = (*data)[x * 4 + ((*height-1) - y) * (*width) * 4 + 3];
        (*data)[x * 4 + y * (*width) * 4 + 0] = b0; (*data)[x * 4 + y * (*width) * 4 + 1] = b1;
        (*data)[x * 4 + y * (*width) * 4 + 2] = b2; (*data)[x * 4 + y * (*width) * 4 + 3] = b3;
        (*data)[x * 4 + ((*height-1) - y) * (*width) * 4 + 0] = t0; (*data)[x * 4 + ((*height-1) - y) * (*width) * 4 + 1] = t1;
        (*data)[x * 4 + ((*height-1) - y) * (*width) * 4 + 2] = t2; (*data)[x * 4 + ((*height-1) - y) * (*width) * 4 + 3] = t3;
    }
  
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
        modelObj->addProg("ssao_geometry_vs.txt", "ssao_geometry_fs.txt");

        // ssao lighting shader
        modelObj->addProg("vs.txt", "ssao_lighting_fs.txt");
        (*modelObj->progs[3])["Ka"] = cy::Vec3f(modelObj->mesh.M(0).Ka[0],modelObj->mesh.M(0).Ka[1],modelObj->mesh.M(0).Ka[2]);
        (*modelObj->progs[3])["Kd"] = cy::Vec3f(modelObj->mesh.M(0).Kd[0], modelObj->mesh.M(0).Kd[1], modelObj->mesh.M(0).Kd[2]);
        (*modelObj->progs[3])["Ks"] = cy::Vec3f(modelObj->mesh.M(0).Ks[0],modelObj->mesh.M(0).Ks[1],modelObj->mesh.M(0).Ks[2]);
        (*modelObj->progs[3])["Ns"] = modelObj->mesh.M(0).Ns;
        (*modelObj->progs[3])["lightColor"] = cy::Vec3f(1.0f,1.0f,1.0f);
        (*modelObj->progs[3])["lightPos"] = lightPos;
        (*modelObj->progs[3])["viewSpacePos"] = 0;
        (*modelObj->progs[3])["ssaoTexture"] = 1;

        // ssao+ geometry shader
        modelObj->addProg("ssao+_geometry_vs.txt", "ssao+_geometry_fs.txt");

        // nnao geometry shader
        modelObj->addProg("nnao_geometry_vs.txt", "nnao_geometry_fs.txt");
    }

    // set up framebuffer for rendering geometric information
    glGenFramebuffers(1, &gbuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gbuffer);
    // view space position buffer
    glGenTextures(1, &viewSpacePosTexture);
    glBindTexture(GL_TEXTURE_2D, viewSpacePosTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 800, 600, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, viewSpacePosTexture, 0);
    // normal buffer
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 800, 600, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);

    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 800, 600);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // also create framebuffer to hold SSAO processing stage 
    // -----------------------------------------------------
    glGenFramebuffers(1, &ssaoFBO); 
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    // SSAO color buffer
    glGenTextures(1, &ssaoColorBuffer);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, 800, 600, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "SSAO Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //ssao blur framebuffer
    glGenFramebuffers(1, &ssaoBlurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glGenTextures(1, &ssaoColorBufferBlur);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, 800, 600, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // set up plane
    planeObj = make_shared<PlaneObject>(&PredefinedModels::quadVertices);
    planeObj->debugScreen.BuildFiles("debug_screen_vs.txt", "debug_screen_fs.txt");
    planeObj->ssaoTexture.BuildFiles("ssao_texture_vs.txt", "ssao_texture_fs.txt");
    planeObj->ssaoPlusTexture.BuildFiles("ssao+_texture_vs.txt", "ssao+_texture_fs.txt");
    planeObj->nnaoTexture.BuildFiles("nnao_texture_vs.txt", "nnao.fs");
    planeObj->ssaoBlurTexture.BuildFiles("ssao+_texture_vs.txt", "ssao_blur_texture_fs.txt");

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
    planeObj->ssaoTexture["viewSpacePos"] = 0;
    planeObj->ssaoPlusTexture["viewSpacePos"] = 0;
    planeObj->ssaoPlusTexture["gNormal"] = 2;
    planeObj->nnaoTexture["viewSpacePos"] = 0;
    planeObj->nnaoTexture["gNormal"] = 2;
    planeObj->nnaoTexture["F0"] = 3;
    planeObj->nnaoTexture["F1"] = 4;
    planeObj->nnaoTexture["F2"] = 5;
    planeObj->nnaoTexture["F3"] = 6;
    planeObj->debugScreen["viewSpacePos"] = 0;
    planeObj->debugScreen["ssaoTexture"] = 1;
    planeObj->debugScreen["gNormal"] = 2;
    planeObj->ssaoBlurTexture["ssaoTexture"] = 1;

    // generate samples in a unit sphere and send them to fragment shader
    Init::setSphereSamples(sample_sphere_radius);
    Init::setHemiSphereSamples(sample_sphere_radius);

     /* Load Filters */
  
    glGenTextures(FILTER_NUM, F_tex);

    for (int i = 0; i < FILTER_NUM; i++) {
        
        char filename[512];
        sprintf(filename, "./nnao_f%i.tga", i);

        uint16_t width, height;
        unsigned char *filter_data;
        sprintf(filename, "./nnao_f%i.tga", i);
        load_tga(filename, &width, &height, &filter_data);
        
        glBindTexture(GL_TEXTURE_2D, F_tex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FILTER_SIZE, FILTER_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, filter_data);
        
        free(filter_data);
    }

    glutMainLoop();

    return 0;
}


/* 

modelObj progs indexes:

0 -- no ambient light
1 -- constant ambient light
2 -- ssao geometry shader (for rendering geometric info into gbuffer)
3 -- ssao lighting shader (final render that applies the ssao texture)
4 -- ssao+ geometry shader
5 -- nnao geometry shader
*/


/* 

Program controls:

o -- No Ambient Occlusion [done]
c -- constant ambient lighting [done]
s -- screen space ao (ssao) [done]
h -- normal-based hemisphere AO (ssao+) [done]
n -- neural network ambient occlusion
t -- toggle AO only mode [done]

*/