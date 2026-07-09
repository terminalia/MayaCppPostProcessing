#include <maya/MSyntax.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/M3dView.h>

#include "viewRenderOverridePostColorCmd.h"
#include "viewRenderOverridePostColor.h"

viewRenderOverridePostColorCmd::viewRenderOverridePostColorCmd()
    : grayscaleState(false), bloomState(true), intensityVal(1.5), glowTrailVal(1.8), shouldReload(false) {
}
viewRenderOverridePostColorCmd::~viewRenderOverridePostColorCmd() {}

void* viewRenderOverridePostColorCmd::creator()
{
    return (void*)(new viewRenderOverridePostColorCmd);
}

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
    if (!renderer)
    {
        MGlobal::displayError("VP2 renderer not active.");
        return status;
    }

    ColorPostProcessOverride* postColorOverride = (ColorPostProcessOverride*)renderer->findRenderOverride("ColorPostProcessOverride");
    if (postColorOverride == NULL)
    {
        MGlobal::displayError("ColorPostProcessOverride is not registered.");
        return status;
    }

    MArgDatabase argData(syntax(), args, &status);
    if (!status) return status;

    // --- HANDLE RELOAD FLAG ---
    if (argData.isFlagSet(kReloadFlag))
    {
        int gIndex = postColorOverride->mOperations.indexOf(ColorPostProcessOverride::kGrayscalePassName);
        if (gIndex != -1)
        {
            PostQuadRender* quadOp = (PostQuadRender*)postColorOverride->mOperations[gIndex];
            if (quadOp) quadOp->releaseCustomShader();
        }

        int bIndex = postColorOverride->mOperations.indexOf(ColorPostProcessOverride::kBloomPassName);
        if (bIndex != -1)
        {
            PostQuadRender* quadOp = (PostQuadRender*)postColorOverride->mOperations[bIndex];
            if (quadOp) quadOp->releaseCustomShader();
        }
        MGlobal::displayInfo("Post-processing shader cache cleared. Recompiling next frame...");
    }

    bool isQuery = argData.isQuery();

    // --- HANDLE GRAYSCALE FLAG ---
    if (argData.isFlagSet(kGrayscaleFlag))
    {
        int index = postColorOverride->mOperations.indexOf(ColorPostProcessOverride::kGrayscalePassName);
        if (index != -1)
        {
            if (isQuery)
            {
                MPxCommand::setResult(postColorOverride->mOperations[index]->enabled());
            }
            else
            {
                argData.getFlagArgument(kGrayscaleFlag, 0, grayscaleState);
                postColorOverride->mOperations[index]->setEnabled(grayscaleState);
            }
        }
    }

    // --- HANDLE BLOOM FLAG ---
    if (argData.isFlagSet(kBloomFlag))
    {
        int index = postColorOverride->mOperations.indexOf(ColorPostProcessOverride::kBloomPassName);
        if (index != -1)
        {
            if (isQuery)
            {
                MPxCommand::setResult(postColorOverride->mOperations[index]->enabled());
            }
            else
            {
                argData.getFlagArgument(kBloomFlag, 0, bloomState);
                postColorOverride->mOperations[index]->setEnabled(bloomState);
            }
        }
    }

    // --- HANDLE INTENSITY FLAG ---
    if (argData.isFlagSet(kIntensityFlag))
    {
        int index = postColorOverride->mOperations.indexOf(ColorPostProcessOverride::kBloomPassName);
        if (index != -1)
        {
            PostQuadRender* bloomOp = (PostQuadRender*)postColorOverride->mOperations[index];
            if (bloomOp)
            {
                if (isQuery)
                {
                    MPxCommand::setResult(bloomOp->intensity());
                }
                else
                {
                    argData.getFlagArgument(kIntensityFlag, 0, intensityVal);
                    bloomOp->setIntensity((float)intensityVal);
                }
            }
        }
    }

    // --- HANDLE GLOW TRAIL FLAG ---
    if (argData.isFlagSet(kGlowTrailFlag))
    {
        int index = postColorOverride->mOperations.indexOf(ColorPostProcessOverride::kBloomPassName);
        if (index != -1)
        {
            PostQuadRender* bloomOp = (PostQuadRender*)postColorOverride->mOperations[index];
            if (bloomOp)
            {
                if (isQuery)
                {
                    MPxCommand::setResult(bloomOp->glowTrail());
                }
                else
                {
                    argData.getFlagArgument(kGlowTrailFlag, 0, glowTrailVal);
                    bloomOp->setGlowTrail((float)glowTrailVal);
                }
            }
        }
    }

    M3dView view = M3dView::active3dView(&status);
    if (status)
    {
        view.refresh(false, true);
    }

    return MStatus::kSuccess;
}