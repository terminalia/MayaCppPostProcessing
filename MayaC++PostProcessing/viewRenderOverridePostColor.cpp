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

    // Get the handle of the module containing this current function
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)&getPluginDirectory, &hm))
    {
        GetModuleFileNameA(hm, path, sizeof(path));

        MString fullPath(path);
        // Find the last backslash to isolate the directory path
        int lastSlash = fullPath.rindexW('\\');
        if (lastSlash != -1)
        {
            MString dir = fullPath.substringW(0, lastSlash);

            // CRITICAL FOR MAYA 2026: Convert all backslashes '\' to forward slashes '/'
            dir.substitute("\\", "/");
            return dir;
        }
    }
    return "";
}


ColorPostProcessOverride::ColorPostProcessOverride(const MString& name)
    : MRenderOverride(name)
    , mUIName("Color Post Grayscale")
{
    MHWRender::MRenderer* theRenderer = MHWRender::MRenderer::theRenderer();
    if (!theRenderer) return;

    // Fetch the standard pipeline layout
    MHWRender::MRenderer::theRenderer()->getStandardViewportOperations(mOperations);

    // Dynamic path handling or hardcoded fallback for Maya 2026 DX11 
    // Ensure you use forward slashes (/) for the absolute path path layout!
    //MString fxPath = "C:/MayaSDK/shaders/GrayscaleEffect.fx";
    MString pluginDir = getPluginDirectory();
    
    //GRAYSCALE
    MString fxPath = pluginDir + "/shaders/GrayscaleEffect.fx";
    MString techniqueName = "GrayscaleTech";
    PostQuadRender* grayscaleOp = new PostQuadRender(kGrayscalePassName, fxPath, techniqueName);
    grayscaleOp->setEnabled(false); 
    mOperations.insertAfter(MHWRender::MRenderOperation::kStandardSceneName, grayscaleOp);

    //BLOOM
    MString bloomPath = pluginDir + "/shaders/BloomEffect.fx";
    PostQuadRender* bloomOp = new PostQuadRender(kBloomPassName, bloomPath, "BloomTech");
    bloomOp->setEnabled(true); // Enabled by default
    mOperations.insertAfter(kGrayscalePassName, bloomOp);
}

ColorPostProcessOverride::~ColorPostProcessOverride() {}

MHWRender::DrawAPI ColorPostProcessOverride::supportedDrawAPIs() const
{
    return MHWRender::kDirectX11; // Explicitly locked to DX11 execution for modern Windows workflows
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

    // --- GENERATE TRULY UNIQUE CACHE-BUSTING FILE ON DISK ---
    int dotIndex = mOriginalFxFilePath.rindexW('.');
    if (dotIndex != -1)
    {
        MString baseName = mOriginalFxFilePath.substringW(0, dotIndex - 1);
        MString ext = mOriginalFxFilePath.substringW(dotIndex, mOriginalFxFilePath.length() - 1);

        // Use the Windows tick count timestamp to ensure absolute uniqueness
        unsigned long long timestamp = GetTickCount64();
        MString timeStr;
        timeStr.set((double)timestamp); // safely format into string

        MString newCopyPath = baseName + "_temp_" + timeStr + ext;

        // 1. Copy your freshly modified shader code to the new temporary destination
        if (CopyFileA(mOriginalFxFilePath.asChar(), newCopyPath.asChar(), FALSE))
        {
            // 2. Delete the old temporary file to avoid leaving garbage behind
            if (mFxFilePath != mOriginalFxFilePath)
            {
                DeleteFileA(mFxFilePath.asChar());
            }

            // 3. Swap the active filepath pointer to the new unique path name
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
                // Maya 2026 demands absolute path parameters to external files
                //mShaderInstance = shaderMgr->getEffectsFileShader(mFxFilePath.asChar(), mTechniqueName.asChar());
                mShaderInstance = shaderMgr->getEffectsFileShader(
                    mFxFilePath.asChar(),
                    mTechniqueName.asChar(),
                    NULL // Macros are no longer needed since the filepath itself is unique
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
    }

    return mShaderInstance;
}

bool PostQuadRender::getInputTargetDescription(const MString& name, MHWRender::MRenderTargetDescription& description)
{
    if (name == kColorTargetName)
    {
        MHWRender::MRenderTarget* outTarget = getInputTarget(kAuxiliaryTargetName);
        if (outTarget) outTarget->targetDescription(description);
        description.setName("_post_grayscale_target");
        return true;
    }
    else if (name == kDepthTargetName)
    {
        MHWRender::MRenderTarget* outTarget = getInputTarget(kAuxiliaryDepthTargetName);
        if (outTarget) outTarget->targetDescription(description);
        description.setName("_post_grayscale_depth");
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