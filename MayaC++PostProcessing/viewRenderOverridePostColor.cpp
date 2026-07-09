#include "viewRenderOverridePostColor.h"
#include <maya/MShaderManager.h>
#include <maya/MGlobal.h>
#include <maya/MColor.h>
#include <Windows.h>
#include <string>

const MString ColorPostProcessOverride::kGrayscalePassName = "ColorPostProcessOverride_Grayscale";
const MString ColorPostProcessOverride::kDownsample1PassName = "ColorPostProcessOverride_Down1";
const MString ColorPostProcessOverride::kDownsample2PassName = "ColorPostProcessOverride_Down2";
const MString ColorPostProcessOverride::kDownsample3PassName = "ColorPostProcessOverride_Down3";
const MString ColorPostProcessOverride::kUpsample1PassName = "ColorPostProcessOverride_Up1";
const MString ColorPostProcessOverride::kUpsample2PassName = "ColorPostProcessOverride_Up2";
const MString ColorPostProcessOverride::kFinalCompositePassName = "ColorPostProcessOverride_FinalComposite";

MString getPluginDirectory()
{
    char path[256];
    HMODULE hm = NULL;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&getPluginDirectory, &hm)) {
        GetModuleFileNameA(hm, path, sizeof(path));
        MString fullPath(path);
        int lastSlash = fullPath.rindexW('\\');
        if (lastSlash != -1) {
            MString dir = fullPath.substringW(0, lastSlash);
            dir.substitute("\\", "/");
            return dir;
        }
    }
    return "";
}

ColorPostProcessOverride::ColorPostProcessOverride(const MString& name)
    : MRenderOverride(name)
    , mUIName("MultiPass Bloom Effects")
    , mTargetHalf(NULL), mTargetQuarter(NULL), mTargetEighth(NULL), mTargetQuarterBlur(NULL), mTargetHalfBlur(NULL)
{
    MHWRender::MRenderer* theRenderer = MHWRender::MRenderer::theRenderer();
    if (!theRenderer) return;

    theRenderer->getStandardViewportOperations(mOperations);
    MString pluginDir = getPluginDirectory();
    MString ext = (theRenderer->drawAPI() == MHWRender::kOpenGLCoreProfile) ? ".ogsfx" : ".fx";
    MString shaderPath = pluginDir + "/shaders/MultiPassBloom" + ext;

    PostQuadRender* grayscaleOp = new PostQuadRender(kGrayscalePassName, pluginDir + "/shaders/GrayscaleEffect.fx", "GrayscaleTech");
    grayscaleOp->setEnabled(false);
    mOwnedOperations.push_back(grayscaleOp);
    mOperations.insertAfter(MHWRender::MRenderOperation::kStandardSceneName, grayscaleOp);

    PostQuadRender* down1 = new PostQuadRender(kDownsample1PassName, shaderPath, "ThresholdDownsample");
    down1->setClearOverride(true);
    mOwnedOperations.push_back(down1);
    mOperations.insertAfter(kGrayscalePassName, down1);

    PostQuadRender* down2 = new PostQuadRender(kDownsample2PassName, shaderPath, "StandardDownsample");
    down2->setClearOverride(true);
    mOwnedOperations.push_back(down2);
    mOperations.insertAfter(kDownsample1PassName, down2);

    PostQuadRender* down3 = new PostQuadRender(kDownsample3PassName, shaderPath, "StandardDownsample");
    down3->setClearOverride(true);
    mOwnedOperations.push_back(down3);
    mOperations.insertAfter(kDownsample2PassName, down3);

    PostQuadRender* up1 = new PostQuadRender(kUpsample1PassName, shaderPath, "UpsampleBlend");
    up1->setClearOverride(true);
    mOwnedOperations.push_back(up1);
    mOperations.insertAfter(kDownsample3PassName, up1);

    PostQuadRender* up2 = new PostQuadRender(kUpsample2PassName, shaderPath, "UpsampleBlend");
    up2->setClearOverride(true);
    mOwnedOperations.push_back(up2);
    mOperations.insertAfter(kUpsample1PassName, up2);

    PostQuadRender* composite = new PostQuadRender(kFinalCompositePassName, shaderPath, "FinalComposite");
    mOwnedOperations.push_back(composite);
    mOperations.insertAfter(kUpsample2PassName, composite);
}

ColorPostProcessOverride::~ColorPostProcessOverride() {
    releaseTargets();
    for (auto op : mOwnedOperations) delete op;
}

MHWRender::MRenderTarget* ColorPostProcessOverride::getTargetHalfBlur() const {
    return mTargetHalfBlur;
}

void ColorPostProcessOverride::releaseTargets() {
    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer) return;
    const MHWRender::MRenderTargetManager* targetMgr = renderer->getRenderTargetManager();
    if (!targetMgr) return;

    if (mTargetHalf) { targetMgr->releaseRenderTarget(mTargetHalf); mTargetHalf = NULL; }
    if (mTargetQuarter) { targetMgr->releaseRenderTarget(mTargetQuarter); mTargetQuarter = NULL; }
    if (mTargetEighth) { targetMgr->releaseRenderTarget(mTargetEighth); mTargetEighth = NULL; }
    if (mTargetQuarterBlur) { targetMgr->releaseRenderTarget(mTargetQuarterBlur); mTargetQuarterBlur = NULL; }
    if (mTargetHalfBlur) { targetMgr->releaseRenderTarget(mTargetHalfBlur); mTargetHalfBlur = NULL; }
}

MStatus ColorPostProcessOverride::setup(const MString& dest) {
    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer) return MStatus::kFailure;
    const MHWRender::MRenderTargetManager* targetMgr = renderer->getRenderTargetManager();
    if (!targetMgr) return MStatus::kFailure;

    unsigned int w = 1920, h = 1080;
    renderer->outputTargetSize(w, h);

    releaseTargets();

    MHWRender::MRenderTargetDescription desc("_bloom_half", w / 2, h / 2, 1, MHWRender::kR16G16B16A16_FLOAT, 0, false);
    mTargetHalf = targetMgr->acquireRenderTarget(desc);

    desc.setName("_bloom_quarter"); desc.setWidth(w / 4); desc.setHeight(h / 4);
    mTargetQuarter = targetMgr->acquireRenderTarget(desc);

    desc.setName("_bloom_eighth"); desc.setWidth(w / 8); desc.setHeight(h / 8);
    mTargetEighth = targetMgr->acquireRenderTarget(desc);

    desc.setName("_bloom_quarter_blur"); desc.setWidth(w / 4); desc.setHeight(h / 4);
    mTargetQuarterBlur = targetMgr->acquireRenderTarget(desc);

    desc.setName("_bloom_half_blur"); desc.setWidth(w / 2); desc.setHeight(h / 2);
    mTargetHalfBlur = targetMgr->acquireRenderTarget(desc);

    PostQuadRender* d1 = (PostQuadRender*)mOwnedOperations[1];
    d1->setOutputTargetPtr(mTargetHalf);

    PostQuadRender* d2 = (PostQuadRender*)mOwnedOperations[2];
    d2->setInputTargetPtr(mTargetHalf);
    d2->setOutputTargetPtr(mTargetQuarter);

    PostQuadRender* d3 = (PostQuadRender*)mOwnedOperations[3];
    d3->setInputTargetPtr(mTargetQuarter);
    d3->setOutputTargetPtr(mTargetEighth);

    PostQuadRender* u1 = (PostQuadRender*)mOwnedOperations[4];
    u1->setInputTargetPtr(mTargetEighth);
    u1->setSecondaryInputTargetPtr(mTargetQuarter);
    u1->setOutputTargetPtr(mTargetQuarterBlur);

    PostQuadRender* u2 = (PostQuadRender*)mOwnedOperations[5];
    u2->setInputTargetPtr(mTargetQuarterBlur);
    u2->setSecondaryInputTargetPtr(mTargetHalf);
    u2->setOutputTargetPtr(mTargetHalfBlur);

    PostQuadRender* comp = (PostQuadRender*)mOwnedOperations[6];
    comp->setInputTargetPtr(mTargetHalfBlur);

    return MRenderOverride::setup(dest);
}

MStatus ColorPostProcessOverride::cleanup() { return MRenderOverride::cleanup(); }

MHWRender::DrawAPI ColorPostProcessOverride::supportedDrawAPIs() const {
    return (MHWRender::kDirectX11 | MHWRender::kOpenGLCoreProfile);
}

// FIXED: Implemented multi-track unified runtime cloning engine logic
void ColorPostProcessOverride::triggerShaderReload()
{
    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer) return;

    MString originalPath = getPluginDirectory() + "/shaders/MultiPassBloom";
    MString ext = (renderer->drawAPI() == MHWRender::kOpenGLCoreProfile) ? ".ogsfx" : ".fx";
    originalPath += ext;

    MString finalPathToLoad = originalPath;

    if (originalPath.toLowerCase().rindexW(".fx") || originalPath.toLowerCase().rindexW(".ogsfx"))
    {
        MString tempPath = originalPath.substringW(0, originalPath.length() - ext.length());
        tempPath += "_temp_" + MString(std::to_string(GetTickCount()).c_str()) + ext;

        if (CopyFileA(originalPath.asChar(), tempPath.asChar(), FALSE)) {
            finalPathToLoad = tempPath;
        }
    }

    // Pass track remapping across bloom pipeline operations (Skipping index 0 - Grayscale)
    for (size_t i = 1; i < mOwnedOperations.size(); ++i)
    {
        PostQuadRender* quadOp = dynamic_cast<PostQuadRender*>(mOwnedOperations[i]);
        if (quadOp)
        {
            quadOp->releaseCustomShader();
            quadOp->setShaderFilePath(finalPathToLoad);
        }
    }

    MGlobal::executeCommand("refresh -f");
}

// --- PostQuadRender Method Implementations ---

PostQuadRender::PostQuadRender(const MString& name, const MString& fxFilePath, const MString& technique)
    : MHWRender::MQuadRender(name)
    , mShaderInstance(NULL), mOriginalFxFilePath(fxFilePath), mFxFilePath(fxFilePath)
    , mTechniqueName(technique), mIntensity(1.5f), mGlowTrail(1.0f), mShouldClear(false)
    , mInputTargetPtr(NULL), mSecondaryInputTargetPtr(NULL), mOutputTargetPtr(NULL)
{
    mOutputTargetArray[0] = NULL;
}

PostQuadRender::~PostQuadRender() {
    if (mShaderInstance) {
        MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
        if (renderer && renderer->getShaderManager()) renderer->getShaderManager()->releaseShader(mShaderInstance);
    }
    // Clean up temporary tracking shader tracks to preserve folder space cleanly
    if (mFxFilePath != mOriginalFxFilePath && mFxFilePath.indexW("_temp_") != -1) {
        DeleteFileA(mFxFilePath.asChar());
    }
}

void PostQuadRender::setInputTargetPtr(MHWRender::MRenderTarget* target) { mInputTargetPtr = target; }
void PostQuadRender::setSecondaryInputTargetPtr(MHWRender::MRenderTarget* target) { mSecondaryInputTargetPtr = target; }
void PostQuadRender::setOutputTargetPtr(MHWRender::MRenderTarget* target) { mOutputTargetPtr = target; mOutputTargetArray[0] = target; }
void PostQuadRender::setShaderFilePath(const MString& path) { mFxFilePath = path; } // FIXED implementation
void PostQuadRender::setClearOverride(bool clear) { mShouldClear = clear; }
void PostQuadRender::setIntensity(float val) { mIntensity = val; }
float PostQuadRender::intensity() const { return mIntensity; }
void PostQuadRender::setGlowTrail(float val) { mGlowTrail = val; }
float PostQuadRender::glowTrail() const { return mGlowTrail; }

void PostQuadRender::releaseCustomShader() {
    if (mShaderInstance) {
        MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
        if (renderer && renderer->getShaderManager()) renderer->getShaderManager()->releaseShader(mShaderInstance);
        mShaderInstance = NULL;
    }
}

MHWRender::MRenderTarget* const* PostQuadRender::targetOverrideList(unsigned int& listSize) {
    if (mOutputTargetPtr) {
        listSize = 1;
        return mOutputTargetArray;
    }
    listSize = 0;
    return NULL;
}

const MHWRender::MShaderInstance* PostQuadRender::shader()
{
    if (mShaderInstance == NULL) {
        MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
        if (renderer && renderer->getShaderManager()) {
            mShaderInstance = renderer->getShaderManager()->getEffectsFileShader(mFxFilePath.asChar(), mTechniqueName.asChar(), NULL);
        }
    }

    if (mShaderInstance) {
        MHWRender::MRenderTargetAssignment assignment;
        if (mInputTargetPtr) {
            assignment.target = mInputTargetPtr;
        }
        else {
            assignment.target = getInputTarget(MHWRender::MRenderOperation::kColorTargetName);
        }
        mShaderInstance->setParameter("gInputTex", assignment);

        if (mSecondaryInputTargetPtr) {
            MHWRender::MRenderTargetAssignment secAssignment;
            secAssignment.target = mSecondaryInputTargetPtr;
            mShaderInstance->setParameter("gSourceMipTex", secAssignment);
        }
        else {
            MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
            if (renderer) {
                ColorPostProcessOverride* ovr = (ColorPostProcessOverride*)renderer->findRenderOverride("ColorPostProcessOverride");
                if (ovr && mInputTargetPtr == ovr->getTargetHalfBlur()) {
                    MHWRender::MRenderTargetAssignment secAssignment;
                    secAssignment.target = getInputTarget(MHWRender::MRenderOperation::kAuxiliaryTargetName);
                    mShaderInstance->setParameter("gSourceMipTex", secAssignment);
                }
            }
        }

        mShaderInstance->setParameter("gBloomIntensity", mIntensity);
        mShaderInstance->setParameter("gGlowTrail", mGlowTrail);
    }
    return mShaderInstance;
}

MHWRender::MClearOperation& PostQuadRender::clearOperation()
{
    mClearOperation.setClearGradient(false);
    mClearOperation.setMask(mShouldClear ? (unsigned int)MHWRender::MClearOperation::kClearColor : (unsigned int)MHWRender::MClearOperation::kClearNone);
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    mClearOperation.setClearColor(clearColor);
    return mClearOperation;
}