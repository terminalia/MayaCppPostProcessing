#ifndef ColorPostProcess_h_
#define ColorPostProcess_h_

#include <maya/MString.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MRenderTargetManager.h>
#include <vector>

class PostQuadRender;
class ColorPostProcessOverride;

class PostQuadRender : public MHWRender::MQuadRender
{
public:
    PostQuadRender(const MString& name, const MString& fxFilePath, const MString& technique);
    ~PostQuadRender() override;

    const MHWRender::MShaderInstance* shader() override;
    MHWRender::MClearOperation& clearOperation() override;

    // VERIFIED SDK METHOD: Correct signature for overriding targets in an MQuadRender operation
    MHWRender::MRenderTarget* const* targetOverrideList(unsigned int& listSize) override;

    void releaseCustomShader();

    void setInputTargetPtr(MHWRender::MRenderTarget* target);
    void setSecondaryInputTargetPtr(MHWRender::MRenderTarget* target);
    void setOutputTargetPtr(MHWRender::MRenderTarget* target);
    void setClearOverride(bool clear);

    void setIntensity(float val);
    float intensity() const;
    void setGlowTrail(float val);
    float glowTrail() const;

protected:
    MHWRender::MShaderInstance* mShaderInstance;
    MString mOriginalFxFilePath;
    MString mFxFilePath;
    MString mTechniqueName;

    float mIntensity;
    float mGlowTrail;
    bool mShouldClear;

    MHWRender::MRenderTarget* mInputTargetPtr;
    MHWRender::MRenderTarget* mSecondaryInputTargetPtr;
    MHWRender::MRenderTarget* mOutputTargetPtr;
    MHWRender::MRenderTarget* mOutputTargetArray[1];
};

class ColorPostProcessOverride : public MHWRender::MRenderOverride
{
public:
    static const MString kGrayscalePassName;
    static const MString kDownsample1PassName;
    static const MString kDownsample2PassName;
    static const MString kDownsample3PassName;
    static const MString kUpsample1PassName;
    static const MString kUpsample2PassName;
    static const MString kFinalCompositePassName;

    ColorPostProcessOverride(const MString& name);
    ~ColorPostProcessOverride() override;
    MHWRender::DrawAPI supportedDrawAPIs() const override;

    MStatus setup(const MString& destination) override;
    MStatus cleanup() override;

    MString uiName() const override { return mUIName; }

    MHWRender::MRenderTarget* getTargetHalfBlur() const;

protected:
    void releaseTargets();

    MString mUIName;
    std::vector<MHWRender::MRenderOperation*> mOwnedOperations;

    MHWRender::MRenderTarget* mTargetHalf;
    MHWRender::MRenderTarget* mTargetQuarter;
    MHWRender::MRenderTarget* mTargetEighth;
    MHWRender::MRenderTarget* mTargetQuarterBlur;
    MHWRender::MRenderTarget* mTargetHalfBlur;

    friend class viewRenderOverridePostColorCmd;
};

#endif