#ifndef ColorPostProcess_h_
#define ColorPostProcess_h_

#include <maya/MString.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MRenderTargetManager.h>

class ColorPostProcessOverride : public MHWRender::MRenderOverride
{
public:
    static const MString kGrayscalePassName;
    static const MString kBloomPassName;

    ColorPostProcessOverride(const MString& name);
    ~ColorPostProcessOverride() override;
    MHWRender::DrawAPI supportedDrawAPIs() const override;

    MStatus setup(const MString& destination) override;
    MStatus cleanup() override;

    MString uiName() const override { return mUIName; }

protected:
    MString mUIName;

    friend class viewRenderOverridePostColorCmd;
};

class PostQuadRender : public MHWRender::MQuadRender
{
public:
    PostQuadRender(const MString& name, const MString& fxFilePath, const MString& technique);
    ~PostQuadRender() override;

    const MHWRender::MShaderInstance* shader() override;
    MHWRender::MClearOperation& clearOperation() override;

    int writableTargets(unsigned int& count) override;
    bool getInputTargetDescription(const MString& name, MHWRender::MRenderTargetDescription& description) override;
    void releaseCustomShader();

protected:
    MHWRender::MShaderInstance* mShaderInstance;
    MString mOriginalFxFilePath;
    MString mFxFilePath;
    MString mTechniqueName;
};

#endif