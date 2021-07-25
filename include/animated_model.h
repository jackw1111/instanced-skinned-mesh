#ifndef ANIMATED_MODEL_H
#define ANIMATED_MODEL_H

#include "animated_mesh.h"
#include "static_model.h"
#include "animated_shader.h"
///@brief loads a skinned model from Blender FBX
/// @note to export skinned models from Blender properly: export as FBX,
/// select all meshes and bone collection and apply "selected only",
/// leave all armature settings as ticked
/// when adding normal maps
/// be sure to use node>vector>normal map
/// and feed the output into normal on the principle bsdf node
class AnimatedModel : public StaticModel {
public:

    AnimatedModel(string path);
    ~AnimatedModel();
    int loadModel(string path);

    void BoneTransform(float TimeInSeconds, vector<mat4>& Transforms);

    int getFrame(unsigned int uniqueID, float currentFrame);
    void setFrames(float start, float end);

    AnimatedMesh processMesh(aiMesh *mesh, const aiScene* pScene);
    void processNode(aiNode *node, const aiScene *scene);

    const aiScene* m_pScene = NULL;
    map<string,unsigned int> m_BoneMapping; // maps a bone name to its index
    string m_FileName;
    std::vector<AnimatedMesh> meshes = {};
    vector<Texture> textures;

    vector<VertexTransform> vertices;
    vector<BoneData> Bones;
    vector<unsigned int> indices;
    bool gammaCorrection;
    mat4 globalInverseTransform;
    static AnimatedShader shader;
    Assimp::Importer *m_Importer;

    float start_frame = 0.0f;
    float end_frame = 0.0f;

    struct BoneInfo
    {
        mat4 BoneOffset;
        mat4 FinalTransformation;

        BoneInfo()
        {
            //BoneOffset.SetZero();
            //FinalTransformation.SetZero();
        }
    };
    vector<BoneInfo> m_BoneInfo;

    unsigned int m_NumBones = 0;

    unsigned int animationIndex = 0;

    unsigned int NumBones() const
    {
        return m_NumBones;
    }

private:
    std::map<std::string, const aiNodeAnim*> nodeAnimMap;

    void loadBones(AnimatedMesh &mesh);
    void ReadNodeHeirarchy(float AnimationTime, const aiNode* pNode, mat4 ParentTransform);

    void storeNodeAnim(const aiAnimation* pAnimation);
    const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, const string NodeName);
    void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    unsigned int FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
    unsigned int FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
    unsigned int FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
};

class AnimatedObject {

public:
    // interface
    AnimatedObject(const std::string &filePath);
    int remove();
    mat4 modelMatrix;
    int setModelMatrix(mat4 m);
    mat4 getModelMatrix();
    // remove this access func asap
    AnimatedModel getModel();

    static int drawAllObjects(float currentFrame);
    static vector<vector<AnimatedObject>> modelRegistry;
    std::shared_ptr<AnimatedModel> model;
    std::vector<unsigned short> meshIsCulled = {};
    std::vector<unsigned short> meshIsOccluded = {};
    std::vector<unsigned int> meshOcclusionQueries = {};
    unsigned int modelID;
    unsigned int uniqueID;
    std::string filePath;
    static unsigned int uniqueObjectCount;


    float start_frame = 0.0f;
    float end_frame = 0.0f;
    float offset = 0.0f;

    void setFrames(float start, float end, float offset);


private:

    static vector<mat4> getObjectTransforms(const vector<AnimatedObject> &objectStore);
};


#endif // ANIMATED_MODEL_H
