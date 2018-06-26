#include "ShadowAnimatedShader.h"

ShadowAnimatedShader::ShadowAnimatedShader(std::string shaderName) : ShadowStaticShader(shaderName) {

}

ShadowAnimatedShader::~ShadowAnimatedShader() {

}

void ShadowAnimatedShader::runShader(Model* model, Light* light) {

    AnimatedModel* animationModel = static_cast<AnimatedModel*>(model);

    //Load in vbo buffers
    VAO* vao = model->getVAO();
    MVP* modelMVP = animationModel->getMVP();
    MVP lightMVP = light->getLightMVP();

    //Use one single shadow shader and replace the vbo buffer from each model
    glUseProgram(_shaderContext); //use context for loaded shader

    glBindVertexArray(vao->getVAOShadowContext());

    //glUniform mat4 combined model and world matrix, GL_TRUE is telling GL we are passing in the matrix as row major
    updateUniform("model", modelMVP->getModelBuffer());

    //glUniform mat4 view matrix, GL_TRUE is telling GL we are passing in the matrix as row major
    updateUniform("view", lightMVP.getViewBuffer());

    //glUniform mat4 projection matrix, GL_TRUE is telling GL we are passing in the matrix as row major
    updateUniform("projection", lightMVP.getProjectionBuffer());

    //Bone uniforms
    auto bones = animationModel->getBones();
    float* bonesArray = new float[16 * bones->size()]; //4x4 times number of bones
    int bonesArrayIndex = 0;
    for (auto bone : *bones) {
        for (int i = 0; i < 16; i++) {
            float* buff = bone.getFlatBuffer();
            bonesArray[bonesArrayIndex++] = buff[i];
        }
    }
    updateUniform("bones[0]", bonesArray);
    delete[] bonesArray;

    auto textureStrides = model->getTextureStrides();
    unsigned int verticesSize = 0;
    for (auto textureStride : textureStrides) {
        verticesSize += textureStride.second;
    }

    //Draw triangles using the bound buffer vertices at starting index 0 and number of vertices
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)verticesSize);

    glBindVertexArray(0);
    glUseProgram(0);//end using this shader
}
