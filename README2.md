[TO BE EDIT]

-IceRender-

Folder explaining:
- Sources (main codes of render)
- External (glad, glfw, glm and etc.)
- Resources (images, model files, and etc.)
	- Shaders: use the ***same name*** for vertex shader and fragment shader. **IceRender** will its relative path to `Resources` folder as shader name. E.g. name **"simple"** wich has relative path to `Resources` folder **`Resources\Shaders\Simple\simple`**, should have two shaders: `Resources\Shaders\Simple\simple.vs`, `Resources\Shaders\Simple\simple.fs`. 
---------------------------------------------------------------------------------------------------------------------
Library Used:
- OpenGL version 4.5
- glad
- glfw
- glm
- stb_image.h, stb_image_write.h
- [nlohmann-json](https://json.nlohmann.me/api/basic_json/)
---------------------------------------------------------------------------------------------------------------------
CMake build:
- used VS2019
---------------------------------------------------------------------------------------------------------------------
Keyboard Mapping:
- **F1**: print help information
- **F2**: reset camera position, rotation
- **F3**: execute previous command
- **\`**: enable to enter command in console
- **Up/Down/Left/Right**: move camera, **Q/E**: rise/descend camera, **W/S/A/D**: rotate camera, **Z/X**: zoom in/out camera, **F2** to reset camera's transformation.
---------------------------------------------------------------------------------------------------------------------
