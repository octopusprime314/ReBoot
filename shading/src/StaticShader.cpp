#include "StaticShader.h"
#include "Model.h"

StaticShader::StaticShader(std::string shaderName) : Shader(shaderName) {

}

StaticShader::~StaticShader() {

}

void StaticShader::runShader(Model* model) {

    //LOAD IN SHADER
    glUseProgram(_shaderContext); //use context for loaded shader

    VAO* vao = model->getVAO();

    glBindVertexArray(vao->getVAOContext());

    MVP* mvp = model->getMVP();
    //glUniform mat4 combined model and world matrix, GL_TRUE is telling GL we are passing in the matrix as row major
    updateUniform("model", mvp->getModelBuffer());

    //glUniform mat4 view matrix, GL_TRUE is telling GL we are passing in the matrix as row major
    updateUniform("view", mvp->getViewBuffer());

    //glUniform mat4 projection matrix, GL_TRUE is telling GL we are passing in the matrix as row major
    updateUniform("projection", mvp->getProjectionBuffer());

    //glUniform mat4 normal matrix, GL_TRUE is telling GL we are passing in the matrix as row major
    updateUniform("normal", mvp->getNormalBuffer());

    auto textureStrides = model->getTextureStrides();
    unsigned int strideLocation = 0;
    for (auto textureStride : textureStrides) {

        //If the texture has layered encoded into the string then it is indeed layered
        if (textureStride.first.substr(0, 7) == "Layered") {

            LayeredTexture* layeredTexture = model->getLayeredTexture(textureStride.first);
            auto textures = layeredTexture->getTextures();

            //We have a layered texture
            int isLayered = 1;
            updateUniform("isLayeredTexture", &isLayered);

            if (textures.size() > 4) {
                updateUniform("tex0", GL_TEXTURE1, textures[0]->getContext(), GL_TEXTURE_2D);
                updateUniform("tex1", GL_TEXTURE2, textures[1]->getContext(), GL_TEXTURE_2D);
                updateUniform("tex2", GL_TEXTURE3, textures[2]->getContext(), GL_TEXTURE_2D);
                updateUniform("tex3", GL_TEXTURE4, textures[3]->getContext(), GL_TEXTURE_2D);
                updateUniform("alphatex0", GL_TEXTURE5, textures[7]->getContext(), GL_TEXTURE_2D);
            }
            else {
                updateUniform("tex0", GL_TEXTURE1, textures[0]->getContext(), GL_TEXTURE_2D);
                updateUniform("tex1", GL_TEXTURE2, textures[1]->getContext(), GL_TEXTURE_2D);
                updateUniform("tex2", GL_TEXTURE3, textures[2]->getContext(), GL_TEXTURE_2D);
                updateUniform("alphatex0", GL_TEXTURE5, textures[3]->getContext(), GL_TEXTURE_2D);
            }
            glDrawArrays(GL_TRIANGLES, strideLocation, (GLsizei)textureStride.second);
            strideLocation += textureStride.second;
        }
        else {
            //If triangle's textures supports transparency then do NOT draw
            //Transparent objects will be rendered after the deferred lighting pass
            if (!model->getTexture(textureStride.first)->getTransparency()) {

                //Not a layered texture
                int isLayered = 0;
                updateUniform("isLayeredTexture", &isLayered);

                updateUniform("textureMap", GL_TEXTURE0, model->getTexture(textureStride.first)->getContext(), GL_TEXTURE_2D);

                if (model->getTexture("../assets/textures/landscape/dirt.jpg") != nullptr) {
                    updateUniform("tex0", GL_TEXTURE0, model->getTexture("../assets/textures/landscape/dirt.jpg")->getContext(), GL_TEXTURE_2D);
                }
                if (model->getTexture("../assets/textures/landscape/grass.jpg") != nullptr) {
                    updateUniform("tex1", GL_TEXTURE1, model->getTexture("../assets/textures/landscape/grass.jpg")->getContext(), GL_TEXTURE_2D);
                }
                if (model->getTexture("../assets/textures/landscape/rocks.jpg") != nullptr) {
                    updateUniform("tex2", GL_TEXTURE2, model->getTexture("../assets/textures/landscape/rocks.jpg")->getContext(), GL_TEXTURE_2D);
                }
                if (model->getTexture("../assets/textures/landscape/snow.jpg") != nullptr) {
                    updateUniform("tex3", GL_TEXTURE3, model->getTexture("../assets/textures/landscape/snow.jpg")->getContext(), GL_TEXTURE_2D);
                }

                //Draw triangles using the bound buffer vertices at starting index 0 and number of triangles
                glDrawArrays(GL_TRIANGLES, strideLocation, (GLsizei)textureStride.second);
            }
            strideLocation += textureStride.second;
        }
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0); //Unbind texture
    glUseProgram(0);//end using this shader
}
