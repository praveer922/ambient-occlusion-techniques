#pragma once
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <memory>
#include "PredefinedModels.h"


using namespace std;

extern bool isSpaceKeyPressed;
extern float tessLevel;

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

    void initTeapotModel(int argc, char** argv, shared_ptr<Object> &modelObj, const char * vs, const char * fs) {
        char * modelName;

        // load model
        if (argc<1) {
            cout << "No model given. Specify model name in cmd-line arg." << endl;
            exit(0);
        } else {
            modelName = argv[1];
            cout << "Loading " << modelName << "..." << endl;
        }

        // create a model object with vs and fs shaders
        modelObj = make_shared<Object>(vs, fs);
        modelObj->loadModel(modelName);
        modelObjPtr = modelObj;

        // the teapot model is top down so let's rotate it 90 degrees
        modelObjPtr->modelMatrix = cy::Matrix4f::RotationX(Util::degreesToRadians(-90)) * modelObjPtr->modelMatrix;

        // set up VAO and VBO and EBO and NBO
        glGenVertexArrays(1, &(modelObjPtr->VAO)); 
        glBindVertexArray(modelObjPtr->VAO);

        GLuint VBO;
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cy::Vec3f)*modelObjPtr->positions.size(), modelObjPtr->positions.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        GLuint EBO;
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * modelObjPtr->mesh.NF() * 3, &modelObjPtr->mesh.F(0), GL_STATIC_DRAW);

        GLuint normalVBO;
        glGenBuffers(1, &normalVBO);
        glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cy::Vec3f) * modelObjPtr->normals.size(), modelObjPtr->normals.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(1); // Assuming attribute index 1 for normals
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

        // texture stuff 
        GLuint texCoordVBO;
        glGenBuffers(1, &texCoordVBO);
        glBindBuffer(GL_ARRAY_BUFFER, texCoordVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cy::Vec3f) * modelObjPtr->texCoords.size(), modelObjPtr->texCoords.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(2); // Assuming attribute index 2 for texture coords
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(cy::Vec3f), (void*)0);

        // teapot texture
        GLuint teapotTexture;
        glGenTextures(1, &teapotTexture);  
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, teapotTexture);  
        modelObjPtr->prog["ambientMap"] = 0;
        modelObjPtr->prog["diffuseMap"] = 0; 
        modelObjPtr->prog["materialAmbient"] = cy::Vec3f(modelObjPtr->mesh.M(0).Ka[0], modelObjPtr->mesh.M(0).Ka[1], modelObjPtr->mesh.M(0).Ka[2]);
        modelObjPtr->prog["materialDiffuse"] = cy::Vec3f(modelObjPtr->mesh.M(0).Kd[0], modelObjPtr->mesh.M(0).Kd[1], modelObjPtr->mesh.M(0).Kd[2]);
        modelObjPtr->prog["materialSpecular"] = cy::Vec3f(modelObjPtr->mesh.M(0).Ks[0], modelObjPtr->mesh.M(0).Ks[1], modelObjPtr->mesh.M(0).Ks[2]);
        modelObjPtr->prog["materialShininess"] = modelObjPtr->mesh.M(0).Ns;

        std::vector<unsigned char> image; // The raw pixels
        unsigned width, height;

        // Decode the image
        unsigned error = lodepng::decode(image, width, height, modelObjPtr->mesh.M(0).map_Kd.data);
        if (error) {
            std::cerr << "Error loading texture: " << lodepng_error_text(error) << std::endl;
            return;
        } else {
            std::cout << "Diffuse texture map loaded successfully." << std::endl;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image[0]);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


        // load specular texture
        GLuint specularTexture;
        glGenTextures(1, &specularTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularTexture);
        modelObjPtr->prog["specularMap"] = 1; // set to texture 1

        std::vector<unsigned char> specular_image; // The raw pixels
        unsigned spec_w, spec_h;

        // Decode the image
        unsigned error_spec = lodepng::decode(specular_image, spec_w, spec_h, modelObjPtr->mesh.M(0).map_Ks.data);
        if (error) {
            std::cerr << "Error loading texture: " << lodepng_error_text(error_spec) << std::endl;
            return;
        } else {
            std::cout << "Specular texture map loaded successfully." << std::endl;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, spec_w, spec_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, &specular_image[0]);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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

    void specialKeyboard(int key, int x, int y) {
        switch (key) {
            case GLUT_KEY_F6:
                // Reload shaders when F6 key is pressed
                modelObjPtr->prog.BuildFiles("vs.txt", "fs.txt");
                modelObjPtr->prog.Bind();
                cout << "Shaders recompiled successfully." << endl;
                glutPostRedisplay();
                break;
            case GLUT_KEY_CTRL_L:
            case GLUT_KEY_CTRL_R:
                controlKeyPressed = true;
                break;
            case GLUT_KEY_ALT_L:
            case GLUT_KEY_ALT_R:
                altButtonPressed = true;
                break;
            case GLUT_KEY_LEFT:
                tessLevel -= 2.0f;
                glutPostRedisplay();
                break;
            case GLUT_KEY_RIGHT:
                tessLevel += 2.0f;
                glutPostRedisplay();
                break;
        }
    }

    void keyboard(unsigned char key, int x, int y) {
        if (key == 27) {  // ASCII value for the Esc key
            glutLeaveMainLoop();
        } else if (key == 32) {// spacebar
            isSpaceKeyPressed = !isSpaceKeyPressed;
        } else {
            cameraPtr->processKeyboard(key, x, y);
        }
        glutPostRedisplay();
    }


    void specialKeyboardUp(int key, int x, int y) {
        switch (key) {
            case GLUT_KEY_CTRL_L:
            case GLUT_KEY_CTRL_R:
                controlKeyPressed = false;
                break;
            case GLUT_KEY_ALT_L:
            case GLUT_KEY_ALT_R:
                altButtonPressed = false;
                break;
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
        glutSpecialUpFunc(specialKeyboardUp);
        glutMouseFunc(handleMouse);
        glutMotionFunc(mouseMotion);
        glutSpecialFunc(specialKeyboard);
    }

    void initCamera(Camera * camera) {
        cameraPtr = camera;
         // initialize camera
        cameraPtr->setOrthographicMatrix(0.1f, 1500.0f, 500.0f);
        cameraPtr->setPerspectiveMatrix(65,800.0f/600.0f, 2.0f, 600.0f);
    }

    void initPlaneCamera(Camera * planeCamera) {
        planeCameraPtr = planeCamera;
         // initialize camera
        planeCameraPtr->setOrthographicMatrix(0.1f, 1500.0f, 500.0f);
        planeCameraPtr->setPerspectiveMatrix(65,800.0f/600.0f, 2.0f, 1000.0f);
    }

    void initCubeMap(const vector<std::string> &faces) {
        GLuint cubeMapTextureId;
        glGenTextures(1, &cubeMapTextureId);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTextureId);

        std::vector<unsigned char> image; // The raw pixels
        unsigned width, height;

        for (int i=0;i<faces.size();i++) {
            // Decode the image
            unsigned error = lodepng::decode(image, width, height, faces[i]);
            if (error) {
                std::cerr << "Error loading texture: " << lodepng_error_text(error) << std::endl;
                return;
            } else {
                std::cout << "Diffuse texture map loaded successfully." << std::endl;
            }

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image[0]);
            image.clear();
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);


    }


}