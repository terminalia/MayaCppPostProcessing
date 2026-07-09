#include "viewRenderOverridePostColor.h"
#include <maya/MShaderManager.h>
#include <maya/MGlobal.h>
#include <Windows.h>
#include <maya/MString.h>

const MString ColorPostProcessOverride::kGrayscalePassName = "ColorPostProcessOverride_Grayscale";
const MString ColorPostProcessOverride::kBloomPassName = "ColorPostProcessOverride_Bloom";

MString getPluginDirectory()
{
    char path[256];
    HMODULE hm = NULL;

    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)&getPluginDirectory, &hm))
    {
        GetModuleFileNameA(hm, path, sizeof(path));

        MString fullPath(path);
        int lastSlash = fullPath.rindexW('\\');
        if (lastSlash != -1)
        {
            MString dir = fullPath.substringW(0, lastSlash);
            dir.substitute("\\", "/");
            return dir;
        }
    }
    return "";
}

ColorPostProcessOverride::ColorPostProcessOverride(const MString& name)
    : MRenderOverride(name)
    , mUIName("Mistwork Post FX")
{
    MHWRender::MRenderer* theRenderer = MHWRender::MRenderer::theRenderer();
    if (!theRenderer) return;

    MHWRender::MRenderer::theRenderer()->getStandardViewportOperations(mOperations);

    MString pluginDir = getPluginDirectory();

    // 1. Setup Grayscale Pass (Disabled by default)
    MString grayscalePath = pluginDir + "/shaders/GrayscaleEffect.fx";
    PostQuadRender* grayscaleOp = new PostQuadRender(kGrayscalePassName, grayscalePath, "GrayscaleTech");
    grayscaleOp->setEnabled(false);
    mOperations.insertAfter(MHWRender::MRenderOperation::kStandardSceneName, grayscaleOp);

    // 2. Setup Bloom Pass (Enabled by default)
    MString bloomPath = pluginDir + "/shaders/BloomEffect.fx";
    PostQuadRender* bloomOp = new PostQuadRender(kBloomPassName, bloomPath, "BloomTech");
    bloomOp->setEnabled(true);
    mOperations.insertAfter(kGrayscalePassName, bloomOp);
}

ColorPostProcessOverride::~ColorPostProcessOverride() {}

MHWRender::DrawAPI ColorPostProcessOverride::supportedDrawAPIs() const
{
    return MHWRender::kDirectX11;
}

MStatus ColorPostProcessOverride::setup(const MString& destination)
{
    return MRenderOverride::setup(destination);
}

MStatus ColorPostProcessOverride::cleanup()
{
    return MRenderOverride::cleanup();
}

// --- PostQuadRender Implementation ---

PostQuadRender::PostQuadRender(const MString& name, const MString& fxFilePath, const MString& technique)
    : MQuadRender(name)
    , mShaderInstance(NULL)
    , mOriginalFxFilePath(fxFilePath)
    , mFxFilePath(fxFilePath)
    , mTechniqueName(technique)
    , mIntensity(1.5f)
    , mGlowTrail(1.8f)
{
    mInputTargetNames.clear();
    mInputTargetNames.append(kAuxiliaryTargetName);
    mInputTargetNames.append(kAuxiliaryDepthTargetName);
    mInputTargetNames.append(kColorTargetName);
    mInputTargetNames.append(kDepthTargetName);

    mOutputTargetNames.clear();
    mOutputTargetNames.append(kColorTargetName);
    mOutputTargetNames.append(kDepthTargetName);
    mOutputTargetNames.append(kAuxiliaryTargetName);
    mOutputTargetNames.append(kAuxiliaryDepthTargetName);
}

PostQuadRender::~PostQuadRender()
{
    if (mShaderInstance)
    {
        MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
        if (renderer)
        {
            const MHWRender::MShaderManager* shaderMgr = renderer->getShaderManager();
            if (shaderMgr)
            {
                shaderMgr->releaseShader(mShaderInstance);
            }
        }

        if (mFxFilePath != mOriginalFxFilePath)
        {
            DeleteFileA(mFxFilePath.asChar());
        }
        mShaderInstance = NULL;
    }
}

void PostQuadRender::releaseCustomShader()
{
    if (mShaderInstance)
    {
        MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
        if (renderer && renderer->getShaderManager())
        {
            renderer->getShaderManager()->releaseShader(mShaderInstance);
        }
        mShaderInstance = NULL;
    }

    int dotIndex = mOriginalFxFilePath.rindexW('.');
    if (dotIndex != -1)
    {
        MString baseName = mOriginalFxFilePath.substringW(0, dotIndex - 1);
        MString ext = mOriginalFxFilePath.substringW(dotIndex, mOriginalFxFilePath.length() - 1);

        unsigned long long timestamp = GetTickCount64();
        MString timeStr;
        timeStr.set((double)timestamp);

        MString newCopyPath = baseName + "_temp_" + timeStr + ext;

        if (CopyFileA(mOriginalFxFilePath.asChar(), newCopyPath.asChar(), FALSE))
        {
            if (mFxFilePath != mOriginalFxFilePath)
            {
                DeleteFileA(mFxFilePath.asChar());
            }
            mFxFilePath = newCopyPath;
        }
    }
}

const MHWRender::MShaderInstance* PostQuadRender::shader()
{
    if (mShaderInstance == NULL)
    {
        MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
        if (renderer)
        {
            const MHWRender::MShaderManager* shaderMgr = renderer->getShaderManager();
            if (shaderMgr)
            {
                mShaderInstance = shaderMgr->getEffectsFileShader(
                    mFxFilePath.asChar(),
                    mTechniqueName.asChar(),
                    NULL
                );
            }
        }
    }

    if (mShaderInstance)
    {
        MHWRender::MRenderTargetAssignment assignment;
        assignment.target = getInputTarget(kColorTargetName);

        MStatus status = mShaderInstance->setParameter("gInputTex", assignment);
        if (status != MStatus::kSuccess)
        {
            MGlobal::displayError("Failed mapping current viewport swapchain texture to 'gInputTex'.");
            return NULL;
        }

        // Forward parameters up to DX11 Effects uniforms dynamically
        mShaderInstance->setParameter("gBloomIntensity", mIntensity);
        mShaderInstance->setParameter("gGlowTrail", mGlowTrail);
    }

    return mShaderInstance;
}

bool PostQuadRender::getInputTargetDescription(const MString& name, MHWRender::MRenderTargetDescription& description)
{
    if (name == kColorTargetName)
    {
        MHWRender::MRenderTarget* outTarget = getInputTarget(kAuxiliaryTargetName);
        if (outTarget) outTarget->targetDescription(description);
        description.setName("_post_effects_target");
        return true;
    }
    else if (name == kDepthTargetName)
    {
        MHWRender::MRenderTarget* outTarget = getInputTarget(kAuxiliaryDepthTargetName);
        if (outTarget) outTarget->targetDescription(description);
        description.setName("_post_effects_depth");
        return true;
    }
    return false;
}

int PostQuadRender::writableTargets(unsigned int& count)
{
    count = 2;
    return 0;
}

MHWRender::MClearOperation& PostQuadRender::clearOperation()
{
    mClearOperation.setClearGradient(false);
    mClearOperation.setMask((unsigned int)MHWRender::MClearOperation::kClearNone);
    return mClearOperation;
}
