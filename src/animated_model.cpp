#include "animated_model.h"

static inline glm::mat4 mat4_cast(const aiMatrix4x4 &m) { return glm::transpose(glm::make_mat4(&m.a1)); }
static inline glm::mat4 mat4_cast(const aiMatrix3x3 &m) { return glm::transpose(glm::make_mat3(&m.a1)); }


// a collection of objects all of the same filePath
vector<vector<AnimatedObject>> AnimatedObject::modelRegistry = {};
unsigned int AnimatedObject::uniqueObjectCount = 0;
AnimatedShader AnimatedModel::shader = AnimatedShader();

// start and end are in seconds
float playFrames(float start, float end, float current_frame) {
    if (start == 0.0f && end == 0.0f) {
        return current_frame;
    }
    return fmod(current_frame, end - start) + start;
}

AnimatedObject::AnimatedObject(const std::string &filePath) {

    // check if a registry for the filePath has been created
    bool found = false;
    bool newBatch = false;
    shared_ptr<AnimatedModel> modelReference;

    for (unsigned int i = 0; i < AnimatedObject::modelRegistry.size(); i++) {
        // get a reference to the first object of each unique filePath
        AnimatedObject* storedObject = &AnimatedObject::modelRegistry[i][0];
        // check if filePaths match
        if (storedObject->filePath == filePath && AnimatedObject::modelRegistry[i].size() < 20) {

            // reference underlying StaticModel data
            this->filePath = storedObject->filePath;
            this->model = storedObject->model;
            this->modelID = storedObject->modelID;
            this->modelMatrix = mat4(1.0);
            this->meshIsCulled = storedObject->meshIsCulled;
            this->meshIsOccluded = storedObject->meshIsOccluded;
            this->meshOcclusionQueries.resize(this->model.get()->meshes.size());
            this->uniqueID = AnimatedObject::modelRegistry[this->modelID].size();

            for (unsigned int j = 0; j < this->meshOcclusionQueries.size(); j++) {
                glGenQueries(1, &this->meshOcclusionQueries[j]);
            }
            // add it to the store of objects
            AnimatedObject::modelRegistry[this->modelID].push_back(*this);
            found = true;
            break;
        } else if (storedObject->filePath == filePath && AnimatedObject::modelRegistry[i].size() >= 20) {
            newBatch = true;
            modelReference = storedObject->model;
            break;
        }
    }
    if (!found || newBatch) {
        // object is unique, create an object store in the registry
        this->filePath = filePath;
        if (newBatch) {
            this->model = modelReference;
        } else {
            this->model = std::shared_ptr<AnimatedModel>(new AnimatedModel(filePath));
        }
        this->modelID = AnimatedObject::uniqueObjectCount++;
        this->modelMatrix = mat4(1.0);
        this->meshIsCulled.resize(this->model.get()->meshes.size());
        this->meshIsOccluded.resize(this->model.get()->meshes.size());
        this->uniqueID = 0;
        for (unsigned int j = 0; j < this->meshOcclusionQueries.size(); j++) {
            glGenQueries(1, &this->meshOcclusionQueries[j]);
        }
        AnimatedObject::modelRegistry.push_back({*this});
    }

}

AnimatedModel AnimatedObject::getModel() {
    return *(AnimatedObject::modelRegistry[this->modelID][this->uniqueID].model.get());
}

mat4 AnimatedObject::getModelMatrix() {
    return AnimatedObject::modelRegistry[this->modelID][this->uniqueID].modelMatrix;
}

int AnimatedObject::setModelMatrix(mat4 m) {
    AnimatedObject::modelRegistry.at(this->modelID).at(this->uniqueID).modelMatrix = m;
    return 0;
}

vector<mat4> AnimatedObject::getObjectTransforms(const vector<AnimatedObject> &currentObjects) {
    vector<mat4> modelTransforms = {};
    for (unsigned int i = 0; i < currentObjects.size(); i++) {
        modelTransforms.push_back(currentObjects[i].modelMatrix);
    }
    return modelTransforms;
}

int AnimatedObject::drawAllObjects(float currentFrame) {

    // for every model in registry
    for (unsigned int index = 0; index <  AnimatedObject::modelRegistry.size(); index++) {
        vector<AnimatedObject> *currentObjects = &AnimatedObject::modelRegistry[index];
        // get the modelMatrices for current objects
        vector<mat4> modelTransforms = AnimatedObject::getObjectTransforms(*currentObjects);

        // get a reference to the underlying model data
        AnimatedModel *pModel = (*currentObjects)[0].model.get();

        // for every mesh
        for (unsigned int i = 0; i < pModel->meshes.size(); i++) {
            // reset transforms
            vector<mat4> visibleModelTransforms = {};
            //for each transform
            for (unsigned int j = 0; j < modelTransforms.size(); j++) {  
                // get the current object
                AnimatedObject *object = &(*currentObjects)[j];
                // check if object mesh is either not culled or not occluded
                if (!object->meshIsCulled[i]) {
                    if (!object->meshIsOccluded[i]) {
                        pModel->getFrame(j, playFrames(object->start_frame, object->end_frame, currentFrame + object->offset));
                        visibleModelTransforms.push_back(modelTransforms[j]);
                    }
                }
            } 
            // draw only those that arent culled
            pModel->meshes[i].DrawInstanced(pModel->shader, visibleModelTransforms);
        }
    }

    return 0;
}

void AnimatedObject::setFrames(float start, float end, float offset) {
    AnimatedObject::modelRegistry[this->modelID][this->uniqueID].start_frame = start;
    AnimatedObject::modelRegistry[this->modelID][this->uniqueID].end_frame = end;
    AnimatedObject::modelRegistry[this->modelID][this->uniqueID].offset = offset;
}

int AnimatedObject::remove() {
    //AnimatedObject::modelRegistry[this->modelID].erase(AnimatedObject::modelRegistry[this->modelID].begin() + this->uniqueID);
    return -1;
}


AnimatedModel::AnimatedModel(string path) {
    loadModel(path);    
}

void AnimatedModel::processNode(aiNode *node, const aiScene *scene)
{
    m_pScene = scene;
    globalInverseTransform = inverse(mat4_cast(scene->mRootNode->mTransformation));
    meshes.reserve(scene->mNumMeshes);
    // process each mesh located at the current node
    for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        // the node object only contains indices to index the actual objects in the scene.
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));

    }
    // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

AnimatedMesh AnimatedModel::processMesh(aiMesh *mesh, const aiScene* pScene)
{
    vector<Texture> textures;
    vector<VertexTransform> vertices;
    vector<unsigned int> indices;

    // Populate the vertex attribute vectors
   // Walk through each of the mesh's vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        VertexTransform vertex;
        vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder vec3 first.
        // positions
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;
        //Positions.push_back(vector);
        // normals
        vector.x = mesh->mNormals[i].x;
        vector.y = mesh->mNormals[i].y;
        vector.z = mesh->mNormals[i].z;
        vertex.Normal = vector;
        //Normals.push_back(vector);
        // texture coordinates
        if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
        {
            vec2 vec;
            // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
            // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
            //TexCoords.push_back(vec);
        }
        else
            vertex.TexCoords = vec2(0.0f, 0.0f);
            //TexCoords.push_back(vec2(0.0f, 0.0f));
        // tangent
        vector.x = mesh->mTangents[i].x;
        vector.y = mesh->mTangents[i].y;
        vector.z = mesh->mTangents[i].z;
        //Tangent.push_back(vector);
        // bitangent
        vector.x = mesh->mBitangents[i].x;
        vector.y = mesh->mBitangents[i].y;
        vector.z = mesh->mBitangents[i].z;
        //Bitangent.push_back(vector);
        vertices.push_back(vertex);
    }

    // Populate the index buffer
    for (unsigned int j = 0 ; j < mesh->mNumFaces ; j++) {
        aiFace face = mesh->mFaces[j];
        for (unsigned int k = 0; k < face.mNumIndices; k++) {
            indices.push_back(face.mIndices[k]);
        }
    }

    aiMaterial* material = pScene->mMaterials[mesh->mMaterialIndex];

    // 1. diffuse maps
    vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    // 2. specular maps
    vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    // 3. normal maps
    std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    // 4. height maps
    std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

    AnimatedMesh m(vertices, indices, textures);

    // load bones for mesh
    loadBones(m);

    return m;
}

int AnimatedModel::getFrame(unsigned int uniqueID, float currentFrame) {
    shader.use();
    vector<mat4> Transforms;
    BoneTransform(currentFrame, Transforms);

    for (unsigned int i = 0 ; i < Transforms.size() ; i++) {
        shader.SetBoneTransform(uniqueID, i, Transforms[i]);
    }
    return 0;
}

int AnimatedModel::loadModel(string path) {

    m_Importer = new Assimp::Importer();

    // read file via ASSIMP
    const aiScene* scene = m_Importer->ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    // check for errors
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
    {
        cout << "ERROR::ASSIMP:: " << m_Importer->GetErrorString() << endl;
        return 0;
    }
    // retrieve the directory path of the filepath
    directory = path.substr(0, path.find_last_of('/'));

    // process ASSIMP's root node recursively
    processNode(scene->mRootNode, scene);


    const aiAnimation* pAnimation = m_pScene->mAnimations[animationIndex];
    storeNodeAnim(pAnimation);

    AnimatedModel::shader.setup("./shaders/animated_model_shader.vs","./shaders/animated_model_shader.fs");

    for (unsigned int i = 0; i < this->meshes.size(); i++) {
        vector<float> AABB =  this->meshes[i].getAABB();
        for (unsigned int i = 0; i < AABB.size(); i++) {
        }
        glm::vec3 min = glm::vec3(AABB[0], AABB[1], AABB[2]);
        glm::vec3 max = glm::vec3(AABB[3], AABB[4], AABB[5]);
        this->meshes[i].boundingCube.setup(min * 2.0f, max * 2.0f);
    }

    model = mat4(1.0);
    view = mat4(1.0);
    useCustomViewMatrix = false;
    start_frame = 0.0f;
    end_frame = 0.0f;


    return 0;
}

void AnimatedModel::loadBones(AnimatedMesh &mesh)
{
        aiMesh* pMesh = m_pScene->mMeshes[meshes.size()];
        for (unsigned int i = 0 ; i < pMesh->mNumBones ; i++) {
          unsigned int BoneIndex = 0;
          string BoneName(pMesh->mBones[i]->mName.data);

          if (m_BoneMapping.find(BoneName) == m_BoneMapping.end()) {
              // Allocate an index for a new bone
              BoneIndex = m_NumBones;
              m_NumBones++;
              BoneInfo bi;
              m_BoneInfo.push_back(bi);
              m_BoneInfo[BoneIndex].BoneOffset = mat4_cast(pMesh->mBones[i]->mOffsetMatrix);
              m_BoneMapping[BoneName] = BoneIndex;
          }
          else {
              BoneIndex = m_BoneMapping[BoneName];
          }
          for (unsigned int j = 0 ; j < pMesh->mBones[i]->mNumWeights ; j++) {
              unsigned int VertexID = pMesh->mBones[i]->mWeights[j].mVertexId;
              float Weight  = pMesh->mBones[i]->mWeights[j].mWeight;
              mesh.vertices[VertexID].Bones.AddBoneData(BoneIndex, Weight);
          }
        }
}

void AnimatedModel::ReadNodeHeirarchy(float AnimationTime, const aiNode* pNode, mat4 ParentTransform)
{
    string NodeName(pNode->mName.data);

    const aiAnimation* pAnimation = m_pScene->mAnimations[animationIndex];

    mat4 NodeTransformation =  mat4_cast(pNode->mTransformation);

    const aiNodeAnim* pNodeAnim = FindNodeAnim(pAnimation, NodeName);

    if (pNodeAnim) {
        // Interpolate scaling and generate scaling transformation matrix
        aiVector3D Scaling;
        CalcInterpolatedScaling(Scaling, AnimationTime, pNodeAnim);
        mat4 ScalingM(1.0f);
        scale(ScalingM, vec3(Scaling.x, Scaling.y, Scaling.z));


        // Interpolate rotation and generate rotation transformation matrix
        aiQuaternion RotationQ;
        CalcInterpolatedRotation(RotationQ, AnimationTime, pNodeAnim);
        mat4 RotationM = mat4_cast(RotationQ.GetMatrix());

        // Interpolate translation and generate translation transformation matrix
        aiVector3D Translation;
        CalcInterpolatedPosition(Translation, AnimationTime, pNodeAnim);
        mat4 TranslationM(1.0f);
        TranslationM = glm::translate(TranslationM, vec3(Translation.x, Translation.y, Translation.z));

        NodeTransformation = TranslationM * RotationM * ScalingM;
    }
    mat4 GlobalTransformation = ParentTransform * NodeTransformation;

    if (m_BoneMapping.find(NodeName) != m_BoneMapping.end()) {
        unsigned int BoneIndex = m_BoneMapping[NodeName];
        mat4 BoneOffset, FinalTransformation;
        BoneOffset = m_BoneInfo[BoneIndex].BoneOffset;
        FinalTransformation = globalInverseTransform * GlobalTransformation * BoneOffset;
        m_BoneInfo[BoneIndex].FinalTransformation = FinalTransformation;
    }

    for (unsigned int i = 0 ; i < pNode->mNumChildren ; i++) {
        ReadNodeHeirarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation);
    }
}
// start and end frames in seconds, spliced from the animation file
void AnimatedModel::setFrames(float start, float end) {
    this->start_frame = start;
    this->end_frame = end;
}

void AnimatedModel::BoneTransform(float TimeInSeconds, vector<mat4>& Transforms)
{
    mat4 Identity(1.0f);
    std::cout << m_pScene->mAnimations[animationIndex]->mTicksPerSecond << std::endl;
    float TicksPerSecond = (float)(m_pScene->mAnimations[animationIndex]->mTicksPerSecond != 0 ? m_pScene->mAnimations[animationIndex]->mTicksPerSecond : 25.0f);
    float TimeInTicks = TimeInSeconds * TicksPerSecond;
    float AnimationTime = fmod(TimeInTicks, (float)m_pScene->mAnimations[animationIndex]->mDuration);

    ReadNodeHeirarchy(AnimationTime, m_pScene->mRootNode, Identity);

    Transforms.resize(m_NumBones);

    for (unsigned int i = 0 ; i < m_NumBones ; i++) {
        Transforms[i] = m_BoneInfo[i].FinalTransformation;
    }
}
// You can store the aiNodeAnim's in a map and use that to look them up instead of your FindNodeAnim function for every node every frame. I don't have an FPS counter set up but my profiler said that function was consuming most of my CPU and by using a map the animation went from mildly choppy to silky smooth. I do realize this is just meant to be illustrative and educational code.

void AnimatedModel::storeNodeAnim(const aiAnimation* pAnimation) {

    for (unsigned int i = 0 ; i < pAnimation->mNumChannels ; i++) {
        const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];

        nodeAnimMap.insert(std::make_pair(string(pNodeAnim->mNodeName.data), pNodeAnim));
    }
}
const aiNodeAnim* AnimatedModel::FindNodeAnim(const aiAnimation* pAnimation, const string NodeName)
{
    if (nodeAnimMap.find(NodeName) != nodeAnimMap.end()) {
        return nodeAnimMap[NodeName];
    }
    // for (unsigned int i = 0 ; i < pAnimation->mNumChannels ; i++) {
    //     const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];

    //     if (string(pNodeAnim->mNodeName.data) == NodeName) {
    //         return pNodeAnim;
    //     }
    // }

    return NULL;
}
// const aiNodeAnim* AnimatedModel::FindNodeAnim(const aiAnimation* pAnimation, const string NodeName)
// {
//     for (unsigned int i = 0 ; i < pAnimation->mNumChannels ; i++) {
//         const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];

//         if (string(pNodeAnim->mNodeName.data) == NodeName) {
//             return pNodeAnim;
//         }
//     }

//     return NULL;
// }

void AnimatedModel::CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    if (pNodeAnim->mNumPositionKeys == 1) {
        Out = pNodeAnim->mPositionKeys[0].mValue;
        return;
    }

    unsigned int PositionIndex = FindPosition(AnimationTime, pNodeAnim);
    unsigned int NextPositionIndex = (PositionIndex + 1);
    assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
    float DeltaTime = (float)(pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime);
    float Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
    //assert(Factor >= 0.0f && Factor <= 1.0f);
    const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
    const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
    aiVector3D Delta = End - Start;
    Out = Start + Factor * Delta;
}


void AnimatedModel::CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumRotationKeys == 1) {
        Out = pNodeAnim->mRotationKeys[0].mValue;
        return;
    }

    unsigned int RotationIndex = FindRotation(AnimationTime, pNodeAnim);
    unsigned int NextRotationIndex = (RotationIndex + 1);
    assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
    float DeltaTime = (float)(pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime);
    float Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
    //assert(Factor >= 0.0f && Factor <= 1.0f);
    const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
    const aiQuaternion& EndRotationQ   = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;
    aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);
    Out = Out.Normalize();
}


void AnimatedModel::CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    if (pNodeAnim->mNumScalingKeys == 1) {
        Out = pNodeAnim->mScalingKeys[0].mValue;
        return;
    }

    unsigned int ScalingIndex = FindScaling(AnimationTime, pNodeAnim);
    unsigned int NextScalingIndex = (ScalingIndex + 1);
    assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
    float DeltaTime = (float)(pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime);
    float Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
    //assert(Factor >= 0.0f && Factor <= 1.0f);
    const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
    const aiVector3D& End   = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
    aiVector3D Delta = End - Start;
    Out = Start + Factor * Delta;
}

unsigned int AnimatedModel::FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    for (unsigned int i = 0 ; i < pNodeAnim->mNumPositionKeys - 1 ; i++) {
        if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime) {
            return i;
        }
    }

    //assert(0);

    return 0;
}


unsigned int AnimatedModel::FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    assert(pNodeAnim->mNumRotationKeys > 0);

    for (unsigned int i = 0 ; i < pNodeAnim->mNumRotationKeys - 1 ; i++) {
        if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime) {
            return i;
        }
    }

    //assert(0);

    return 0;
}


unsigned int AnimatedModel::FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    assert(pNodeAnim->mNumScalingKeys > 0);

    for (unsigned int i = 0 ; i < pNodeAnim->mNumScalingKeys - 1 ; i++) {
        if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime) {
            return i;
        }
    }

    //assert(0);

    return 0;
}

AnimatedModel::~AnimatedModel() {
        delete m_Importer;
}





