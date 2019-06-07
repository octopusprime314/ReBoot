#include "FbxLoader.h"
#include "AnimatedModel.h"
#include "Model.h"
#include <map>
#include "Texture.h"
#include "Triangle.h"
#include <algorithm>
#include <limits>
#include "RenderBuffers.h"
#include "ModelBroker.h"
#include "MutableTexture.h"
#include "EngineManager.h"
#include <cctype>
using namespace fbxsdk;

FbxLoader::FbxLoader(std::string name) : 
    _fileName(name),
    _strideIndex(0),
    _copiedOverFlag(false) {
    _fbxManager = FbxManager::Create();
    
    if (_fbxManager == nullptr) {
        printf("ERROR %s : %d failed creating FBX Manager!\n", __FILE__, __LINE__);
    }

    _ioSettings = FbxIOSettings::Create(_fbxManager, IOSROOT);
    _fbxManager->SetIOSettings(_ioSettings);

    FbxString filePath = FbxGetApplicationDirectory();
    _fbxManager->LoadPluginsDirectory(filePath.Buffer());

    _scene = FbxScene::Create(_fbxManager, "");

    int fileMinor, fileRevision;
    int sdkMajor, sdkMinor, sdkRevision;
    int fileFormat;

    FbxManager::GetFileFormatVersion(sdkMajor, sdkMinor, sdkRevision);
    FbxImporter* importer = FbxImporter::Create(_fbxManager, "");

    if (!_fbxManager->GetIOPluginRegistry()->DetectReaderFileFormat(name.c_str(), fileFormat)) {
        //Unrecognizable file format. Try to fall back on FbxImorter::eFBX_BINARY
        fileFormat = _fbxManager->GetIOPluginRegistry()->FindReaderIDByDescription("FBX binary (*.fbx)");
    }

    bool importStatus = importer->Initialize(name.c_str(), fileFormat, _fbxManager->GetIOSettings());
    importer->GetFileVersion(fileMinor, fileMinor, fileRevision);

    if (!importStatus) {
        printf("ERROR %s : %d FbxImporter Initialize failed!\n", __FILE__, __LINE__);
    }

    importStatus = importer->Import(_scene);
    if (!importStatus) {
        const char* fbxErrorString = importer->GetStatus().GetErrorString();
        printf("ERROR %s : %d FbxImporter failed to import '%s' to the scene: %s\n",
            __FILE__, __LINE__, name.c_str(), fbxErrorString);
    }

    importer->Destroy();

    _parseTags(_scene->GetRootNode());
   
    auto modelBroker = ModelBroker::instance();
    auto tileTextureInstances = _tileTextures.begin();
    for (auto transform : _clonedWorldTransforms) {
        if (transform.first.size() != 0) {
            
            std::string modelName = transform.first.substr(0, transform.first.find_first_of("_"));
            Model* modelToInstance = modelBroker->getModel(modelName);
            if (modelToInstance != nullptr) {
                auto entity = EngineManager::addEntity(modelToInstance, transform.second, false);
                if (modelToInstance->getName().find("tile") != std::string::npos) {
                    auto textureBroker = TextureBroker::instance();
                    std::string layeredTextureName = "Layered";
                    for (auto& texture : tileTextureInstances->second) {
                        layeredTextureName += texture;
                    }
                    auto layeredTexture = textureBroker->getLayeredTexture(layeredTextureName);
                    entity->setLayeredTexture(layeredTexture);
                    tileTextureInstances++;
                }
            }
            else {
                std::cout << "Model name doesn't exist for instancing: " << transform.first << std::endl;
            }
        }
    }

    _export.manager = FbxManager::Create();
    _export.ioSettings = FbxIOSettings::Create(_export.manager, IOSROOT);
    _export.ioSettings->SetBoolProp(EXP_FBX_MATERIAL, true);
    _export.ioSettings->SetBoolProp(EXP_FBX_TEXTURE, true);
    // create an empty scene to be exported
    _export.scene = FbxScene::Create(_export.manager, "");
    // create an exporter.
    _export.exporter = FbxExporter::Create(_export.manager, "");
    int asciiFormatIndex = _getASCIIFormatIndex(_export.manager);
    // initialize the exporter by providing a filename and the IOSettings to use
    //exports ascii fbx
    _export.exporter->Initialize((_fileName + "test").c_str(), asciiFormatIndex, _export.ioSettings);
    //exports binary fbx i.e. much smaller
    //_export.exporter->Initialize((_fileName + "test").c_str(), -1, _export.ioSettings);
}

FbxLoader::~FbxLoader() {
    _scene->Destroy();
    _ioSettings->Destroy();
    _fbxManager->Destroy();

    // destroy the exporter
    _export.exporter->Destroy();
    _export.scene->Destroy();
    _export.ioSettings->Destroy();
    _export.manager->Destroy();
}

bool isNumber(const std::string& s) {
    return !s.empty() && std::find_if(s.begin(),
        s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}

FbxScene* FbxLoader::getScene() {
    return _scene;
}

void FbxLoader::loadModel(Model* model, FbxNode* node) {

    // Determine the number of children
    int numChildren = node->GetChildCount();
    FbxNode* childNode = nullptr;
    for (int i = 0; i < numChildren; i++) {
        childNode = node->GetChild(i);

        FbxMesh* mesh = childNode->GetMesh();
        if (mesh != nullptr) {
            loadModelData(model, mesh, childNode);
        }
        loadModel(model, childNode);
    }
}

void FbxLoader::_parseTags(FbxNode* node) {

    static std::map<std::string, Matrix> transformations;
    // Determine the number of children there are
    int numChildren = node->GetChildCount();
    FbxNode* childNode = nullptr;
    for (int i = 0; i < numChildren; i++) {
        childNode = node->GetChild(i);
        FbxDouble* TBuff = childNode->LclTranslation.Get().Buffer();
        FbxDouble* RBuff = childNode->LclRotation.Get().Buffer();
        FbxDouble* SBuff = childNode->LclScaling.Get().Buffer();

        //Compute x, y and z rotation vectors by multiplying through
        Matrix rotation = Matrix::rotationAroundX(static_cast<float>(RBuff[0])) *
            Matrix::rotationAroundY(static_cast<float>(RBuff[1])) *
            Matrix::rotationAroundZ(static_cast<float>(RBuff[2]));
        Matrix translation = Matrix::translation(static_cast<float>(TBuff[0]), static_cast<float>(TBuff[1]), static_cast<float>(TBuff[2]));
        Matrix scale = Matrix::scale(static_cast<float>(SBuff[0]), static_cast<float>(SBuff[1]), static_cast<float>(SBuff[2]));

        FbxMesh* mesh = childNode->GetMesh();
        if (mesh != nullptr) {
            std::string nodeName = std::string(childNode->GetName());
            if (nodeName.find("_") != std::string::npos) {
                auto index = nodeName.find_first_of("_");

                _clonedInstances[nodeName.substr(0, index)]++;
                std::string temp = nodeName.substr(index + 1);

                auto tempIndex = temp.find_first_of("_");
                if (isNumber(nodeName.substr(index + 1, tempIndex))) {
                    index += temp.find_first_of("_");
                    _clonedWorldTransforms[nodeName.substr(0, index + 1)] = translation * rotation * scale;
                }

                int materialCount = childNode->GetSrcObjectCount<FbxSurfaceMaterial>();
                //There should only be one material for tiles but something is broken which adds one
                for (int i = 0; i < 1; i++) { 

                    FbxSurfaceMaterial* material = (FbxSurfaceMaterial*)childNode->GetSrcObject<FbxSurfaceMaterial>(i);

                    // This only gets the material of type sDiffuse,
                    // you probably need to traverse all Standard Material Property by its name to get all possible textures.
                    FbxProperty propDiffuse = material->FindProperty(FbxSurfaceMaterial::sDiffuse);

                    // Check if it's layeredtextures
                    int layeredTextureCount = propDiffuse.GetSrcObjectCount<FbxLayeredTexture>();

                    //Record layered texture data
                    for (int j = 0; j < layeredTextureCount; j++) {
                        FbxLayeredTexture* layeredTexture = 
                            FbxCast<FbxLayeredTexture>(propDiffuse.GetSrcObject<FbxLayeredTexture>(j));
                        if (layeredTexture != nullptr) {
                            int textureCount = layeredTexture->GetSrcObjectCount<FbxTexture>();
                            for (int textureIndex = 0; textureIndex < textureCount; textureIndex++) {
                                //Fetch the diffuse texture
                                FbxFileTexture* textureFbx = 
                                    FbxCast<FbxFileTexture>(layeredTexture->GetSrcObject<FbxFileTexture>(textureIndex));
                                if (textureFbx != nullptr) {
                                    _tileTextures[nodeName.substr(0, index + 1)].push_back(textureFbx->GetFileName());
                                }
                            }
                            auto textureBroker = TextureBroker::instance();
                            textureBroker->addLayeredTexture(_tileTextures[nodeName.substr(0, index + 1)]);
                        }
                    }
                }
            }
            else {
                _clonedWorldTransforms[nodeName] = translation * rotation * scale;
            }

        }
        _parseTags(childNode);
    }
}

int FbxLoader::_getASCIIFormatIndex(FbxManager* pManager) {
    
    int numFormats = pManager->GetIOPluginRegistry()->GetWriterFormatCount();

    //Set the default format to the native binary format.
    int formatIndex = pManager->GetIOPluginRegistry()->GetNativeWriterFormat();

    //Get the FBX format index whose corresponding description contains "ascii".
    for (int i = 0; i < numFormats; i++) {

        //First check if the writer is an FBX writer.
        if( pManager->GetIOPluginRegistry()->WriterIsFBX(i)) {

            //Obtain the description of the FBX writer.
            std::string description = pManager->GetIOPluginRegistry()->GetWriterFormatDescription(i);

            // Check if the description contains 'ascii'.
            if (description.find("ascii") != std::string::npos) {
                formatIndex = i;
                break;
            }
        }
    }
    return formatIndex;
}

void FbxLoader::saveScene() {
    
    //If current fbx scene has not been copied over yet
    if (!_copiedOverFlag) {
        // Obtain the root node of a scene.
        FbxNode* sceneNode = _scene->GetRootNode();
        FbxCloneManager::Clone(sceneNode, _export.scene->GetRootNode());

        // Determine the number of children there are
        auto rootNode = _export.scene->GetRootNode();
        int numChildren = rootNode->GetChildCount();
        std::vector<FbxNode*> nodesToBeRemoved;
        FbxNode* childNode = nullptr;
        for (int i = 0; i < numChildren; i++) {
            childNode = rootNode->GetChild(i);
            //Prevent root nodes from being saved!
            if (std::string(childNode->GetName()).find("RootNode") != std::string::npos) {
                nodesToBeRemoved.push_back(childNode);
            }
        }
        for (auto child : nodesToBeRemoved) {
            rootNode->RemoveChild(child);
        }
        _copiedOverFlag = true; //no more copying of original scene
    }

    // export the scene.
    _export.exporter->Export(_export.scene);
}

void FbxLoader::clearScene() {

    //If current fbx scene has not been copied over yet
    _export.scene->Clear();
}

std::string FbxLoader::getModelName() {
    return _fileName;
}

void FbxLoader::_searchAndEditNode(FbxNode* rootNode, FbxNode* childNode, std::string modelName, Vector4 location) {

    int numChildren = childNode->GetChildCount();
    if (numChildren == 0) {
        return;
    }
    for (int i = 0; i < numChildren; i++) {
        auto childToEdit = rootNode->FindChild(childNode->GetChild(i)->GetName());
        if (childToEdit != nullptr) {
            childToEdit->LclScaling.Set(FbxDouble3(location.getw(), location.getw(), location.getw()));
            childToEdit->LclTranslation.Set(FbxDouble3(location.getx(), location.gety(), location.getz()));
            childToEdit->SetName((modelName + "_0_"
                + std::string(childToEdit->GetName())).c_str());
        }
        _searchAndEditNode(rootNode, childNode->GetChild(i), modelName, location);
    }
}

void FbxLoader::_searchAndEditMesh(FbxNode* childNode, std::string modelName, Vector4 location) {
    
    int childrenCount = childNode->GetChildCount();
    if (childrenCount == 0) {
        return;
    }
    for (auto i = 0; i < childrenCount; i++) {

        auto node = childNode->GetChild(i);
        FbxMesh* mesh = node->GetMesh();
        if (mesh != nullptr) {
            std::string nodeName = std::string(node->GetName());
            OutputDebugString(nodeName.c_str());
            nodeName.insert(nodeName.find_first_of("_"), "_" + std::to_string(_clonedInstances[modelName]));
            FbxNode* lNode = FbxNode::Create(_export.scene,
                (nodeName +
                    "instance" + std::to_string(_export.scene->GetRootNode()->GetChildCount())).c_str());
            lNode->SetNodeAttribute(mesh);
            lNode->LclScaling.Set(FbxDouble3(location.getw(), location.getw(), location.getw()));
            lNode->LclTranslation.Set(FbxDouble3(location.getx(), location.gety(), location.getz()));
            lNode->LclRotation.Set(node->LclRotation.Get());

            int materialCount = node->GetSrcObjectCount<FbxSurfaceMaterial>();

            for (int i = 0; i < materialCount; i++) {
                FbxSurfaceMaterial* material = (FbxSurfaceMaterial*)node->GetSrcObject<FbxSurfaceMaterial>(i);
                lNode->AddMaterial(material);
            }
            // Add node to the scene
            _export.scene->GetRootNode()->AddChild(lNode);
        }
        _searchAndEditMesh(node, modelName, location);
    }
}

void FbxLoader::_cloneFbxNode(Model* modelAddedTo, FbxLoader* fbxLoader, Vector4 location) {
   
    // Determine the number of children there are
    auto rootNode  = _export.scene->GetRootNode();
    auto node = fbxLoader->getScene()->GetRootNode();
    FbxCloneManager::Clone(node, rootNode);

    auto modelName = fbxLoader->getModelName();
    modelName = modelName.substr(modelName.find_last_of("/") + 1);
    modelName = modelName.substr(0, modelName.find_last_of("."));

    //First check for root node copies
    int numChildren = node->GetChildCount();
    for (int i = 0; i < numChildren; i++) {
        auto childToEdit = rootNode->FindChild(node->GetChild(i)->GetName());
        if (childToEdit != nullptr) {
            childToEdit->LclScaling.Set(FbxDouble3(location.getw(), location.getw(), location.getw()));
            childToEdit->LclTranslation.Set(FbxDouble3(location.getx(), location.gety(), location.getz()));
            childToEdit->SetName((modelName + "_0_"
                + std::string(childToEdit->GetName())).c_str());
        }
    }
    //then search recursively for the rest of the nodes to copy and rename
    _searchAndEditNode(rootNode, node, modelName, location);

    //Remove the root node. is always copied over but is bad for the fbx stability
    int numCopiedChildren = rootNode->GetChildCount();
    std::vector<FbxNode*> childrenToRemove;
    for (int i = 0; i < numCopiedChildren; i++) {
        node = rootNode->GetChild(i);
        //Prevent root nodes from being saved!
        if (std::string(node->GetName()).find("RootNode") != std::string::npos) {
            childrenToRemove.push_back(node);
        }
    }
    for (auto node : childrenToRemove) {
        rootNode->RemoveChild(node);
    }
}

void FbxLoader::_nodeExists(std::string modelName, FbxNode* node, std::vector<FbxNode*>& nodes) {

    // Determine the number of children there are
    int numChildren = node->GetChildCount();
    FbxNode* childNode = nullptr;
    for (int i = 0; i < numChildren; i++) {
        childNode = node->GetChild(i);
        modelName = modelName.substr(modelName.find_last_of("/") + 1);
        modelName = modelName.substr(0, modelName.find_last_of("."));
        std::string name = modelName + "_0_" + childNode->GetName();
        if (childNode != nullptr) {
            auto node = _scene->GetRootNode()->FindChild(name.c_str(), true);
            if (node != nullptr) {
                nodes.push_back(node);
            }
            node = _export.scene->GetRootNode()->FindChild(name.c_str(), true);
            if(node != nullptr) {
                nodes.push_back(node);
            }
        }
        _nodeExists(modelName, childNode, nodes);
    }
}

void FbxLoader::addToScene(Model* modelAddedTo, FbxLoader* modelToLoad, Vector4 location) {

    modelAddedTo->getRenderBuffers()->getVertices()->resize(0);
    modelAddedTo->getRenderBuffers()->getNormals()->resize(0);
    modelAddedTo->getRenderBuffers()->getTextures()->resize(0);
    modelAddedTo->getRenderBuffers()->getIndices()->resize(0);
    modelAddedTo->getRenderBuffers()->getTextureMapIndices()->resize(0);
    modelAddedTo->getRenderBuffers()->getTextureMapNames()->resize(0);

    modelAddedTo->getVAO()->push_back(new VAO());
    //Stride index needs to be reset when adding new models to the scene
    _strideIndex = 0;

    std::vector<FbxNode*> copiedNodes;
    _nodeExists(modelToLoad->getModelName(), modelToLoad->getScene()->GetRootNode(), copiedNodes);
    if (copiedNodes.size() == 0) {
        _cloneFbxNode(modelAddedTo, modelToLoad, location);
    }
    //instance off of cloned mesh
    else {

        std::string modelName = modelToLoad->getModelName();
        modelName = modelName.substr(modelName.find_last_of("/") + 1);
        modelName = modelName.substr(0, modelName.find_last_of("."));

        _clonedInstances[modelName]++;

        // Determine the number of children there are
        auto numChildren = copiedNodes.size();
        FbxNode* childNode = nullptr;
        for (auto i = 0; i < numChildren; i++) {
            childNode = copiedNodes[i];

            FbxMesh* mesh = childNode->GetMesh();
            if (mesh != nullptr) {
                std::string nodeName = std::string(childNode->GetName());
                nodeName.insert(nodeName.find_first_of("_"), "_" + std::to_string(_clonedInstances[modelName]));
                FbxNode* lNode = FbxNode::Create(_export.scene, 
                    (nodeName +
                        "instance" + std::to_string(_export.scene->GetRootNode()->GetChildCount())).c_str());
                lNode->SetNodeAttribute(mesh);
                lNode->LclScaling.Set(FbxDouble3(location.getw(), location.getw(), location.getw()));
                lNode->LclTranslation.Set(FbxDouble3(location.getx(), location.gety(), location.getz()));
                lNode->LclRotation.Set(childNode->LclRotation.Get());

                int materialCount = childNode->GetSrcObjectCount<FbxSurfaceMaterial>();

                for (int i = 0; i < materialCount; i++) {
                    FbxSurfaceMaterial* material = (FbxSurfaceMaterial*)childNode->GetSrcObject<FbxSurfaceMaterial>(i);
                    lNode->AddMaterial(material);
                }
                // Add node to the scene
                _export.scene->GetRootNode()->AddChild(lNode);
            }
            else {

                //Search for mesh in root node
                FbxMesh* mesh = childNode->GetMesh();
                if (mesh != nullptr) {
                    std::string nodeName = std::string(childNode->GetName());
                    OutputDebugString(nodeName.c_str());
                    nodeName.insert(nodeName.find_first_of("_"), "_" + std::to_string(_clonedInstances[modelName]));
                    FbxNode* lNode = FbxNode::Create(_export.scene,
                        (nodeName +
                            "instance" + std::to_string(_export.scene->GetRootNode()->GetChildCount())).c_str());
                    lNode->SetNodeAttribute(mesh);
                    lNode->LclScaling.Set(FbxDouble3(location.getw(), location.getw(), location.getw()));
                    lNode->LclTranslation.Set(FbxDouble3(location.getx(), location.gety(), location.getz()));
                    lNode->LclRotation.Set(childNode->LclRotation.Get());

                    int materialCount = childNode->GetSrcObjectCount<FbxSurfaceMaterial>();

                    for (int i = 0; i < materialCount; i++) {
                        FbxSurfaceMaterial* material = (FbxSurfaceMaterial*)childNode->GetSrcObject<FbxSurfaceMaterial>(i);
                        lNode->AddMaterial(material);
                    }
                    // Add node to the scene
                    _export.scene->GetRootNode()->AddChild(lNode);
                }
                //then search recursively for any other submeshes
                _searchAndEditMesh(childNode, modelName, location);
            }
        }
    }

    std::string modelName = modelToLoad->getModelName();
    modelName = modelName.substr(modelName.find_last_of("/") + 1);
    modelName = modelName.substr(0, modelName.find_last_of("."));

    auto transformation =
        Matrix::translation(location.getx(), location.gety(), location.getz()) * 
        Matrix::scale(location.getw()) * 
        modelToLoad->getObjectSpaceTransform();

    EngineManager::addEntity(ModelBroker::instance()->getModel(modelName), transformation, false);
}

Matrix FbxLoader::getObjectSpaceTransform() {
    return _objectSpaceTransform;
}

void FbxLoader::addTileToScene(Model* modelAddedTo, FbxLoader* modelToLoad, Vector4 location, std::vector<std::string> textures) {

    modelAddedTo->getRenderBuffers()->getVertices()->resize(0);
    modelAddedTo->getRenderBuffers()->getNormals()->resize(0);
    modelAddedTo->getRenderBuffers()->getTextures()->resize(0);
    modelAddedTo->getRenderBuffers()->getIndices()->resize(0);
    modelAddedTo->getRenderBuffers()->getTextureMapIndices()->resize(0);
    modelAddedTo->getRenderBuffers()->getTextureMapNames()->resize(0);

    modelAddedTo->getVAO()->push_back(new VAO());
    //Stride index needs to be reset when adding new models to the scene
    _strideIndex = 0;

    std::vector<FbxNode*> copiedNodes;
    _nodeExists(modelToLoad->getModelName(), modelToLoad->getScene()->GetRootNode(), copiedNodes);

    std::string modelName = modelToLoad->getModelName();
    modelName = modelName.substr(modelName.find_last_of("/") + 1);
    modelName = modelName.substr(0, modelName.find_last_of("."));

    _clonedInstances[modelName]++;

    auto rootNode = _export.scene->GetRootNode();
    std::string nodeName = std::string(modelName);
    nodeName += "_" + std::to_string(_clonedInstances[modelName]) + "_";
    nodeName += "instance" + std::to_string(_export.scene->GetRootNode()->GetChildCount());

    FbxNode* node = FbxNode::Create(_export.scene,
        (nodeName +
            "instance" + std::to_string(_export.scene->GetRootNode()->GetChildCount())).c_str());
    FbxCloneManager::Clone(modelToLoad->getScene()->GetRootNode(), node);

    int numChildren = node->GetChildCount();
    FbxNode* child = nullptr;
    for (int i = 0; i < numChildren; i++) {
        child = node->GetChild(i);
        auto childToEdit = node;
        if (childToEdit != nullptr) {

            childToEdit->LclScaling.Set(FbxDouble3(location.getw(), location.getw(), location.getw()));
            childToEdit->LclTranslation.Set(FbxDouble3(location.getx(), location.gety(), location.getz()));

            std::string nodeName = std::string(modelName);
            nodeName += "_" + std::to_string(_clonedInstances[modelName]) + "_";

            childToEdit->SetName((nodeName +
                "instance" + std::to_string(_export.scene->GetRootNode()->GetChildCount())).c_str());

            int materialCount = childToEdit->GetSrcObjectCount<FbxSurfaceMaterial>();

            for (int i = 0; i < materialCount; i++) {

                FbxSurfaceMaterial* material = (FbxSurfaceMaterial*)childToEdit->GetSrcObject<FbxSurfaceMaterial>(i);

                // This only gets the material of type sDiffuse,
                // you probably need to traverse all Standard Material Property by its name to get all possible textures.
                FbxProperty propDiffuse = material->FindProperty(FbxSurfaceMaterial::sDiffuse);

                // Check if it's layeredtextures
                int layeredTextureCount = propDiffuse.GetSrcObjectCount<FbxLayeredTexture>();

                std::vector< FbxFileTexture*> toRemove;
                std::vector< FbxFileTexture*> toAdd;
                FbxLayeredTexture* layeredTextureToEdit = nullptr;

                if (layeredTextureCount > 0) {
                    for (int j = 0; j < layeredTextureCount; j++) {
                        FbxLayeredTexture* layeredTexture = FbxCast<FbxLayeredTexture>(propDiffuse.GetSrcObject<FbxLayeredTexture>(j));
                        layeredTextureToEdit = layeredTexture;
                        if (layeredTexture != nullptr) {

                            int textureCount = layeredTexture->GetSrcObjectCount<FbxTexture>();
                            int diffuseTextureCounter = 0;
                            for (int textureIndex = 0; textureIndex < textureCount; textureIndex++) {

                                //Fetch the diffuse texture
                                FbxFileTexture* textureFbx = FbxCast<FbxFileTexture >(layeredTexture->GetSrcObject<FbxFileTexture >(textureIndex));
                                if (textureFbx != nullptr) {

                                    if (std::string(textureFbx->GetFileName()).find("alpha") != std::string::npos) {

                                        std::string alphaMapName = modelName + "/alphamapclone";
                                        alphaMapName += std::to_string(static_cast<int>(location.getx())) + "_" +
                                            std::to_string(static_cast<int>(location.gety())) + "_" +
                                            std::to_string(static_cast<int>(location.getz()));
                                        MutableTexture newAlphaMap("alphamap", alphaMapName);
                                        // Then, you can get all the properties of the texture, including its name
                                        std::string texture = textureFbx->GetFileName();
                                        //Finds second to last position of string and use that for file access name
                                        std::string filePrefix = texture.substr(texture.substr(0, texture.find_last_of("/\\")).find_last_of("/\\"));
                                        filePrefix = filePrefix.substr(0, filePrefix.find_last_of("/\\") + 1);

                                        FbxFileTexture* gTexture = FbxFileTexture::Create(_fbxManager, "");
                                        // Set texture properties.
                                        gTexture->SetFileName((TEXTURE_LOCATION + "/" + alphaMapName + ".tif").c_str());
                                        gTexture->SetTextureUse(FbxFileTexture::eStandard);
                                        gTexture->SetMappingType(FbxFileTexture::eUV);
                                        gTexture->SetMaterialUse(FbxFileTexture::eModelMaterial);
                                        gTexture->SetSwapUV(false);
                                        gTexture->SetTranslation(0.0, 0.0);
                                        gTexture->SetScale(1.0, 1.0);
                                        gTexture->SetRotation(0.0, 0.0);

                                        toAdd.push_back(gTexture);
                                        toRemove.push_back(textureFbx);
                                    }
                                    else {

                                        // Then, you can get all the properties of the texture, including its name
                                        std::string texture = textureFbx->GetFileName();
                                        //Finds second to last position of string and use that for file access name
                                        std::string filePrefix = texture.substr(texture.substr(0, texture.find_last_of("/\\")).find_last_of("/\\"));
                                        filePrefix = filePrefix.substr(0, filePrefix.find_last_of("/\\") + 1);

                                        FbxFileTexture* gTexture = FbxFileTexture::Create(_fbxManager, "");
                                        // Set texture properties.
                                        gTexture->SetFileName((TEXTURE_LOCATION + modelName + "/" + textures[textureIndex]).c_str());
                                        gTexture->SetTextureUse(FbxFileTexture::eStandard);
                                        gTexture->SetMappingType(FbxFileTexture::eUV);
                                        gTexture->SetMaterialUse(FbxFileTexture::eModelMaterial);
                                        gTexture->SetSwapUV(false);
                                        gTexture->SetTranslation(0.0, 0.0);
                                        gTexture->SetScale(1.0, 1.0);
                                        gTexture->SetRotation(0.0, 0.0);

                                        toAdd.push_back(gTexture);
                                        toRemove.push_back(textureFbx);
                                    }
                                }
                            }
                        }
                        layeredTextureToEdit->WipeAllConnections();
                        for (int z = 0; z < toRemove.size(); z++) {
                            layeredTextureToEdit->DisconnectSrcObject(toRemove[z]);
                            layeredTextureToEdit->DisconnectDstObject(toRemove[z]);
                            toRemove[z]->DisconnectAllDstObject(); 
                            toRemove[z]->DisconnectAllSrcObject();
                        }

                        for (int z = 0; z < toAdd.size(); z++) {
                            OutputDebugString(toAdd[z]->GetFileName());
                            OutputDebugStringW(L"\n");
                            layeredTextureToEdit->ConnectSrcObject(toAdd[z]);
                        }
                    }
                }
            }
        }
    }

    rootNode->AddChild(node);

    //Remove the root node. is always copied over but is bad for the fbx stability
    int numCopiedChildren = rootNode->GetChildCount();
    std::vector<FbxNode*> childrenToRemove;
    for (int i = 0; i < numCopiedChildren; i++) {
        auto childNode = rootNode->GetChild(i);
        //Prevent root nodes from being saved!
        if (std::string(childNode->GetName()).find("RootNode") != std::string::npos) {
            childrenToRemove.push_back(childNode);
        }
    }
    for (auto node : childrenToRemove) {
        rootNode->RemoveChild(node);
    }
    
    auto transformation =
        Matrix::translation(location.getx(), location.gety(), location.getz()) *
        Matrix::scale(location.getw()) *
        modelToLoad->getObjectSpaceTransform();

    auto entity = EngineManager::addEntity(ModelBroker::instance()->getModel(modelName), transformation, false);

    auto textureBroker = TextureBroker::instance();
    std::string layeredTextureName = "Layered";
    std::vector<std::string> createLayeredTextureNames;
    for (auto& texture : textures) {
        layeredTextureName += TEXTURE_LOCATION + modelName + "/" + texture;
        createLayeredTextureNames.push_back(TEXTURE_LOCATION + modelName + "/" + texture);
    }
    std::string alphaMapName = modelName + "/alphamapclone";
    alphaMapName += std::to_string(static_cast<int>(location.getx())) + "_" +
        std::to_string(static_cast<int>(location.gety())) + "_" +
        std::to_string(static_cast<int>(location.getz()));
    MutableTexture newAlphaMap("alphamap", alphaMapName);
    layeredTextureName += TEXTURE_LOCATION + alphaMapName + ".tif";
    createLayeredTextureNames.push_back(TEXTURE_LOCATION + alphaMapName + ".tif");

    //Add if doesn't exist
    textureBroker->addLayeredTexture(createLayeredTextureNames);

    auto layeredTexture = textureBroker->getLayeredTexture(layeredTextureName);
    entity->setLayeredTexture(layeredTexture);
}

void FbxLoader::loadAnimatedModel(AnimatedModel* model, FbxNode* node) {

    // Determine the number of children there are
    int numChildren = node->GetChildCount();
    FbxNode* childNode = nullptr;
    for (int i = 0; i < numChildren; i++) {
        childNode = node->GetChild(i);

        FbxMesh* mesh = childNode->GetMesh();
        if (mesh != nullptr) {
            //Look for a skin animation
            int deformerCount = mesh->GetDeformerCount();

            FbxDeformer* pFBXDeformer;
            FbxSkin*     pFBXSkin;

            for (int i = 0; i < deformerCount; ++i) {
                pFBXDeformer = mesh->GetDeformer(i);

                if (pFBXDeformer == nullptr || pFBXDeformer->GetDeformerType() != FbxDeformer::eSkin) {
                    continue;
                }

                pFBXSkin = (FbxSkin*)(pFBXDeformer);
                if (pFBXSkin == nullptr) {
                    continue;
                }
                loadAnimatedModelData(model, pFBXSkin, node, mesh);
            }
        }
        loadAnimatedModel(model, childNode);
    }

}

void FbxLoader::loadAnimatedModelData(AnimatedModel* model, FbxSkin* pSkin, FbxNode* node, FbxMesh* mesh) {

    //Used to supply animation frame data for skinning a model
    std::vector<SkinningData> skins;

    //Load the global animation stack information
    FbxAnimStack* currAnimStack = node->GetScene()->GetSrcObject<FbxAnimStack>(0);
    FbxString animStackName = currAnimStack->GetName();
    FbxTakeInfo* takeInfo = node->GetScene()->GetTakeInfo(animStackName);
    FbxTime start = takeInfo->mLocalTimeSpan.GetStart();
    FbxTime end = takeInfo->mLocalTimeSpan.GetStop();
    int animationFrames = (int)(end.GetFrameCount(FbxTime::eFrames60) - start.GetFrameCount(FbxTime::eFrames60) + 1);

    auto animation = model->getAnimation();
    animation->setFrames(animationFrames); //Set the number of frames this animation contains
    auto previousSkins = animation->getSkins();
    int indexOffset = 0;
    if (previousSkins.size() > 0) {
        indexOffset = previousSkins.back().getIndexOffset();
    }

    int clusterCount = pSkin->GetClusterCount();
    for (int i = 0; i < clusterCount; ++i) {

        auto pCluster = pSkin->GetCluster(i);
        if (pCluster == nullptr) {
            continue;
        }

        auto pLinkNode = pCluster->GetLink();
        if (pLinkNode == nullptr) {
            continue;
        }
        skins.push_back(SkinningData(pCluster, node, animationFrames, indexOffset + mesh->GetControlPointsCount()));
    }

    if (model->getClassType() == ModelClass::AnimatedModelType) {

        if (animation->getSkins().size() > 0) {
            for (SkinningData& skin : skins) {
                auto ind = skin.getIndexes();
                transform(ind->begin(), ind->end(), ind->begin(), bind2nd(std::plus<int>(), indexOffset));
            }
        }
    }
    animation->addSkin(skins);
}

void FbxLoader::loadGeometry(Model* model, FbxNode* node) {

    // Determine the number of children there are
    int numChildren = node->GetChildCount();
    FbxNode* childNode = nullptr;
    for (int i = 0; i < numChildren; i++) {
        childNode = node->GetChild(i);

        FbxMesh* mesh = childNode->GetMesh();
        if (mesh != nullptr) {
            loadGeometryData(model, mesh, childNode);
        }
        loadGeometry(model, childNode);
    }
}

void FbxLoader::buildAnimationFrames(AnimatedModel* model, std::vector<SkinningData>& skins) {

    RenderBuffers* renderBuffers = model->getRenderBuffers();
    Animation* animation = model->getAnimation();
    std::vector<Vector4>* vertices = renderBuffers->getVertices(); //Get the reference to an object to prevent copy
    size_t verticesSize = vertices->size();
    std::vector<int>* indices = renderBuffers->getIndices();
    size_t indicesSize = indices->size();
    int animationFrames = animation->getFrameCount();

    std::vector<std::vector<int>>* boneIndexes = new std::vector<std::vector<int>>(verticesSize); //Bone indexes per vertex
    std::vector<std::vector<float>>* boneWeights = new std::vector<std::vector<float>>(verticesSize); //Bone weights per vertex
    int boneIndex = 0;
    for (SkinningData skin : skins) { //Dereference vertices
        std::vector<int>*    indexes = skin.getIndexes(); //Get all of the vertex indexes that are affected by this skinning data
        std::vector<float>* weights = skin.getWeights(); //Get all of the vertex weights that are affected by this skinning data
        int subSkinIndex = 0;
        for (int index : *indexes) { //Derefence indexes

            (*boneIndexes)[index].push_back(boneIndex);
            (*boneWeights)[index].push_back((*weights)[subSkinIndex]);
            ++subSkinIndex; //Indexer for other vectors
        }
        boneIndex++;
    }

    //Pad data
    for (int i = 0; i < boneWeights->size(); i++) {
        (*boneWeights)[i].resize(8);
        (*boneIndexes)[i].resize(8);
    }

    animation->setBoneIndexWeights(boneIndexes, boneWeights);

    for (int animationFrame = 0; animationFrame < animationFrames; ++animationFrame) {

        std::vector<Matrix>* bones = new std::vector<Matrix>(); //Bone weights per vertex
        for (SkinningData skin : skins) { //Dereference vertices
            std::vector<Matrix>* vertexTransforms = skin.getFrameVertexTransforms(); //Get all of the vertex transform data per frame
            bones->push_back((*vertexTransforms)[animationFrame]);
        }
        animation->addBoneTransforms(bones); //Add a new frame to the animation and do not store in GPU memory yet
    }
}

void FbxLoader::loadGeometryData(Model* model, FbxMesh* meshNode, FbxNode* childNode) {

    //Get the indices from the mesh
    int  numIndices = meshNode->GetPolygonVertexCount();
    int* indicesPointer = meshNode->GetPolygonVertices();
    std::vector<int> indices(indicesPointer, indicesPointer + numIndices);

    //Get the vertices from the mesh
    std::vector<Vector4> vertices;
    _loadVertices(meshNode, vertices);

    //Build the entire model in one long stream of vertex, normal and texture buffers
    _buildGeometryData(model, vertices, indices, childNode);
}

void FbxLoader::loadModelData(Model* model, FbxMesh* meshNode, FbxNode* childNode) {

    
    //Get the indices from the mesh
    int* indices = nullptr;
    _loadIndices(model, meshNode, indices);

    //Get the vertices from the mesh
    std::vector<Vector4> vertices;
    _loadVertices(meshNode, vertices);

    //Get the normals from the mesh
    std::vector<Vector4> normals;
    _loadNormals(meshNode, indices, normals);

    //Get the textures UV/ST coordinates from the mesh
    std::vector<Tex2> textures;
    _loadTextureUVs(meshNode, textures);

    //Send texture image data to GPU
    _loadTextures(model, meshNode, childNode);

    //Build the entire model in one long stream of vertex, normal and texture buffers
    _buildModelData(model, meshNode, childNode, vertices, normals, textures);
}

void FbxLoader::buildAABB(Model* model) {
    
    //Choose a min and max in either x y or z and that will be the diameter of the sphere
    float min[3] = { (std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)() };
    float max[3] = { (std::numeric_limits<float>::min)(), (std::numeric_limits<float>::min)(), (std::numeric_limits<float>::min)() };

    auto geometry = model->getGeometry();

    for (auto sphere : *geometry->getSpheres()) {
        Vector4 center = sphere.getObjectPosition();
        float radius = sphere.getRadius();
        if (center.getx() - radius < min[0]) {
            min[0] = center.getx() - radius;
        }
        if (center.getx() + radius > max[0]) {
            max[0] = center.getx() + radius;
        }

        if (center.gety() - radius < min[1]) {
            min[1] = center.gety() - radius;
        }
        if (center.gety() + radius > max[1]) {
            max[1] = center.gety() + radius;
        }

        if (center.getz() - radius < min[2]) {
            min[2] = center.getz() - radius;
        }
        if (center.getz() + radius > max[2]) {
            max[2] = center.getz() + radius;
        }
    }
   
    float xCenter = min[0] + ((max[0] - min[0]) / 2.0f);
    float yCenter = min[1] + ((max[1] - min[1]) / 2.0f);
    float zCenter = min[2] + ((max[2] - min[2]) / 2.0f);
    Vector4 center(xCenter, yCenter, zCenter);
    model->setAABB(new Cube(max[0] - min[0], max[1] - min[1], max[2] - min[2], center));
}

void FbxLoader::_buildGeometryData(Model* model, std::vector<Vector4>& vertices, std::vector<int>& indices, FbxNode* node) {

    //Global transform doesn't work always so keep it here just in case
    /*FbxDouble* TBuff = node->EvaluateGlobalTransform().GetT().Buffer();
    FbxDouble* RBuff = node->EvaluateGlobalTransform().GetR().Buffer();
    FbxDouble* SBuff = node->EvaluateGlobalTransform().GetS().Buffer();*/

    FbxDouble* TBuff = node->LclTranslation.Get().Buffer();
    FbxDouble* RBuff = node->LclRotation.Get().Buffer();
    FbxDouble* SBuff = node->LclScaling.Get().Buffer();

    //Compute x, y and z rotation vectors by multiplying through
    Matrix rotation = Matrix::rotationAroundX(static_cast<float>(RBuff[0])) *
        Matrix::rotationAroundY(static_cast<float>(RBuff[1])) *
        Matrix::rotationAroundZ(static_cast<float>(RBuff[2]));
    Matrix translation = Matrix::translation(static_cast<float>(TBuff[0]), static_cast<float>(TBuff[1]), static_cast<float>(TBuff[2]));
    Matrix scale = Matrix::scale(static_cast<float>(SBuff[0]), static_cast<float>(SBuff[1]), static_cast<float>(SBuff[2]));

    int triCount = 0;
    Matrix transformation = translation * rotation * scale;

    if (model->getGeometryType() == GeometryType::Triangle) {
        size_t totalTriangles = indices.size() / 3; //Each index represents one vertex and a triangle is 3 vertices
        //Read each triangle vertex indices and store them in triangle array
        for (int i = 0; i < totalTriangles; i++) {

            Vector4 A(vertices[indices[triCount]].getx(),
                vertices[indices[triCount]].gety(),
                vertices[indices[triCount]].getz(),
                1.0);
            triCount++;

            Vector4 B(vertices[indices[triCount]].getx(),
                vertices[indices[triCount]].gety(),
                vertices[indices[triCount]].getz(),
                1.0);
            triCount++;

            Vector4 C(vertices[indices[triCount]].getx(),
                vertices[indices[triCount]].gety(),
                vertices[indices[triCount]].getz(),
                1.0);
            triCount++;

            //Add triangle to geometry object stored in model class
            model->addGeometryTriangle(Triangle(transformation * A, transformation * B, transformation * C));
        }
    }
    else if (model->getGeometryType() == GeometryType::Sphere) {
        //Choose a min and max in either x y or z and that will be the diameter of the sphere
        float min[3] = { (std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)() };
        float max[3] = { (std::numeric_limits<float>::min)(), (std::numeric_limits<float>::min)(), (std::numeric_limits<float>::min)() };
        size_t totalTriangles = indices.size(); //Each index represents one vertex

        //Find the minimum and maximum x position which will render us a diameter
        for (int i = 0; i < totalTriangles; i++) {

            auto vert = transformation * vertices[indices[i]];
            float xPos = vert.getx();
            float yPos = vert.gety();
            float zPos = vert.getz();

            if (xPos < min[0]) {
                min[0] = xPos;
            }
            if (xPos > max[0]) {
                max[0] = xPos;
            }

            if (yPos < min[1]) {
                min[1] = yPos;
            }
            if (yPos > max[1]) {
                max[1] = yPos;
            }

            if (zPos < min[2]) {
                min[2] = zPos;
            }
            if (zPos > max[2]) {
                max[2] = zPos;
            }
        }
        float radius = (max[0] - min[0]) / 2.0f;
        float xCenter = min[0] + radius;
        float yCenter = min[1] + radius;
        float zCenter = min[2] + radius;
        //Add sphere to geometry object stored in model class
        model->addGeometrySphere(Sphere(radius, Vector4(xCenter, yCenter, zCenter, 1.0f)));
    }
}

void FbxLoader::_loadIndices(Model* model, FbxMesh* meshNode, int*& indices) {

    RenderBuffers* renderBuffers = model->getRenderBuffers();
    //Get the indices from the model
    int  numIndices = meshNode->GetPolygonVertexCount();
    indices = meshNode->GetPolygonVertices();

    if (model->getClassType() == ModelClass::ModelType) {
        renderBuffers->setVertexIndices(std::vector<int>(indices, indices + numIndices)); //Copy vector
    }
    else if (model->getClassType() == ModelClass::AnimatedModelType) {
        std::vector<int> newIndices(indices, indices + numIndices);

        //Find previous maximum vertex index and add to all of the sequential indexes
        auto currentIndices = renderBuffers->getIndices();
        if (currentIndices->size() > 0) {
            auto maxIndex = std::max_element(currentIndices->begin(), currentIndices->end());
            //Add one to the maxIndex otherwise this model uses the last vertex from the previous model!!!!!!!!!!!!!
            transform(newIndices.begin(), newIndices.end(), newIndices.begin(), bind2nd(std::plus<int>(), (*maxIndex) + 1));
        }
        renderBuffers->addVertexIndices(newIndices); //Copy vector
    }
}

void FbxLoader::_loadVertices(FbxMesh* meshNode, std::vector<Vector4>& vertices) {
    //Get the vertices from the model
    int  numVerts = meshNode->GetControlPointsCount();
    for (int j = 0; j < numVerts; j++) {
        FbxVector4 coord = meshNode->GetControlPointAt(j);
        vertices.push_back(Vector4((float)coord.mData[0], (float)coord.mData[1], (float)coord.mData[2], 1.0));
    }
}

void FbxLoader::_buildModelData(Model* model, FbxMesh* meshNode, FbxNode* childNode, std::vector<Vector4>& vertices,
    std::vector<Vector4>& normals, std::vector<Tex2>& textures) {

    RenderBuffers* renderBuffers = model->getRenderBuffers();
    int numVerts = meshNode->GetControlPointsCount();
    //Load in models differently based on type
    if (model->getClassType() == ModelClass::AnimatedModelType) {
        //Load vertices and normals into model
        for (int i = 0; i < numVerts; i++) {
            renderBuffers->addVertex(vertices[i]);
            renderBuffers->addNormal(normals[i]);
            renderBuffers->addDebugNormal(vertices[i]);
            renderBuffers->addDebugNormal(vertices[i] + normals[i]);
        }
        //Load texture coordinates
        for (int i = 0; i < textures.size(); i++) {
            renderBuffers->addTexture(textures[i]);
        }

        FbxDouble* TBuff = childNode->LclTranslation.Get().Buffer();
        FbxDouble* RBuff = childNode->LclRotation.Get().Buffer();
        FbxDouble* SBuff = childNode->LclScaling.Get().Buffer();

        //Compute x, y and z rotation vectors by multiplying through
        Matrix rotation = Matrix::rotationAroundX(static_cast<float>(RBuff[0])) *
            Matrix::rotationAroundY(static_cast<float>(RBuff[1])) *
            Matrix::rotationAroundZ(static_cast<float>(RBuff[2]));
        Matrix translation = Matrix::translation(static_cast<float>(TBuff[0]), static_cast<float>(TBuff[1]), static_cast<float>(TBuff[2]));
        Matrix scale = Matrix::scale(static_cast<float>(SBuff[0]), static_cast<float>(SBuff[1]), static_cast<float>(SBuff[2]));

        Matrix transformation = translation * rotation;// *scale;
        _objectSpaceTransform = transformation;
    }
    else if (model->getClassType() == ModelClass::ModelType) {
        //Load in the entire model once if the data is not animated
        _buildTriangles(model, vertices, normals, textures, *renderBuffers->getIndices(), childNode);
    }
}

void FbxLoader::_loadNormals(FbxMesh* meshNode, int* indices, std::vector<Vector4>& normals) {
    std::map<int, Vector4> mappingNormals;
    FbxGeometryElementNormal* normalElement = meshNode->GetElementNormal();
    const FbxLayerElement::EMappingMode mapMode = normalElement->GetMappingMode();
    if (normalElement) {
        if (mapMode == FbxLayerElement::EMappingMode::eByPolygonVertex) {
            int numNormals = meshNode->GetPolygonCount() * 3;

            // Loop through the triangle meshes
            for (int polyCounter = 0; polyCounter < numNormals; ++polyCounter) {
                //Get the normal for this vertex
                FbxVector4 normal = normalElement->GetDirectArray().GetAt(polyCounter);
                mappingNormals[indices[polyCounter]] = Vector4((float)normal[0], (float)normal[1], (float)normal[2], 1.0f);
            }

            for (int j = 0; j < mappingNormals.size(); j++) {
                FbxVector4 coord = meshNode->GetControlPointAt(j);
                normals.push_back(Vector4(mappingNormals[j].getx(), mappingNormals[j].gety(), mappingNormals[j].getz(), 1.0));
            }
        }
        else if (mapMode == FbxLayerElement::EMappingMode::eByControlPoint) {
            //Let's get normals of each vertex, since the mapping mode of normal element is by control point
            for (int vertexIndex = 0; vertexIndex < meshNode->GetControlPointsCount(); ++vertexIndex) {
                int normalIndex = 0;
                //reference mode is direct, the normal index is same as vertex index.
                //get normals by the index of control vertex
                if (normalElement->GetReferenceMode() == FbxGeometryElement::eDirect) {
                    normalIndex = vertexIndex;
                }

                //reference mode is index-to-direct, get normals by the index-to-direct
                if (normalElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect) {
                    normalIndex = normalElement->GetIndexArray().GetAt(vertexIndex);
                }

                //Got normals of each vertex.
                FbxVector4 normal = normalElement->GetDirectArray().GetAt(normalIndex);
                normals.push_back(Vector4((float)normal[0], (float)normal[1], (float)normal[2], 1.0));
            }
        }
    }
}

void FbxLoader::_loadTextureUVs(FbxMesh* meshNode, std::vector<Tex2>& textures) {

    int numIndices = meshNode->GetPolygonVertexCount();

    //Get the texture coordinates from the mesh
    FbxVector2 uv;
    FbxGeometryElementUV*                 leUV = meshNode->GetElementUV(0);
    const FbxLayerElement::EReferenceMode refMode = leUV->GetReferenceMode();
    const FbxLayerElement::EMappingMode   textMapMode = leUV->GetMappingMode();
    if (refMode == FbxLayerElement::EReferenceMode::eDirect) {
        int numTextures = meshNode->GetTextureUVCount();
        // Loop through the triangle meshes
        for (int textureCounter = 0; textureCounter < numTextures; textureCounter++) {
            //Get the normal for this vertex
            FbxVector2 uvTexture = leUV->GetDirectArray().GetAt(textureCounter);
            textures.push_back(Tex2(static_cast<float>(uvTexture[0]), static_cast<float>(uvTexture[1])));
        }
    }
    else if (refMode == FbxLayerElement::EReferenceMode::eIndexToDirect) {
        int indexUV;
        for (int i = 0; i < numIndices; i++) {
            //Get the normal for this vertex
            indexUV = leUV->GetIndexArray().GetAt(i);
            FbxVector2 uvTexture = leUV->GetDirectArray().GetAt(indexUV);
            textures.push_back(Tex2(static_cast<float>(uvTexture[0]), static_cast<float>(uvTexture[1])));
        }
    }
}

void FbxLoader::_generateTextureStrides(FbxMesh* meshNode, std::vector<std::pair<int, int>>& textureStrides) {
    //Get material element info
    FbxLayerElementMaterial* pLayerMaterial = meshNode->GetLayer(0)->GetMaterials();
    //Get material mapping info
    FbxLayerElementArrayTemplate<int> *tmpArray = &pLayerMaterial->GetIndexArray();
    //Get mapping mode
    FbxLayerElement::EMappingMode mapMode = pLayerMaterial->GetMappingMode();

    int textureCountage = tmpArray->GetCount();

    if (mapMode == FbxLayerElement::EMappingMode::eAllSame) {
        textureStrides.push_back(std::pair<int, int>(0, meshNode->GetPolygonVertexCount())); //All the same assign 0
    }
    else {
        int currMaterial;
        int vertexStride = 0;
        for (int i = 0; i < textureCountage; i++) {
            if (i == 0) {
                currMaterial = tmpArray->GetAt(i);
            }
            else {
                if (currMaterial != tmpArray->GetAt(i)) {
                    if (mapMode == FbxLayerElement::EMappingMode::eByPolygon) {
                        textureStrides.push_back(std::pair<int, int>(currMaterial, vertexStride * 3)); //Multiply by 3 because materials are done per triangle not per vertex
                    }
                    else if (mapMode == FbxLayerElement::EMappingMode::eByPolygonVertex) {
                        textureStrides.push_back(std::pair<int, int>(currMaterial, vertexStride));
                    }
                    vertexStride = 0;
                    currMaterial = tmpArray->GetAt(i);
                }
            }
            vertexStride++;
        }
        if (mapMode == FbxLayerElement::EMappingMode::eByPolygon) {
            textureStrides.push_back(std::pair<int, int>(currMaterial, vertexStride * 3)); //Multiply by 3 because materials are done per triangle not per vertex
        }
        else if (mapMode == FbxLayerElement::EMappingMode::eByPolygonVertex) {
            textureStrides.push_back(std::pair<int, int>(currMaterial, vertexStride));
        }
    }
}

bool FbxLoader::_loadTexture(Model* model, int textureStride, FbxFileTexture* textureFbx) {

    if (textureFbx != nullptr) {
        // Then, you can get all the properties of the texture, including its name
        std::string textureName = textureFbx->GetFileName();
        std::string textureNameTemp = textureName; //Used a temporary storage to not overwrite textureName
        std::string texturePath = TEXTURE_LOCATION;
        //Finds second to last position of string and use that for file access name
        texturePath.append(textureName.substr(textureNameTemp.substr(0, textureNameTemp.find_last_of("/\\")).find_last_of("/\\") + 1));
        model->addTexture(texturePath, textureStride);
        return true;
    }
    return false;
}

bool FbxLoader::_loadLayeredTexture(Model* model, int textureStride, FbxLayeredTexture* layered_texture) {

    std::vector<std::string> layeredTextureNames;
    int textureCount = layered_texture->GetSrcObjectCount<FbxTexture>();
    for (int textureIndex = 0; textureIndex < textureCount; textureIndex++) {

        //Fetch the diffuse texture
        FbxFileTexture* textureFbx = FbxCast<FbxFileTexture >(layered_texture->GetSrcObject<FbxFileTexture >(textureIndex));
        if (textureFbx != nullptr) {

            // Then, you can get all the properties of the texture, including its name
            std::string textureName = textureFbx->GetFileName();
            std::string textureNameTemp = textureName; //Used a temporary storage to not overwrite textureName
            std::string texturePath = TEXTURE_LOCATION;
            //Finds second to last position of string and use that for file access name
            texturePath.append(textureName.substr(textureNameTemp.substr(0, textureNameTemp.find_last_of("/\\")).find_last_of("/\\")));

            layeredTextureNames.push_back(texturePath);
        }
        else {
            return false;
        }
    }

    model->addLayeredTexture(layeredTextureNames, textureStride);

    return true;
}

void FbxLoader::_loadTextures(Model* model, FbxMesh* meshNode, FbxNode* childNode) {

    //First verify if mesh nodes has materials...
    if (meshNode->GetLayerCount() == 0) {
        return;
    }
    std::vector<std::pair<int, int>> strides;
    _generateTextureStrides(meshNode, strides);

    //Return the number of materials found in mesh
    int materialCount = childNode->GetSrcObjectCount<FbxSurfaceMaterial>();
    int textureStrideIndex = 0;
    for(auto stride : strides) {

        int materialIndex = stride.first;

        FbxSurfaceMaterial* material = (FbxSurfaceMaterial*)childNode->GetSrcObject<FbxSurfaceMaterial>(materialIndex);
        if (material == nullptr) {
            continue;
        }

        // This only gets the material of type sDiffuse,
        // you probably need to traverse all Standard Material Property by its name to get all possible textures.
        FbxProperty propDiffuse = material->FindProperty(FbxSurfaceMaterial::sDiffuse);

        // Check if it's layeredtextures
        int layeredTextureCount = propDiffuse.GetSrcObjectCount<FbxLayeredTexture>();

        if (layeredTextureCount > 0) {
            for (int j = 0; j < layeredTextureCount; j++) {
                FbxLayeredTexture* layered_texture = FbxCast<FbxLayeredTexture>(propDiffuse.GetSrcObject<FbxLayeredTexture>(j));
                if (layered_texture != nullptr) {
                    int textureCount = layered_texture->GetSrcObjectCount<FbxTexture>();
                    if (textureCount == 5 &&
                        _loadLayeredTexture(model, stride.second, layered_texture)) {
                        if (textureStrideIndex < strides.size() - 1) {
                            textureStrideIndex++;
                        }
                    }
                    else {
                        for (int textureIndex = 0; textureIndex < textureCount; textureIndex++) {
                            //Fetch the diffuse texture
                            FbxFileTexture* textureFbx = FbxCast<FbxFileTexture >(layered_texture->GetSrcObject<FbxFileTexture >(textureIndex));
                            if (_loadTexture(model, stride.second, textureFbx)) {
                                if (textureStrideIndex < strides.size() - 1) {
                                    textureStrideIndex++;
                                }
                            }
                        }
                    }
                }
            }
        }
        else {
            // Get number of textures in the material which could contain diffuse, bump, normal and/or specular maps...
            int textureCount = propDiffuse.GetSrcObjectCount<FbxTexture>();

            for (int textureIndex = 0; textureIndex < textureCount; ++textureIndex) {

                //Fetch the diffuse texture
                FbxFileTexture* textureFbx = FbxCast<FbxFileTexture>(propDiffuse.GetSrcObject<FbxFileTexture>(textureIndex));
                if (_loadTexture(model, stride.second, textureFbx)) {
                    if (textureStrideIndex < strides.size() - 1) {
                        textureStrideIndex++;
                    }
                }
            }
        }
    }
}

void FbxLoader::_buildTriangles(Model* model, std::vector<Vector4>& vertices, std::vector<Vector4>& normals,
    std::vector<Tex2>& textures, std::vector<int>& indices, FbxNode* node) {

    RenderBuffers* renderBuffers = model->getRenderBuffers();
    //Global transform doesn't work always so keep it here just in case
    /*FbxDouble* TBuff = node->EvaluateGlobalTransform().GetT().Buffer();
    FbxDouble* RBuff = node->EvaluateGlobalTransform().GetR().Buffer();
    FbxDouble* SBuff = node->EvaluateGlobalTransform().GetS().Buffer();*/

    FbxDouble* TBuff = node->LclTranslation.Get().Buffer();
    FbxDouble* RBuff = node->LclRotation.Get().Buffer();
    FbxDouble* SBuff = node->LclScaling.Get().Buffer();

    //Compute x, y and z rotation vectors by multiplying through
    Matrix rotation = Matrix::rotationAroundX(static_cast<float>(RBuff[0])) *
        Matrix::rotationAroundY(static_cast<float>(RBuff[1])) *
        Matrix::rotationAroundZ(static_cast<float>(RBuff[2]));
    Matrix translation = Matrix::translation(static_cast<float>(TBuff[0]), static_cast<float>(TBuff[1]), static_cast<float>(TBuff[2]));
    Matrix scale = Matrix::scale(static_cast<float>(SBuff[0]), static_cast<float>(SBuff[1]), static_cast<float>(SBuff[2]));

    int triCount = 0;
    _objectSpaceTransform = translation * rotation;// *scale;

    std::vector<int> flattenStrides;
    auto vao = model->getVAO();
    auto strides = (*vao)[vao->size() - 1]->getTextureStrides();
    int i = 0;
    for (auto stride : strides) {
        if (i >= _strideIndex) {
            renderBuffers->addTextureMapName(stride.first);
            int textureIndex = renderBuffers->getTextureMapIndex(stride.first);
            for (int j = 0; j < stride.second; j++) {
                flattenStrides.push_back(textureIndex);
            }
        }
        i++;
    }

    if (i == _strideIndex) {
        int strideCount = 0;
        for (auto stride : strides) {
            strideCount += stride.second;
        }
        flattenStrides.resize(strideCount);
    }
    _strideIndex = static_cast<int>(strides.size());

    size_t totalTriangles = indices.size() / 3; //Each index represents one vertex and a triangle is 3 vertices
    //Read each triangle vertex indices and store them in triangle array
    for (int i = 0; i < totalTriangles; i++) {
        Vector4 A(vertices[indices[triCount]].getx(),
            vertices[indices[triCount]].gety(),
            vertices[indices[triCount]].getz(),
            1.0);
        renderBuffers->addVertex(A); //Scale then rotate vertex

        Vector4 AN(normals[indices[triCount]].getx(),
            normals[indices[triCount]].gety(),
            normals[indices[triCount]].getz(),
            1.0);
        renderBuffers->addNormal(rotation * AN); //Scale then rotate normal
        renderBuffers->addTexture(textures[triCount]);
        renderBuffers->addDebugNormal(A);
        renderBuffers->addDebugNormal((A) + (rotation * AN));
        renderBuffers->addTextureMapIndex(flattenStrides[triCount]);
        triCount++;

        Vector4 B(vertices[indices[triCount]].getx(),
            vertices[indices[triCount]].gety(),
            vertices[indices[triCount]].getz(),
            1.0);
        renderBuffers->addVertex(B); //Scale then rotate vertex

        Vector4 BN(normals[indices[triCount]].getx(),
            normals[indices[triCount]].gety(),
            normals[indices[triCount]].getz(),
            1.0);
        renderBuffers->addNormal(rotation * BN); //Scale then rotate normal
        renderBuffers->addTexture(textures[triCount]);
        renderBuffers->addDebugNormal(B);
        renderBuffers->addDebugNormal((B) + (rotation * BN));
        renderBuffers->addTextureMapIndex(flattenStrides[triCount]);
        triCount++;

        Vector4 C(vertices[indices[triCount]].getx(),
            vertices[indices[triCount]].gety(),
            vertices[indices[triCount]].getz(),
            1.0);
        renderBuffers->addVertex(C); //Scale then rotate vertex

        Vector4 CN(normals[indices[triCount]].getx(),
            normals[indices[triCount]].gety(),
            normals[indices[triCount]].getz(),
            1.0);
        renderBuffers->addNormal(rotation * CN); //Scale then rotate normal
        renderBuffers->addTexture(textures[triCount]);
        renderBuffers->addDebugNormal(C);
        renderBuffers->addDebugNormal((C) + (rotation * CN));
        renderBuffers->addTextureMapIndex(flattenStrides[triCount]);
        triCount++;
    }
}

