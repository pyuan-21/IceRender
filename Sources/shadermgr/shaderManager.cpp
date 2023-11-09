#include "shaderManager.hpp"
#include "../globals.hpp"
#include "../helpers/logger.hpp"
#include "../helpers/utility.hpp"

using namespace IceRender;

bool ShaderManager::FindShaderProgram(const string& _shaderName, shared_ptr<ShaderProgram>& result) const
{
	// Check existing shadername first
	auto iter = shaderMap.find(_shaderName);
	bool isExisted = iter != shaderMap.end();
	if (!isExisted)
		return false;
	result = iter->second;
	return true;
}

void ShaderManager::StopShaderProgram()
{
	// Desactivate the current program
	glUseProgram(0);
	activeShader.clear();
}

bool ShaderManager::ActiveShaderProgram(const string& _shaderName)
{
	shared_ptr<ShaderProgram> target;
	if (FindShaderProgram(_shaderName, target))
	{
		target->Active();
		activeShader = _shaderName;
		return true;
	}
	else
		Print("[Error] No such \'" + _shaderName + "\' shader in ShaderManager.");
	return false;
}

bool ShaderManager::CreateShaderProgram(const string& _shaderName)
{
	// shaderName should be the prefix of vertex/fragment shaders. E.g. "simple" for "simple.vs" and "simple.fs"
	shared_ptr<ShaderProgram> pShaderPro = make_shared<ShaderProgram>();
	bool result = pShaderPro->LoadShader(_shaderName + ".vs", _shaderName + ".fs");
	if (!result)
		return false;

	shaderMap[_shaderName] = pShaderPro;
	return true;
}

void ShaderManager::Clear()
{
	activeShader.clear();
	shaderMap.clear();
}

void ShaderManager::PrintActiveShader()
{
	Print("Current Active Shader Program: " + activeShader);
	shaderMap[activeShader]->PrintShader();
}

string ShaderManager::GetActiveShader() const { return activeShader; }

bool ShaderManager::LoadShader(const string& _shaderName)
{
	// Check existing shadername first
	shared_ptr<ShaderProgram> oldShaderPro = nullptr; // cache it, only the new one is created successfully then we can remove it.
	FindShaderProgram(_shaderName, oldShaderPro);
	shaderMap.erase(_shaderName); // once shared_ptr<ShaderProgram> get removed from map, it will call desctructor of ShaderProgram to delete gl_shaderprogram
	if (CreateShaderProgram(_shaderName))
	{
		shaderMap[_shaderName]->Active();
		activeShader = _shaderName;
		return true;
	}
	else
	{
		shaderMap[_shaderName] = oldShaderPro; // put it back.
		return false;
	}
}


shared_ptr<ShaderProgram> ShaderManager::GetActiveShaderProgram() const
{
	if (activeShader.empty())
	{
		Print("Error: Active Shader Program is empty.");
		return nullptr;
	}
	shared_ptr<ShaderProgram> target;
	if (FindShaderProgram(activeShader, target))
		return target;
	else
		Print("[Error] No such \'" + activeShader + "\' shader in ShaderManager.");
	return nullptr;
}

shared_ptr<ShaderProgram> ShaderManager::TryActivateShaderProgram(const string& _shaderName)
{
	shared_ptr<ShaderProgram> target = nullptr;
	if (!FindShaderProgram(_shaderName, target))
	{
		if (CreateShaderProgram(_shaderName))
			target = shaderMap[_shaderName];
		else
			return nullptr; // can not create 
	}
	// activate it
	target->Active();
	activeShader = _shaderName;
	return target;
}

void ShaderManager::GenerateShaderFile(const std::string& _outputFileName, const std::string& _mainFileName)
{
	// If forget how to use, check "Resources/Shaders/README-UsageOfGenerateShader.md"

	// try open main file
	std::ifstream iStrm(_mainFileName);
	if (!iStrm.is_open())
	{
		Print("[Error] failed to open: " + _mainFileName);
		return;
	}

	// open/create output file
	std::ofstream oStrm(_outputFileName, std::ios::out);
	if (!oStrm.is_open())
	{
		Print("[Error] failed to open: " + _outputFileName);
		return;
	}

	oStrm << "/* This file is automatically generated by \"" << _mainFileName << "\". */\n";

	std::string importTag = "#import:";

	std::string line;
	int lineNum = 0;
	while (std::getline(iStrm, line))
	{
		lineNum++;
		if (line.find(importTag) != std::string::npos)
		{
			//e.g. #import:"Resources/SubShaders/a.sub_fs"#
			std::size_t startFlagPos = line.find('#');
			std::size_t endFlagPos = line.find('#', importTag.size());
			if (endFlagPos == std::string::npos)
			{
				Print("[Error] line: " + std::to_string(lineNum) + ", can not find ending '#'.");
				return;
			}
			std::string subFileName = line.substr(importTag.size() + 1 + startFlagPos, endFlagPos - 2 - startFlagPos - importTag.size());

			std::ifstream subStrm(GLOBAL.shaderPathPrefix + subFileName);
			if (!subStrm.is_open())
			{
				Print("[Error] line: " + std::to_string(lineNum) + ", failed to open: " + GLOBAL.shaderPathPrefix + subFileName);
				return;
			}

			oStrm << subStrm.rdbuf() << "\n";
		}
		else
			oStrm << line << "\n";
	}

	Print("[Done] " + _outputFileName + " has been created successfully.");
}

void ShaderManager::GenerateShaderFilesFromConfig(nlohmann::json _data)
{
	// If forget how to use, check "Resources/Shaders/README-UsageOfGenerateShader.md"
	std::vector<std::string> inputs, outputs;
	if (_data.contains("inputs"))
		inputs = _data["inputs"].get<std::vector<std::string>>();
	if (_data.contains("outputs"))
		outputs = _data["outputs"].get<std::vector<std::string>>();
	if (inputs.size() != outputs.size())
	{
		Print("[Error] In GenerateShaderFilesFromConfig(), inputs and outputs don't have the same size.");
		return;
	}
	for (size_t i = 0; i < inputs.size(); i++)
		GenerateShaderFile(GLOBAL.shaderPathPrefix + outputs[i], GLOBAL.shaderPathPrefix + inputs[i]);
}

void ShaderManager::UnrollShaderFilesFromConfig(nlohmann::json _data)
{
	// If forget how to use, check "Resources/Shaders/README-UnrollShader.md"
	std::vector<std::string> inputs, outputs;
	if (_data.contains("inputs"))
		inputs = _data["inputs"].get<std::vector<std::string>>();
	if (_data.contains("outputs"))
		outputs = _data["outputs"].get<std::vector<std::string>>();
	if (inputs.size() != outputs.size())
	{
		Print("[Error] In UnrollShaderFilesFromConfig(), inputs and outputs don't have the same size.");
		return;
	}
	
	std::map<std::string, std::vector<std::tuple<float, float, float>>> extraIterVarParametersMap;
	if (_data.contains("extra_params_map"))
	{
		auto extraParamsMap = _data["extra_params_map"].get<std::map<std::string, std::vector<std::vector<float>>>>();
		for (const auto& extraParams : extraParamsMap)
		{
			extraIterVarParametersMap[extraParams.first] = std::vector<std::tuple<float, float, float>>();

			for (auto extra : extraParams.second)
			{
				if (extra.size() != 3)
				{
					Print("[Error] In UnrollShaderFilesFromConfig(), each parameter tuple inside extra_params_map should have size of 3.");
					return;
				}
				extraIterVarParametersMap[extraParams.first].push_back({ extra[0],extra[1],extra[2] });
			}
		}
	}

	for (size_t i = 0; i < inputs.size(); i++)
	{
		std::vector<std::tuple<float, float, float>> extraIterVarParameters;
		if (extraIterVarParametersMap.find(inputs[i]) != extraIterVarParametersMap.end())
		{
			extraIterVarParameters = extraIterVarParametersMap[inputs[i]];
			//Print("[Error] In UnrollShaderFilesFromConfig(), extraIterVarParametersMap has no such key: " + inputs[i]);
			//return;
		}
		UnrollShaderFile(GLOBAL.shaderPathPrefix + outputs[i], GLOBAL.shaderPathPrefix + inputs[i], extraIterVarParameters);
	}
}

void ShaderManager::UnrollShaderFile(
	const std::string& _outputFileName, 
	const std::string& _inputFileName,
	const std::vector<std::tuple<float, float, float>> _extraIterVarParameters)
{
	// try open main file
	std::ifstream istrm(_inputFileName);
	if (!istrm.is_open())
	{
		Print("[Error] failed to open: " + _inputFileName);
		return;
	}

	// open/create output file
	std::ofstream ostrm(_outputFileName, std::ios::out);
	if (!ostrm.is_open())
	{
		Print("[Error] failed to open: " + _outputFileName);
		return;
	}

	ostrm << "/* This file is automatically generated by \"" << _inputFileName << "\". */\n";

	std::string iterTag = "#iteration:";
	// TODO: using "regular expression library" to do the matching?
	std::string iterVarInitTag = "#iterVarInit:", iterVarMaxTag = "#iterVarMax:", iterVarStepTag = "#iterVarStep:", iterVarTag = "#iterVar:";
	std::string iterFunctionTag = "#function:";

	std::string line;
	int lineNum = 0;
	while (std::getline(istrm, line))
	{
		lineNum++;
		if (line.find(iterTag) != std::string::npos)
		{
			// input could be:
			// e.g. #iteration: #iterVarInit:0# #iterVarMax:3# #iterVarStep:1# #iterVarInit:0.0# #iterVarMax:0.2# #iterVarStep:0.1# #function: Test(#iterVar:0#, #iterVar:1#);##
			std::size_t startFlagPos = line.find('#');
			std::size_t endFlagPos = line.rfind('#');
			if (endFlagPos == std::string::npos || startFlagPos == endFlagPos)
			{
				Print("[Error] line: " + std::to_string(lineNum) + ", can not find ending '#'.");
				return;
			}

			std::string iterMainBody = line.substr(iterTag.size() + startFlagPos, endFlagPos - startFlagPos - iterTag.size());
			// e.g. "iterMainBody" could be such as:
			// #iterVarInit:0# #iterVarMax:3# #iterVarStep:1# #iterVarInit:0.0# #iterVarMax:0.2# #iterVarStep:0.1# #function: Test(#iterVar:0#, #iterVar:1#);#

			std::vector<std::tuple<float, float, float>> iterVarParameters;
			std::size_t iterVarInitTagPos = iterMainBody.find(iterVarInitTag);
			std::size_t iterVarMaxTagPos = iterMainBody.find(iterVarMaxTag);
			std::size_t iterVarStepTagPos = iterMainBody.find(iterVarStepTag);

			// a function for retrieving the float number from tag-string
			auto retrieveFunc = [](std::string& _mainBodyStr, std::string& _tag, std::size_t& _tagPos, auto& _result, std::size_t& _endFlagPos)
			{
				_endFlagPos = _mainBodyStr.find('#', _tagPos + _tag.size()); // changing the ouside 'endFlagPos'
				try
				{
					_result = std::stof(_mainBodyStr.substr(_tagPos + _tag.size(), _endFlagPos - _tagPos - _tag.size()));
					return true;
				}
				catch (const std::exception& e)
				{
					return false; // can not retrieve value (it's supported: e.g. "#iterVarInit:#" has no number at all. Then it will use the _extraIterVarParameters
				}
			};

			auto erroCheckFunc = [&lineNum](std::size_t& _endFlagPos, std::string& _tag, bool _forceError = false)
			{
				if (_endFlagPos == std::string::npos || _forceError)
				{
					Print("[Error] line: " + std::to_string(lineNum) + ", find an end flag '#' for " + _tag);
					return true;
				}
				return false;
			};

			auto extraIndex = _extraIterVarParameters.begin();

			while (
				iterVarInitTagPos != std::string::npos &&
				iterVarMaxTagPos != std::string::npos &&
				iterVarStepTagPos != std::string::npos)
			{
				float iterVarInit, iterVarMax, iterVarStep;
				// e.g. find an end flag '#' for '#iterVarInit:'
				bool hasIterVarInit = retrieveFunc(iterMainBody, iterVarInitTag, iterVarInitTagPos, iterVarInit, endFlagPos);
				if (erroCheckFunc(endFlagPos, iterVarInitTag))
					return;
				bool hasIterVarMax = retrieveFunc(iterMainBody, iterVarMaxTag, iterVarMaxTagPos, iterVarMax, endFlagPos);
				if (erroCheckFunc(endFlagPos, iterVarMaxTag))
					return;
				bool hasIterVarStep = retrieveFunc(iterMainBody, iterVarStepTag, iterVarStepTagPos, iterVarStep, endFlagPos);
				if (erroCheckFunc(endFlagPos, iterVarStepTag))
					return;

				if (hasIterVarInit && hasIterVarMax && hasIterVarStep)
					iterVarParameters.push_back({ iterVarInit, iterVarMax, iterVarStep });
				else
				{
					if (extraIndex != _extraIterVarParameters.end())
					{
						iterVarParameters.insert(iterVarParameters.end(), extraIndex, extraIndex + 1);
						extraIndex += 1;
					}
					else
					{
						Print("[Error] line: " + std::to_string(lineNum) + ", not enough extra parameters for initializing the iterated variables");
						return;
					}
				}

				// try to find next iteration
				iterMainBody = iterMainBody.substr(endFlagPos + 1);
				iterVarInitTagPos = iterMainBody.find(iterVarInitTag);
				iterVarMaxTagPos = iterMainBody.find(iterVarMaxTag);
				iterVarStepTagPos = iterMainBody.find(iterVarStepTag);
			}

			std::string finalResult;

			if (iterVarParameters.size() <= 0)
			{
				Print("[Error] line: " + std::to_string(lineNum) +
					", can not find any matched iterVarParamers. Expected example: '#iterVarInit:0# #iterVarMax:6# #iterVarStep:1# function(#iterVar:0#)'.");
				return;
			}
			else
			{
				std::size_t iterFunctionTagPos = iterMainBody.find(iterFunctionTag);

				if (iterFunctionTagPos == std::string::npos)
				{
					Print("[Error] line: " + std::to_string(lineNum) + ", can not find iterFunctionTag:" + iterFunctionTag);
					return;
				}

				endFlagPos = iterMainBody.rfind('#');

				if (erroCheckFunc(endFlagPos, iterFunctionTag, endFlagPos == iterFunctionTagPos))
					return;

				// extract the function body
				iterMainBody = iterMainBody.substr(iterFunctionTagPos + iterFunctionTag.size(), endFlagPos - iterFunctionTagPos - iterFunctionTag.size());

				// replacement: replace all #iterVar:i# by using iterVarParameters[i]
				// the idea is to run iteration and during each iteration, we replace #iterVar# tag one by one util no more #iterVar#
				std::queue<std::string> replacedResults;
				replacedResults.push(iterMainBody);
				for (int iterVarIndex = 0; iterVarIndex < iterVarParameters.size(); iterVarIndex++)
				{
					float vInit = std::get<0>(iterVarParameters[iterVarIndex]), 
						vMax = std::get<1>(iterVarParameters[iterVarIndex]), 
						vStep = std::get<2>(iterVarParameters[iterVarIndex]);

					// [important] no other character between ":" and "#" inside iterVar tag.
					std::string replaceIterVarStr = iterVarTag + std::to_string(iterVarIndex) + "#";

					int popCount = replacedResults.size();
					while (popCount-- > 0)
					{
						auto originalRes = replacedResults.front(); // copy it
						replacedResults.pop(); // remove it
						// generate new many result and then push them into queue
						for (auto i = vInit; i <= vMax; i += vStep)
						{
							auto result = originalRes; // copy it
							std::size_t pos = result.find(replaceIterVarStr);
							while (pos != std::string::npos)
							{
								result.replace(result.find(replaceIterVarStr), replaceIterVarStr.size(), std::to_string(i));
								pos = result.find(replaceIterVarStr);
							}
							replacedResults.push(result);
						}
					}
				}

				// combine all sub-result(function call with the true iterate number from loop)
				while (!replacedResults.empty())
				{
					finalResult.append(replacedResults.front());
					if (replacedResults.size() > 1)
						finalResult.append("\n");
					replacedResults.pop();
				}
			}

			if (finalResult.find(iterVarTag) != std::string::npos)
			{
				Print("[Error] line: " + std::to_string(lineNum) + ", after replacing for iter-variables, there is still '#iterVar:#' remaining.");
				return;
			}

			ostrm << std::string("/*--------------------------------------------------------*/") << "\n";
			ostrm << std::string("/* below codes are generated by \"unroll_shader\" command.*/") << "\n";
			ostrm << finalResult << "\n";
			ostrm << std::string("/*--------------------------------------------------------*/") << "\n";
		}
		else
			ostrm << line << "\n";
	}

	Print("File " + _outputFileName + " has been created successfully.");
}


void ShaderManager::ReplaceShaderFilesFromConfig(nlohmann::json _data)
{
	// If forget how to use, check "Resources/Shaders/README-ReplaceShader.md"
	std::vector<std::string> inputs, outputs;
	if (_data.contains("inputs"))
		inputs = _data["inputs"].get<std::vector<std::string>>();
	if (_data.contains("outputs"))
		outputs = _data["outputs"].get<std::vector<std::string>>();
	if (inputs.size() != outputs.size())
	{
		Print("[Error] In ReplaceShaderFilesFromConfig(), inputs and outputs don't have the same size.");
		return;
	}

	std::vector<std::string> stringParams;
	if (_data.contains("string_params"))
	{
		stringParams = _data["string_params"].get<std::vector<std::string>>();
	}
	if (stringParams.size() <= 0)
	{
		Print("[Error] In ReplaceShaderFilesFromConfig(), stringParams size is 0.");
		return;
	}

	for (size_t i = 0; i < inputs.size(); i++)
	{
		ReplaceShaderFile(GLOBAL.shaderPathPrefix + outputs[i], GLOBAL.shaderPathPrefix + inputs[i], stringParams);
	}
}


void ShaderManager::ReplaceShaderFile(const std::string& _outputFileName, const std::string& _inputFileName, const std::vector<std::string> _stringParams)
{
	// try open main file
	std::ifstream istrm(_inputFileName);
	if (!istrm.is_open())
	{
		Print("[Error] failed to open: " + _inputFileName);
		return;
	}

	// open/create output file
	std::ofstream ostrm(_outputFileName, std::ios::out);
	if (!ostrm.is_open())
	{
		Print("[Error] failed to open: " + _outputFileName);
		return;
	}

	ostrm << "/* This file is automatically generated by \"" << _inputFileName << "\". */\n";

	// TODO: using "regular expression library" to do the matching?
	std::string replaceTag = "#replace:";
	std::size_t startFlagPos = std::string::npos, endFlagPos = std::string::npos;

	std::string line;
	int lineNum = 0;
	while (std::getline(istrm, line))
	{
		lineNum++;

		// a function for retrieving the number from tag-string
		auto retrieveFunc = [](std::string& _inputStr, std::string& _tag, std::size_t& _tagPos, auto& _result, std::size_t& _endFlagPos)
		{
			_endFlagPos = _inputStr.find('#', _tagPos + _tag.size()); // changing the ouside 'endFlagPos'
			try
			{
				_result = std::stof(_inputStr.substr(_tagPos + _tag.size(), _endFlagPos - _tagPos - _tag.size()));
				return true;
			}
			catch (const std::exception& e)
			{
				return false; // can not retrieve value
			}
		};
		startFlagPos = line.find(replaceTag);
		while (startFlagPos != std::string::npos)
		{
			int result_index = -1;
			bool hasRetrieved = retrieveFunc(line, replaceTag, startFlagPos, result_index, endFlagPos);
			if (!hasRetrieved || result_index < 0 || result_index > _stringParams.size())
			{
				Print(NULL, "sds", "[Error] line: ", lineNum, ", please provide a valid index inside the #replace:# tag.");
				return;
			}

			for (int i = 0; i < _stringParams.size(); i++)
			{
				std::string replaceTagStr = replaceTag + std::to_string(i) + "#";
				std::size_t pos = line.find(replaceTagStr);
				while (pos != std::string::npos)
				{
					line.replace(line.find(replaceTagStr), replaceTagStr.size(), _stringParams[i]);
					pos = line.find(replaceTagStr);
				}
			}
			startFlagPos = line.find(replaceTag);
		}
		
		ostrm << line << "\n";
	}

	Print("File " + _outputFileName + " has been created successfully.");
}
