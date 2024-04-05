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
    Object() : modelMatrix(cy::Matrix4f(1.0)) {
    }

    cy::Matrix4f modelMatrix; // Model matrix for the object
    std::vector<cy::GLSLProgram *> progs; 
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

    void addProg(const std::string& vsFile, const std::string& fsFile) {
        cy::GLSLProgram* newProg = new cy::GLSLProgram();
        newProg->BuildFiles(vsFile.c_str(),fsFile.c_str());
        progs.push_back(newProg);
    }
};

class PlaneObject : public Object {
    public:
    PlaneObject(std::vector<float>* vertices) 
        : vertices(vertices) {
    }
    
    std::vector<float>* vertices; // Pointer to a vector of vertices
    cy::GLSLProgram debugScreen; 
    cy::GLSLProgram ssaoTexture; 
    cy::GLSLProgram ssaoLighting;
};