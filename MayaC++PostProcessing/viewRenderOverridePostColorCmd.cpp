#include <maya/MSyntax.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/M3dView.h>

#include "viewRenderOverridePostColorCmd.h"
#include "viewRenderOverridePostColor.h"

viewRenderOverridePostColorCmd::viewRenderOverridePostColorCmd()
    : grayscaleState(false), bloomState(true), intensityVal(1.5), glowTrailVal(1.0), shouldReload(false) {
}
viewRenderOverridePostColorCmd::~viewRenderOverridePostColorCmd() {}

void* viewRenderOverridePostColorCmd::creator() { return (void*)(new viewRenderOverridePostColorCmd); }

MSyntax viewRenderOverridePostColorCmd::newSyntax()
{
    MSyntax syntax;
    syntax.addFlag(kGrayscaleFlag, kGrayscaleFlagLong, MSyntax::kBoolean);
    syntax.addFlag(kBloomFlag, kBloomFlagLong, MSyntax::kBoolean);
    syntax.addFlag(kIntensityFlag, kIntensityFlagLong, MSyntax::kDouble);
    syntax.addFlag(kGlowTrailFlag, kGlowTrailFlagLong, MSyntax::kDouble);
    syntax.addFlag(kReloadFlag, kReloadFlagLong, MSyntax::kNoArg);
    syntax.enableQuery(true);
    return syntax;
}

MStatus viewRenderOverridePostColorCmd::doIt(const MArgList& args)
{
    MStatus status = MStatus::kFailure;
    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer) return status;

    ColorPostProcessOverride* postColorOverride = (ColorPostProcessOverride*)renderer->findRenderOverride("ColorPostProcessOverride");
    if (!postColorOverride) return status;

    MArgDatabase argData(syntax(), args, &status);
    if (!status) return status;

    // --- RELOAD HANDLER ---
    if (argData.isFlagSet(kReloadFlag)) {
        for (int i = 0; i < postColorOverride->mOperations.length(); ++i) {
            PostQuadRender* quadOp = dynamic_cast<PostQuadRender*>(postColorOverride->mOperations[i]);
            if (quadOp) quadOp->releaseCustomShader();
        }
        MGlobal::displayInfo("MultiPass Bloom shader pyramid recompiled successfully.");
    }

    bool isQuery = argData.isQuery();

    // --- BLOOM STATE CONTROL ---
    if (argData.isFlagSet(kBloomFlag)) {
        int idx = postColorOverride->mOperations.indexOf(ColorPostProcessOverride::kDownsample1PassName);
        if (idx != -1) {
            if (isQuery) {
                MPxCommand::setResult(postColorOverride->mOperations[idx]->enabled());
            }
            else {
                argData.getFlagArgument(kBloomFlag, 0, bloomState);
                // Toggle all pyramid stages
                const MString passes[] = { ColorPostProcessOverride::kDownsample1PassName, ColorPostProcessOverride::kDownsample2PassName, ColorPostProcessOverride::kDownsample3PassName, ColorPostProcessOverride::kUpsample1PassName, ColorPostProcessOverride::kUpsample2PassName, ColorPostProcessOverride::kFinalCompositePassName };
                for (const auto& p : passes) {
                    int pIdx = postColorOverride->mOperations.indexOf(p);
                    if (pIdx != -1) postColorOverride->mOperations[pIdx]->setEnabled(bloomState);
                }
            }
        }
    }

    // --- INTENSITY CONTROL ---
    if (argData.isFlagSet(kIntensityFlag)) {
        int idx = postColorOverride->mOperations.indexOf(ColorPostProcessOverride::kFinalCompositePassName);
        if (idx != -1) {
            PostQuadRender* compOp = dynamic_cast<PostQuadRender*>(postColorOverride->mOperations[idx]);
            if (compOp) {
                if (isQuery) {
                    MPxCommand::setResult(compOp->intensity());
                }
                else {
                    argData.getFlagArgument(kIntensityFlag, 0, intensityVal);
                    compOp->setIntensity((float)intensityVal);
                }
            }
        }
    }

    // --- GLOW TRAIL / RADIUS CONTROL ---
    if (argData.isFlagSet(kGlowTrailFlag)) {
        int idx = postColorOverride->mOperations.indexOf(ColorPostProcessOverride::kDownsample1PassName);
        if (idx != -1) {
            PostQuadRender* sampleOp = dynamic_cast<PostQuadRender*>(postColorOverride->mOperations[idx]);
            if (sampleOp) {
                if (isQuery) {
                    MPxCommand::setResult(sampleOp->glowTrail());
                }
                else {
                    argData.getFlagArgument(kGlowTrailFlag, 0, glowTrailVal);
                    // Distribute radius adjustments evenly across all operation targets
                    for (int i = 0; i < postColorOverride->mOperations.length(); ++i) {
                        PostQuadRender* op = dynamic_cast<PostQuadRender*>(postColorOverride->mOperations[i]);
                        if (op) op->setGlowTrail((float)glowTrailVal);
                    }
                }
            }
        }
    }

    M3dView view = M3dView::active3dView(&status);
    if (status) view.refresh(false, true);

    return MStatus::kSuccess;
}