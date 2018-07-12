#include "ModelBroker.h"
#include "Dirent.h"
#include <algorithm>
#include <cctype>
#include "AnimatedModel.h"
#include "Model.h"
//#include "Logger.h"
ModelBroker* ModelBroker::_broker = nullptr;

ModelBroker* ModelBroker::instance() { //Only initializes the static pointer once
    if (_broker == nullptr) {
        _broker = new ModelBroker();
    }
    return _broker;
}
ModelBroker::ModelBroker() {

}
ModelBroker::~ModelBroker() {

}

//helper function to capitalize everything
std::string ModelBroker::_strToUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return std::toupper(c); } // correct
    );
    return s;
}

void ModelBroker::buildModels() {

    _gatherModelNames();
}

Model* ModelBroker::getModel(std::string modelName) {
    std::string upperCaseMapName = _strToUpper(modelName);
    return _models[upperCaseMapName];
}

void ModelBroker::updateModel(std::string modelName) {

    std::string upperCaseMapName = _strToUpper(modelName + "/" + modelName + ".fbx");

    if (_models.find(upperCaseMapName) != _models.end()) {

        if (_models[upperCaseMapName]->getClassType() == ModelClass::ModelType) {
            auto model = new Model(upperCaseMapName);
            //TODO: Release previous model data
            _models[upperCaseMapName]->updateModel(model);
            delete model;
        }
        else if (_models[upperCaseMapName]->getClassType() == ModelClass::AnimatedModelType) {
            auto model = new AnimatedModel(upperCaseMapName);
            //TODO: Release previous model data
            _models[upperCaseMapName]->updateModel(model);
            //delete model;
        }

    }
    else {
        std::cout << "Model doesn't exist so add it!" << std::endl;
    }
}


void ModelBroker::_gatherModelNames()
{
    bool isFile = false;
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(STATIC_MESH_LOCATION.c_str())) != nullptr)
    {
        //Logger::WriteLog("Files to be processed: \n");

        while ((ent = readdir(dir)) != nullptr) {
            if (*ent->d_name) {
                std::string fileName = std::string(ent->d_name);
           
                if (!fileName.empty() && 
                    fileName != "." && 
                    fileName != ".." && 
                    fileName.find(".ini") == std::string::npos) {
                    
                    //Logger::WriteLog(ent->d_name, "\n");

                    std::string mapName = fileName + "/" + fileName + ".fbx";
                    std::string upperCaseMapName = _strToUpper(mapName);
                    _models[upperCaseMapName] = new Model(mapName);
                }
            }
        }
        closedir(dir);
    }
    else
    {
        std::cout << "Problem reading from directory!" << std::endl;
    }

    if ((dir = opendir(ANIMATED_MESH_LOCATION.c_str())) != nullptr)
    {
        //Logger::WriteLog("Files to be processed: \n");

        while ((ent = readdir(dir)) != nullptr) {
            if (*ent->d_name) {
                std::string fileName = std::string(ent->d_name);

                if (!fileName.empty() &&
                    fileName != "." &&
                    fileName != ".." &&
                    fileName.find(".ini") == std::string::npos) {

                    //Logger::WriteLog(ent->d_name, "\n");

                    std::string mapName = fileName + "/" + fileName + ".fbx";
                    std::string upperCaseMapName = _strToUpper(mapName);
                    _models[upperCaseMapName] = static_cast<Model*>(new AnimatedModel(mapName));
                }
            }
        }
        closedir(dir);
    }
    else
    {
        std::cout << "Problem reading from directory!" << std::endl;
    }
}
