#include "viewRenderOverridePostColor.h"
#include <maya/MGlobal.h>
#include <maya/MArgDatabase.h>
#include <maya/MSyntax.h>
#include <maya/MPxCommand.h>

class viewRenderOverridePostColorCmd : public MPxCommand
{
public:
    viewRenderOverridePostColorCmd() {}
    ~viewRenderOverridePostColorCmd() override {}

    MStatus doIt(const MArgList& args) override;

    static MSyntax newSyntax();
    static void* creator();

private:
    static const char* kReloadShortFlag;
    static const char* kReloadLongFlag;
    static const char* kIntensityShortFlag;
    static const char* kIntensityLongFlag;
    static const char* kGlowTrailShortFlag;
    static const char* kGlowTrailLongFlag;

    // FIXED: Added tracking flags for Bloom Enable state
    static const char* kBloomShortFlag;
    static const char* kBloomLongFlag;
};

const char* viewRenderOverridePostColorCmd::kReloadShortFlag = "-rl";
const char* viewRenderOverridePostColorCmd::kReloadLongFlag = "-reload";
const char* viewRenderOverridePostColorCmd::kIntensityShortFlag = "-i";
const char* viewRenderOverridePostColorCmd::kIntensityLongFlag = "-intensity";
const char* viewRenderOverridePostColorCmd::kGlowTrailShortFlag = "-gt";
const char* viewRenderOverridePostColorCmd::kGlowTrailLongFlag = "-glowTrail";
const char* viewRenderOverridePostColorCmd::kBloomShortFlag = "-b";
const char* viewRenderOverridePostColorCmd::kBloomLongFlag = "-bloom";

void* viewRenderOverridePostColorCmd::creator()
{
    return new viewRenderOverridePostColorCmd();
}

MSyntax viewRenderOverridePostColorCmd::newSyntax()
{
    MSyntax syntax;

    // Enable global query and edit switches natively in the command syntax core
    syntax.enableQuery(true);
    syntax.enableEdit(false); // We handle edits through clean direct flags

    syntax.addFlag(kReloadShortFlag, kReloadLongFlag, MSyntax::kNoArg);
    syntax.addFlag(kIntensityShortFlag, kIntensityLongFlag, MSyntax::kDouble);
    syntax.addFlag(kGlowTrailShortFlag, kGlowTrailLongFlag, MSyntax::kDouble);
    syntax.addFlag(kBloomShortFlag, kBloomLongFlag, MSyntax::kLong);

    return syntax;
}

MStatus viewRenderOverridePostColorCmd::doIt(const MArgList& args)
{
    MStatus status;

    MArgDatabase argData(syntax(), args, &status);
    if (!status) {
        MGlobal::displayError("Failed to parse command line arguments.");
        return status;
    }

    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer) return MStatus::kFailure;

    const MHWRender::MRenderOverride* baseOverride = renderer->findRenderOverride("ColorPostProcessOverride");
    if (!baseOverride) {
        MGlobal::displayError("ColorPostProcessOverride pipeline is not active.");
        return MStatus::kFailure;
    }

    ColorPostProcessOverride* ourOverride = const_cast<ColorPostProcessOverride*>(dynamic_cast<const ColorPostProcessOverride*>(baseOverride));
    if (!ourOverride) return MStatus::kFailure;

    // Grab a pointer to the first custom post pass to extract values during queries
    PostQuadRender* firstOp = nullptr;
    if (!ourOverride->mOwnedOperations.empty()) {
        // Skip index 0 (Grayscale), grab target bloom operation
        firstOp = dynamic_cast<PostQuadRender*>(ourOverride->mOwnedOperations[1]);
    }

    // --- 1. HANDLE QUERY MODE (postColor -q ...) ---
    if (argData.isQuery())
    {
        if (!firstOp) return MStatus::kFailure;

        if (argData.isFlagSet(kIntensityLongFlag)) {
            setResult(firstOp->intensity());
            return MStatus::kSuccess;
        }
        if (argData.isFlagSet(kGlowTrailLongFlag)) {
            setResult(firstOp->glowTrail());
            return MStatus::kSuccess;
        }
        if (argData.isFlagSet(kBloomLongFlag)) {
            // Check if the bloom operation pass is enabled in Viewport 2.0 graph
            setResult(firstOp->enabled());
            return MStatus::kSuccess;
        }

        MGlobal::displayError("Invalid query flag specified.");
        return MStatus::kFailure;
    }

    // --- 2. HANDLE EDIT / COMMAND INVOCATIONS ---
    if (argData.isFlagSet(kReloadLongFlag))
    {
        ourOverride->triggerShaderReload();
        return MStatus::kSuccess;
    }

    bool explicitUpdate = false;

    // Loop through operations to apply edit updates cleanly
    for (auto op : ourOverride->mOwnedOperations) {
        PostQuadRender* quadOp = dynamic_cast<PostQuadRender*>(op);
        if (!quadOp) continue;

        if (argData.isFlagSet(kIntensityLongFlag)) {
            double val;
            argData.getFlagArgument(kIntensityLongFlag, 0, val);
            quadOp->setIntensity((float)val);
            explicitUpdate = true;
        }
        if (argData.isFlagSet(kGlowTrailLongFlag)) {
            double val;
            argData.getFlagArgument(kGlowTrailLongFlag, 0, val);
            quadOp->setGlowTrail((float)val);
            explicitUpdate = true;
        }
        if (argData.isFlagSet(kBloomLongFlag)) {
            int state = 1; // FIXED: Uses matching type signature for MArgDatabase
            argData.getFlagArgument(kBloomLongFlag, 0, state);

            if (quadOp->name() != ColorPostProcessOverride::kFinalCompositePassName &&
                quadOp->name() != ColorPostProcessOverride::kGrayscalePassName) {
                quadOp->setEnabled(state != 0);
            }
            explicitUpdate = true;
        }
    }

    if (explicitUpdate) {
        MGlobal::executeCommand("refresh -f");
    }

    return MStatus::kSuccess;
}