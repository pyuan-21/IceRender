# Steps for generating the Gaussian Filter Sub Shader.

In order to generate the Gaussian kernel with user-defined radius, here are the steps:

1. First use **"gaussian_filter_generator.py"** to generate the sub shader with **sigma(standard deviation)** and **epsilon(lower bound of Gaussian function)**. More details please check the .py file.

2. Once we finish the first step, we will have a **"gaussianFilter_Template.iter_fs"** which need to use command **replace_shader** to specify the desired data type such as **float**, **vec2** and etc.

3. Next, use command **"unroll_shader"** to unroll the **.iter_fs** shader such as **"gaussianFilter_Float.iter_fs"** into **"gaussianFilter_Float.fs"**

4. Finally, we need to use tag **"#import:file_path#"** with command **"generate_shader"** to include the **".fs"** shader to generate the final Gaussian Filter shader.

**Note**: an example:
- open **"cmd"** in current folder, then use **"python gaussian_filter_generator.py"** to generate the **.iter_fs**
- run **IceRender** and load scene file with such command: **"load_scene GenerateShadersScene/gen_gaussian_float_shader.json"** to generate the **".fs"** files.
