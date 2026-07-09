#include <maya/MSyntax.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/M3dView.h>

#include "viewRenderOverridePostColorCmd.h"
#include "viewRenderOverridePostColor.h"

viewRenderOverridePostColorCmd::viewRenderOverridePostColorCmd() : grayscaleState(true) {}
viewRenderOverridePostColorCmd::~viewRenderOverridePostColorCmd() {}

void* viewRenderOverridePostColorCmd::creator()
{
    return (void*)(new viewRenderOverridePostColorCmd);
}

MSyntax viewRenderOverridePostColorCmd::newSyntax()
{
    MSyntax syntax;
    syntax.addFlag(kGrayscaleFlag, kGrayscaleFlagLong, MSyntax::kBoolean);
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

    bool isQuery = argData.isQuery();

    if (argData.isFlagSet(kGrayscaleFlag))
    {
        int index = postColorOverride->mOperations.indexOf(ColorPostProcessOverride::kGrayscalePassName);
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

    M3dView view = M3dView::active3dView(&status);
    if (status)
    {
        view.refresh(false, true);
    }

    return MStatus::kSuccess;
}