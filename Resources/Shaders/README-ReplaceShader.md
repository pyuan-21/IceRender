# Syntax for using replace string

In order to use this feature, we need to put such a tag into the codes:
- `#replace:index#`\
	This tag is used to indicate which string we will use to replace the tag. The number(**index**) between `:` and the final `#` will be converted to `int` for locating the string variables, therefore it is not allow to have any other character such as space. E.g. `#replace:0#` means taking the `index=0` as input to get the string from `string_params`(string list given by `.json` file).

Example:
A shader code in `Examples/ReplaceShader/input.fs`:
```
#replace:0# #replace:1#
```
with `scene.json` file:
```
{
  "replace_shader": {
                        "string_params": ["test1", "test2"],
                        "inputs": ["Examples/ReplaceShader/input.fs"],
                        "outputs": ["Examples/ReplaceShader/output.fs"]
                   }
}

```
will be converted to `Examples/ReplaceShader/output.fs`
```
test1 test2
```
**Note:** 
Examples files can be found the folder: `/Resources/Shaders/Examples/ReplaceShader/`
