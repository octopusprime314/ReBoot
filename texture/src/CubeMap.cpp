#include "CubeMap.h"
#include "GLIncludes.h"

CubeMap::CubeMap(unsigned int width, unsigned int height, bool isDepth) :
    Texture("cube", width, height) {

    //Generate texture context
    glGenTextures(1, &_cubemap);

    //Bind the texture and create 6 sides of a texture cube
    glBindTexture(GL_TEXTURE_CUBE_MAP, _cubemap);
    for (unsigned int i = 0; i < 6; ++i) {

        //A depth or color buffer can be rendered to for a generic cube map
        if (isDepth) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, _width, _height,
                0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        }
        else {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, _width, _height,
                0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        }
    }

    //Texture params
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    //Generate frame buffer context and bind it
    glGenFramebuffers(1, &_cubeFrameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, _cubeFrameBuffer);

    //Attach the framebuffer object to the cubemap texture
    if (isDepth) {
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _cubemap, 0);
        //Disables color buffer rendering for the depth only passage
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }
    else {
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _cubemap, 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

CubeMap::~CubeMap() {

}

unsigned int CubeMap::getCubeFrameBufferContext() {
    return _cubeFrameBuffer;
}

unsigned int CubeMap::getCubeMapContext() {
    return _cubemap;
}
