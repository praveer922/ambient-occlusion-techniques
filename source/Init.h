#pragma once
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <memory>
#include "PredefinedModels.h"


using namespace std;

extern int AOMode;

namespace Init {
    float lastX = 400, lastY = 300;
    bool leftButtonPressed = false;
    Camera* cameraPtr;
    shared_ptr<Object> modelObjPtr;

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

    shared_ptr<Object> initUntexturedModel(const char * objFile) {
        // create a model object with vs and fs shaders
        shared_ptr<Object> modelObj = make_shared<Object>();
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
        } else if (key == 'o' || key == 'O') { 
            AOMode = 0;
            cout << "Switched to no ambient light mode." << endl;
        } else if (key == 'c' || key == 'C') { 
            AOMode = 1;
            cout << "Switched to constant ambient light." << endl;
        }
        glutPostRedisplay();
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