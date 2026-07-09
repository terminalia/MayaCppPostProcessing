# Maya C++ Post Processing

## Setup
### Download Maya 2026 SDK
1. Download Maya 2026 SDK
2. Uncompress the zip 
3. Inside there is a folder called **devkitBase**. Rename the folder to MayaSDK and move it to C:\

### Compile the Visual Studio Project
If you set the Maya 2026 SDK as shown before, Visual Studio should find the include and lib folders

### Copy the shaders folder to where the plugin resides
Copy the **shaders** folder to where the plugin resides (e.g x64 folder)

### Load the plugin in Maya
1. Go to **Windows** -> **Settings/Preferences** -> **Plug-in Manager**
2. Click on **Browse** and select the compiled plugin to load (.mll)

3. Ensure that **Loaded** is checked

### Add MEL script to control bloom intensity and glow trails via UI
1. Go to **Windows** -> **General Editors** -> **Script Editor**

2. Inside the **MEL** folder in the repository you can find the **PostProcessUI** code. Copy the code and paste it inside the **Script Editor**

3. Select all the code in the script editor and with middle-mouse pressed drag it to a shelf so it can be opened by clicking on an icon