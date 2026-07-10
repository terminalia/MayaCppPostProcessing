#include <maya/MIOStream.h>
#include <cstdlib>
#include <maya/MFnPlugin.h>
#include "viewRenderOverridePostColor.h"
#include "viewRenderOverridePostColorCmd.h"

MStatus initializePlugin(MObject obj)
{
    MFnPlugin plugin(obj, "CustomDeveloper", "1.0", "Any");
    MStatus status = MStatus::kSuccess;

    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if (renderer)
    {
        ColorPostProcessOverride* overridePtr = new ColorPostProcessOverride("ColorPostProcessOverride");
        if (overridePtr)
        {
            renderer->registerOverride(overridePtr);
        }

        plugin.registerCommand(commandName,
            viewRenderOverridePostColorCmd::creator,
            viewRenderOverridePostColorCmd::newSyntax);
    }
    return status;
}

MStatus uninitializePlugin(MObject obj)
{
    MStatus status = MStatus::kSuccess;
    MFnPlugin plugin(obj);

    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if (renderer)
    {
        const MHWRender::MRenderOverride* overridePtr = renderer->findRenderOverride("ColorPostProcessOverride");
        if (overridePtr)
        {
            renderer->deregisterOverride(overridePtr);
            delete overridePtr;
        }
        plugin.deregisterCommand(commandName);
    }
    return status;
}