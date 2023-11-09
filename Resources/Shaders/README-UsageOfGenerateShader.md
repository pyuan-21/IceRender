# Usage for generating shaders

All shaders files end with ".fs" or ".vs":
- ".fs": fragment shader file, ".vs": vertex shader file.
- ".sub_fs" or ".sub_vs" is **SubShader** file, which can be **import** into ".main_fs" or ".main_vs" to generate final ".fs" or ".vs"
- ".main_fs" or ".main_vs" can use `#import:"sub shader path"#` to import sub shader. For example, if we have a sub shader file "refer.sub_fs" in folder "/Resources/SubShaders/", then we can write `#import:"SubShaders/refer.sub_fs"#` in a ".main_fs" file. The final output shader file must be specified.(explained below "outputs")

Using command `load_scene xxx.json` can possibly generate shader files. Here is an example for such ".json" files:
```
{
	"generate_shader": {
                        "inputs": ["Test/input.main_fs", "Test/input.main_vs"],
                        "outputs": ["Test/output.fs", "Test/output.vs"]
                       }
}
```
- "outputs" is used to specify the final shader file name.

Command for unrolling shaders: `unroll_shader`. Here's an example inside ".json" files:
```
{
  "unroll_shader": {
                        "extraParamsMap": {
                          "Examples/UnrollShader/input.fs": [[0, 2, 1], [0.0, 1, 0.1]]
                        },
                        "inputs": ["Examples/UnrollShader/input.fs"],
                        "outputs": ["Examples/UnrollShader/output.fs"]
                   }
}
```
- `extraParamsMap` is a map whose `key` is the input shader name, while the `value` is a list of 3-data(iterInit, iterMax, iterStep). More details please check [**README-UnrollShader.md**](README-UnrollShader.md)
- Normally, the input shaders should have **".iter_fs"** and output it as **".sub_fs"** or **".fs"**.

Command for replacing shaders: `replace_shader`, but it is used to replace string only. Here's an example inside ".json" files:
```
{
  "replace_shader": {
                        "string_params": ["string1", "string2"],
                        "inputs": ["Examples/ReplaceShader/input.fs"],
                        "outputs": ["Examples/ReplaceShader/output.fs"]
                   }
}
```
- `string_params` is a list of string which will be used to replace the tag `#replace:index#` in shader, where `index` is the index of this list of string. More details please check [**README-ReplaceShader.md**](README-ReplaceShader.md)