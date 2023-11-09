/* This file is automatically generated by "Resources/Shaders/Examples/UnrollShader/input.iter_fs". */

void Test()
{
	/*function body*/
}

void OneLoopOneVariable()
{
	/*below is an example showing that no specific init/max/step. It will use extraParams from .json file*/
/*--------------------------------------------------------*/
/* below codes are generated by "unroll_shader" command.*/
	Test(3.000000);
	Test(4.000000);
	Test(5.000000);
/*--------------------------------------------------------*/
}

void TwoLoopsTwoVariables()
{
	/*example for using two loops with two iterate variable: iterVar0 and iterVar1, and specify the init/max/step variables*/ 
/*--------------------------------------------------------*/
/* below codes are generated by "unroll_shader" command.*/
	Test(0.000000, 0.000000);
	Test(0.000000, 0.100000);
	Test(0.000000, 0.200000);
	Test(1.000000, 0.000000);
	Test(1.000000, 0.100000);
	Test(1.000000, 0.200000);
	Test(2.000000, 0.000000);
	Test(2.000000, 0.100000);
	Test(2.000000, 0.200000);
/*--------------------------------------------------------*/
}

void main()
{
	OneLoopOneVariable();
	TwoLoopsTwoVariables();
}

