#include <maya/MPxCommand.h>

#define kGrayscaleFlag        "-g"
#define kGrayscaleFlagLong    "-grayscale"
#define kReloadFlag           "-r"
#define kReloadFlagLong       "-reload"
#define commandName           "postColor"

class viewRenderOverridePostColorCmd : public MPxCommand
{
public:
    viewRenderOverridePostColorCmd();
    ~viewRenderOverridePostColorCmd() override;

    MStatus         doIt(const MArgList& args) override;
    static MSyntax  newSyntax();
    static void* creator();
private:
    bool            grayscaleState;
    bool            shouldReload;
};