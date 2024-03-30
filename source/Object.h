#pragma once
#include <string>
#include <iostream>
#include <GL/freeglut.h>
#include "includes/cyTriMesh.h"
#include "includes/cyMatrix.h"
#include "Util.h"
#include <iostream>


using namespace std;

class Object {
public:
    Object(const std::string& vsFile, const std::string& fsFile) 
        : modelMatrix(cy::Matrix4f(1.0)), vsFile(vsFile), fsFile(fsFile) {
            prog.BuildFiles(vsFile.c_str(),fsFile.c_str());
    }

    cy::Matrix4f modelMatrix; // Model matrix for the object
    string vsFile;    // Vertex shader file path
    string fsFile;    // Fragment shader file path
    cy::GLSLProgram prog;
    GLuint VAO;
    
    cy::TriMesh mesh;

    // store all of position, normal and texCoord vertices ordered according to the index ordering given by the mesh faces in one place
    std::vector<cy::Vec3f> positions;
    std::vector<cy::Vec3f> normals;
    std::vector<cy::Vec3f> texCoords;

    void loadModel(const char * filename) {
        bool success = mesh.LoadFromFileObj(filename);
        if (!success) {
            cout << "Model loading failed." << endl;
            exit(0);
        } else {
            cout << "Loaded model successfully." << endl;
            mesh.ComputeNormals();
            mesh.ComputeBoundingBox();
        }

        storeAllVertices();
    }

    void storeAllVertices() {
        positions = std::vector<cy::Vec3f>(mesh.NV());
        normals = std::vector<cy::Vec3f>(mesh.NV());
        for (int i=0;i<mesh.NF(); i++) {
            cy::TriMesh::TriFace f = mesh.F(i); //indices for positions
            cy::TriMesh::TriFace fn = mesh.FN(i); //indices for normals
            for (int j=0;j<3;j++) {
                int pos_idx = f.v[j];
                int norm_idx = fn.v[j];
                positions[pos_idx] = mesh.V(pos_idx);
                normals[pos_idx] = mesh.VN(norm_idx);
            }
        }
    }
};


class LightCubeObject : public Object {
public:
    LightCubeObject(std::vector<float>* vertices, cy::Vec3f worldSpacePos, cy::Vec3f lightColor, const std::string& vsFile, const std::string& fsFile) 
        : Object(vsFile, fsFile), vertices(vertices), worldSpacePos(worldSpacePos), lightColor(lightColor), rot_y(0.0f), rot_z(0.0f) {
            prog.BuildFiles(vsFile.c_str(),fsFile.c_str());
            // update the model matrix so that the vertices are translated to the given worldSpacePos
            modelMatrix = cy::Matrix4f::Translation(worldSpacePos);
    }
    
    std::vector<float>* vertices; // Pointer to a vector of vertices
    cy::Vec3f worldSpacePos;
    cy::Vec3f lightColor;
    float rot_y;
    float rot_z;

    void updateModelMatrix() {
        // update model matrix based on worldSpacePos()
        modelMatrix = cy::Matrix4f::Translation(worldSpacePos);
    }


    void processMouseMovement(float xoffset, float yoffset, bool leftButtonPressed) {
        if (leftButtonPressed) {
            rot_y -=xoffset * 0.01f;
            rot_z += yoffset * 0.01f;
        }

        // update worldSpacePos and model matrix with new rot values
        worldSpacePos = cy::Matrix3f::RotationY(Util::degreesToRadians(rot_y)) * cy::Matrix3f::RotationZ(Util::degreesToRadians(rot_z)) * worldSpacePos;
        updateModelMatrix();

    }
    
};

class PlaneObject : public Object {
    public:
    PlaneObject(std::vector<float>* vertices, cy::Vec3f worldSpacePos, const std::string& vsFile, const std::string& fsFile) 
        : Object(vsFile, fsFile), vertices(vertices), worldSpacePos(worldSpacePos) {
            prog.BuildFiles(vsFile.c_str(),fsFile.c_str());
            // update the model matrix so that the vertices are translated to the given worldSpacePos
            modelMatrix = cy::Matrix4f::Translation(worldSpacePos);
    }
    
    std::vector<float>* vertices; // Pointer to a vector of vertices
    cy::Vec3f worldSpacePos;
    cy::GLSLProgram geometryProg;

    void updateModelMatrix() {
        // update model matrix based on worldSpacePos()
        modelMatrix = cy::Matrix4f::Translation(worldSpacePos);
    }
};