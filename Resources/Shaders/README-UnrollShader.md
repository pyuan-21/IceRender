# Syntax for using unroll iteration(loop)

In order to use loop-unrolling feature, we need to put several tags into the codes:
- `#iteration:#`\
	the first tag we need, which ends with `#`. We can put a lot of strings between `:` and the final `#`.
- `#iterVarInit:#`\
	We can define more than one iterated variable. Just like we need to define an initialized variable for the loop: `for(int i=init;i<max;i+=step)`, this tag is used for replace the `init`. We can explicitly define init variable by inserting number between `:` and the final `#`, e.g. `#iterVarInit:0#` will use `0` as init variable. If we don't declare any number in it, then it will try to find the data from `extraIterVarParameters` when using json file to generate the shader file.
- `#iterVarMax:#`\
	Same as `#iterVarInit:#` but it is for max iterated variable.
- `#iterVarStep:#`\
	Same as `#iterVarInit:#` but it is for step iterated variable.
- `#function:#`\
	The main code(logic) which uses the iterated variables, and it will be defined between `:` and the final `#`.
- `#iterVar:#`\
	This tag is used to indicate which iterated variable we will use. The number(required no other character) between `:` and the final `#` will be converted to `int` for locating the iterated variables. Normally, all explicit iterated variables will be pushed into a queue(if they exist), then the `extraIterVarParameters` will be pushed into the same queue. We can not use an index which is outside of this queue. (This tag is optional.)

Example:
A shader code:
```
/*example for using two loops with two iterate variable: iterVar0 and iterVar1, and specify the init/max/step variables*/ 
#iteration: #iterVarInit:0# #iterVarMax:2# #iterVarStep:1# #iterVarInit:0.0# #iterVarMax:0.2# #iterVarStep:0.1# #function:Test(#iterVar:0#, #iterVar:1#);##
```
will be converted to
```
/*example for using two loops with two iterate variable: iterVar0 and iterVar1, and specify the init/max/step variables*/ 
Test(0.000000, 0.000000);
Test(0.000000, 0.100000);
Test(0.000000, 0.200000);
Test(1.000000, 0.000000);
Test(1.000000, 0.100000);
Test(1.000000, 0.200000);
Test(2.000000, 0.000000);
Test(2.000000, 0.100000);
Test(2.000000, 0.200000);
```

**Note:** all iterated variables will be converted into `float`. Of course it also supports no-specific parameters(it will read the **extraParams** from **".json"** file in this case.)
(More examples files can be found the folder: `/Resources/Shaders/Examples/UnrollShader/`)

Normally, the input shaders should have **".iter_fs"** and output it as **".sub_fs"** or **".fs"**.
