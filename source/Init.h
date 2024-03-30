#pragma once
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <memory>
#include "PredefinedModels.h"


using namespace std;

namespace Init {
    float lastX = 400, lastY = 300;
    bool controlKeyPressed = false;
    bool leftButtonPressed = false;
    bool altButtonPressed = false;
    Camera* cameraPtr;
    Camera* planeCameraPtr;
    shared_ptr<Object> modelObjPtr;
    shared_ptr<LightCubeObject> lightCubeObjPtr;

    void initGL(const char *name, int argc, char** argv) {
        // Initialize GLUT
        glutInit(&argc, argv);

        // Set OpenGL version and profile
        glutInitContextVersion(3, 3);
        glutInitContextProfile(GLUT_CORE_PROFILE);

        // Set up a double-buffered window with RGBA color
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_ALPHA);

        // some default settings
        glutInitWindowSize(800, 600);
        glutInitWindowPosition(100, 100);


        // Create a window with a title
        glutCreateWindow(name);

        // Initialize GLEW
        glewInit();
        glEnable(GL_DEPTH_TEST);  
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glEnable(GL_BLEND);

    }

    void initLightCube(shared_ptr<LightCubeObject> &lightCubeObj, const char * vs, const char * fs) {
        // create lightcube object at (15,15,15) with color (1,1,1)
        lightCubeObj = make_shared<LightCubeObject>(&PredefinedModels::lightCubeVertices, 
                                                    cy::Vec3f(15.0, 15.0, 15.0), cy::Vec3f(1.0f,1.0f,1.0f), 
                                                    vs, fs);
        lightCubeObjPtr = lightCubeObj;

        // set up light cube
        glGenVertexArrays(1, &(lightCubeObjPtr->VAO)); 
        glBindVertexArray(lightCubeObjPtr->VAO);

        GLuint lightCubeVBO;
        glGenBuffers(1, &lightCubeVBO);
        glBindBuffer(GL_ARRAY_BUFFER, lightCubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * lightCubeObjPtr->vertices->size(), lightCubeObjPtr->vertices->data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    }

    shared_ptr<Object> initUntexturedModel(const char * objFile, const char * vs, const char * fs) {
        // create a model object with vs and fs shaders
        shared_ptr<Object> modelObj = make_shared<Object>(vs, fs);
        modelObj->loadModel(objFile);

        // set up VAO and VBO and EBO and NBO
        glGenVertexArrays(1, &(modelObj->VAO)); 
        glBindVertexArray(modelObj->VAO);

        GLuint VBO;
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cy::Vec3f)*modelObj->positions.size(), modelObj->positions.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        GLuint EBO;
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * modelObj->mesh.NF() * 3, &modelObj->mesh.F(0), GL_STATIC_DRAW);

        GLuint normalVBO;
        glGenBuffers(1, &normalVBO);
        glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cy::Vec3f) * modelObj->normals.size(), modelObj->normals.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(1); // Assuming attribute index 1 for normals
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

        return modelObj;
    }

    void keyboard(unsigned char key, int x, int y) {
        if (key == 27) {  // ASCII value for the Esc key
            glutLeaveMainLoop();
        } 
    }

    void handleMouse(int button, int state, int x, int y) {
        if (button == GLUT_LEFT_BUTTON) {
            leftButtonPressed = (state == GLUT_DOWN);
        }

        // Update last mouse position
        lastX = x;
        lastY = y;
    }

    void mouseMotion(int x, int y) {
        float xoffset = x - lastX;
        float yoffset = lastY - y; // reversed since y-coordinates range from bottom to top
        lastX = x;
        lastY = y;

        cameraPtr->processMouseMovement(xoffset, yoffset, leftButtonPressed);

        glutPostRedisplay();
    }

    void setCallbacks(void (*displayFunc)(void)) {
        // Set up callbacks
        glutDisplayFunc(displayFunc);
        glutKeyboardFunc(keyboard);
        glutMouseFunc(handleMouse);
        glutMotionFunc(mouseMotion);
    }

    void initCamera(Camera * camera) {
        cameraPtr = camera;
         // initialize camera
        cameraPtr->setOrthographicMatrix(0.1f, 1500.0f, 500.0f);
        cameraPtr->setPerspectiveMatrix(65,800.0f/600.0f, 2.0f, 600.0f);
    }

}