#include "viewRenderOverridePostColor.h"
#include <maya/MGlobal.h>
#include <maya/MArgDatabase.h>
#include <maya/MSyntax.h>
#include <maya/MPxCommand.h>

// Assuming your command class is named viewRenderOverridePostColorCmd
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
};

// Define flag constants safely
const char* viewRenderOverridePostColorCmd::kReloadShortFlag = "-rl";
const char* viewRenderOverridePostColorCmd::kReloadLongFlag = "-reload";

void* viewRenderOverridePostColorCmd::creator()
{
    return new viewRenderOverridePostColorCmd();
}

// Register the strict syntax constraints for the Autodesk flag parser
MSyntax viewRenderOverridePostColorCmd::newSyntax()
{
    MSyntax syntax;

    // Register the flag. It doesn't require extra arguments (like strings or ints), it's a simple trigger switch.
    syntax.addFlag(kReloadShortFlag, kReloadLongFlag, MSyntax::kNoArg);

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

    if (argData.isFlagSet(kReloadLongFlag))
    {
        MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
        if (renderer)
        {
            // 1. Fetch the read-only override pointer from the Maya API
            const MHWRender::MRenderOverride* baseOverride = renderer->findRenderOverride("ColorPostProcessOverride");

            if (baseOverride)
            {
                // 2. FIXED: Cast away constness from the base pointer first to eliminate E1086
                MHWRender::MRenderOverride* mutableOverride = const_cast<MHWRender::MRenderOverride*>(baseOverride);

                // 3. Perform a clean, mutable dynamic cast
                ColorPostProcessOverride* ourOverride = dynamic_cast<ColorPostProcessOverride*>(mutableOverride);
                if (ourOverride)
                {
                    // Now completely authorized to invoke the reload tracking function
                    ourOverride->triggerShaderReload();
                    MGlobal::displayInfo("Post-process shader pyramid recompiled live successfully.");
                    return MStatus::kSuccess;
                }
            }

            MGlobal::displayError("ColorPostProcessOverride pipeline is not actively loaded or registered.");
            return MStatus::kFailure;
        }

        return MStatus::kFailure;
    }

    MGlobal::displayInfo("postColor command executed without modifying shader tracks.");
    return MStatus::kSuccess;
}