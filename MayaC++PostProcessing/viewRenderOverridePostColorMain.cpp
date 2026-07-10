#include <maya/MIOStream.h>
#include <cstdlib>
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h> // <-- ADD THIS INCLUDE
#include "viewRenderOverridePostColor.h"
#include "viewRenderOverridePostColorCmd.h"

MStatus initializePlugin(MObject obj)
{
    MFnPlugin plugin(obj, "CustomDeveloper", "1.0", "Any");
    MStatus status = MStatus::kSuccess;

    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if (renderer)
    {
        // --- ADD LOGGING FOR THE ACTIVE VIEWPORT 2.0 API ---
        MString apiName = "Unknown API";
        switch (renderer->drawAPI())
        {
        case MHWRender::kDirectX11:
            apiName = "DirectX 11";
            break;
        case MHWRender::kOpenGLCoreProfile:
            apiName = "OpenGL Core Profile (Strict)";
            break;
        default:
            apiName = "Other/Unsupported Profile";
            break;
        }
        MGlobal::displayInfo("[MistworkPostFX] MistworkPostFX Loaded successfully. Viewport 2.0 Active API: " + apiName);
        // --------------------------------------------------

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
        MGlobal::displayInfo("[PostColorPlugin] Unloaded successfully.");
    }
    return status;
}