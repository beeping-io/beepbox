#ifndef INCLUDED_CONCAT_CLIPARSER_HXX
#define INCLUDED_CONCAT_CLIPARSER_HXX

#include <vector>
#include <map>
#include <string>
#include <utility>
#include <cassert>
#include <sstream>

#undef min
#undef max
#include <algorithm>

#include <StringUtils.hxx>

// o arguments are (unnamed) values
//   - for the parser to be able to know which value is what in a list of arguments, 
//   arguments have to be in a fixed order
//   - values can consist of integers, floats/doubles, strings or a list
//   - a list should be homogeneous (for now) and enclosed in square brackets and values 
//   separated using white space (e.g. '[2 3 5 1]')
//   - if a string includes whitespace, it should be enclosed in double quotes (e.g. '"hi how are you"')
//   - arguments are always required to be given
// o options are values preceded by a named flag
//   - flags can be in short (single letter) or long form, short form uses '-', long form uses '--'
//   - flags may followed by an argument (see above) or not (for on/off type things)
//   - options may be required or optional
// o in a mixed list of options and arguments, all options come first, then arguments
class CliParser
{
public:
	// used to allow error checking at parsing, not when extracting 
	// data.. makes error handling easier for the client app
	enum DataType
	{
		CLI_NONE,
		CLI_INT,
		CLI_FLOAT, // or double
		CLI_STRING//,
//		CLI_LIST_INT,
//		CLI_LIST_FLOAT,
//		CLI_LIST_STRING
	};

	enum FlagType
	{
		CLI_SHORT,
		CLI_LONG
	};

	struct Option
	{
		std::string shortName;			// e.g. "n" (-n)
		std::string longName;			// e.g. "num_iter" (--num_iter)
		DataType type;					// e.g. CliParser::CLI_INT
		bool isOptional;				// e.g. true
		std::string valueDescription;	// e.g. "count" (used for generating usage help message)
		std::string description;		// e.g. "Number of iterations to perform in search." (used for generating usage help message)
		std::string defaultValue;		// e.g. "4"


		std::string defaultValueFormatted() const
		{
			if (type == CLI_STRING)
				return "\"" + defaultValue + "\"";
			else
				return defaultValue;
		}

		std::string getName(FlagType flagType = CLI_SHORT, bool withHyphen = false) const
		{
			if (flagType == CLI_LONG && longName.size() > 0)
			{
				if (withHyphen)
					return "--" + longName;
				else
					return longName;
			}
			else
			{
				if (withHyphen)
					return "-" + shortName;
				else
					return shortName;
			}
		}
	};

	struct Argument
	{
		std::string name; // name of argument used for generating usage help message
		DataType type;
		std::string description;
	};

public:
	// setup:
	void addOption(std::string shortName, std::string longName, DataType type, bool isOptional, std::string valueDescription, std::string description = "", std::string defaultValue = "");
	void addArgument(std::string name, DataType type, std::string description = "");

	// parsing:
	bool parse(int numArgs, char *argStrs[]);

	// extracting data:
	bool hasOption(std::string shortFlagName);
	int getOptionAsInt(std::string shortFlagName, int inexistant = 0);
	float getOptionAsFloat(std::string shortFlagName, float inexistant = 0.f);
	double getOptionAsDouble(std::string shortFlagName, double inexistant = 0.0);
	std::string getOptionAsString(std::string shortFlagName, std::string inexistant = "");

	std::string getArgumentAsString(int idx, std::string inexistant = "");

	// help:
	std::string generateUsageMessage(FlagType flagType = CLI_LONG, bool hideAltFlag = false, int wrapAt = 90, int tabSize = 2);

private:
	std::vector<Argument> arguments_;
	std::vector<Option> options_;
	
	std::string executableName_;
	std::vector<std::string> parsedArguments_;
	std::map<std::string, std::string> parsedOptions_;

	// -------------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable : 4018) // signed/unsigned comparison
//#pragma warning(disable : 4267)

	std::string spacer(std::string str, int size)
	{
		assert(size >= str.size());
		return str + std::string(size - str.size(), ' ');
	}

	std::string wrapTextExceptFirstLine(std::string str, int indent, int wrapAt)
	{
		std::vector<std::string> lines = split(str, "\n");
		std::vector<std::string> linesWrapped;
		for (int i = 0; i < len(lines); ++i)
		{
			std::string lineWrapped = wrapTextExceptFirstLineS(lines[i], indent, wrapAt);
			if (i > 0)
				lineWrapped = std::string(indent, ' ') + lineWrapped;
			linesWrapped.push_back(lineWrapped);
		}
		return join(linesWrapped, "\n");
	}

	std::string wrapTextExceptFirstLineS(std::string str, int indent, int wrapAt)
	{
		assert(indent < wrapAt);

		std::string result = "";

		int lineCount = 0;

		while (1)
		{
			// str fits, stop:
			if (indent + str.size() < wrapAt)
			{
				if (lineCount > 0)
					result += std::string(indent, ' ');
				result += str;
				break;
			}

			// break part of str, keep rest:
			std::string::size_type pos = str.rfind(" ", wrapAt - indent);
			if (pos == std::string::npos)
			{
				// can't break line (no preceding white space):
				if (lineCount > 0)
					result += std::string(indent, ' ');
				result += str;
				break;
			}

			if (lineCount > 0)
				result += std::string(indent, ' ');
			result += str.substr(0, pos + 1) + "\n";
			str = str.substr(pos + 1, str.size() - (pos + 1)); // remaining string
			lineCount += 1;
		}

		return result;
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	bool isOptionUnique(const Option &option)
	{
		for (int i = 0; i < options_.size(); ++i)
		{
			if (options_[i].shortName == option.shortName ||
				options_[i].longName == option.longName)
				return false;
		}

		return true;
	}

	bool isShortOptionFlagValid(std::string flag)
	{
		for (int i = 0; i < options_.size(); ++i)
		{
			if (options_[i].shortName == flag)
			{
				return true;
			}
		}

		return false;
	}


	bool isLongOptionFlagValid(std::string flag)
	{
		for (int i = 0; i < options_.size(); ++i)
		{
			//std::string xxx3 = options_[i].longName;
			if (options_[i].longName == flag)
			{
				return true;
			}
		}

		return false;
	}

	std::string lookUpShortFromLongOptionFlag(std::string longFlagName)
	{
		for (int i = 0; i < options_.size(); ++i)
		{
			if (options_[i].longName == longFlagName)
				return options_[i].shortName;
		}

		return "";
	}

	bool isShortOptionFlag(std::string str)
	{
		return (str.find("-") == 0 && str.find("--") != 0);
	}

	bool isLongOptionFlag(std::string str)
	{
		return (str.find("--") == 0);
	}

	void endOptionFlag(std::string shortFlagName, std::string data)
	{
		parsedOptions_.insert(std::make_pair(shortFlagName, data));
	}

	void endArgument(std::string data)
	{
		parsedArguments_.push_back(data);
	}

	bool isStringConvertibleToType(std::string str, DataType type)
	{
		switch (type)
		{
			case CLI_NONE:
			{
				return true;
			}
			case CLI_INT:
			{
				std::stringstream ss;
				ss << str;
				int test;
				ss >> test;
				return (ss.fail() != true);
			}
			case CLI_FLOAT:
			{
				std::stringstream ss;
				ss << str;
				double test;
				ss >> test;
				return (ss.fail() != true);
			}
			case CLI_STRING:
			{
				return true;
			}
		}

		assert(0);
		return false; // shouldn't be reached
	}

	const Option *getOptionByShortName(std::string name)
	{
		for (int i = 0; i < options_.size(); ++i)
		{
			if (options_[i].shortName == name)
				return &options_[i];
		}

		return NULL;
	}

#pragma warning(pop)
};


// ---------------------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable : 4018)
//#pragma warning(disable : 4267)

inline void CliParser::addOption(std::string shortName, std::string longName, DataType type, bool isOptional, std::string valueDescription, std::string description, std::string defaultValue)
{
	Option option;
	option.shortName = shortName;
	option.longName = longName;
	option.type = type;
	option.isOptional = isOptional;
	option.valueDescription = valueDescription;
	option.description = description;
	option.defaultValue = defaultValue;

	if (isOptionUnique(option))
		options_.push_back(option);
}

inline void CliParser::addArgument(std::string name, DataType type, std::string description)
{
	Argument arg;
	arg.name = name;
	arg.type = type;
	arg.description = description;

	// NOTE: arguments are always unique (by index, not necessarily by name)
	arguments_.push_back(arg);
}

// -----------------------------------------------------------------------------------

inline bool CliParser::parse(int numArgs, char *argStrs[])
{
	if (numArgs < 1)
		return false; // error

	// Store executable name:
	executableName_ = argStrs[0];

	if (numArgs >= 2)
	{
		std::string firstArg = argStrs[1];
		if (firstArg == "-?" || firstArg == "--?" || firstArg == "/?" || firstArg == "-h" || firstArg == "--help" || firstArg == "-help" || firstArg == "--h" || firstArg == "/h")
			return false; // fail to display usage (not really error)
	}

	// Parser internal:
	int state = 0;
	// state = 0 : next argStr can be anything (argument/option)
	// state = 1 : next argStr should be value for flag (for options with data)
//	// state = 2 : next argStr should be list item

	std::string curShortFlagName;
	std::string curList; // white space separated items
	int curArgumentIdx = 0;

	// Parser loop:
	for (int i = 1; i < numArgs; ++i)
	{
		std::string argStr = argStrs[i];

    //BEGIN HACK to allow negative parameter values that start with - !!
    if ( ((state == 1) && (strcmp(argStrs[i - 1], "-v") == 0)) ||
         ((state == 1) && (strcmp(argStrs[i - 1], "--volumebeeps") == 0)) )
    { // close option
      endOptionFlag(curShortFlagName, argStr);
      state = 0;
      curShortFlagName = "";
    }
    else if (((state == 1) && (strcmp(argStrs[i - 1], "-p") == 0)) ||
      ((state == 1) && (strcmp(argStrs[i - 1], "--volumeprogram") == 0)))
    { // close option
      endOptionFlag(curShortFlagName, argStr);
      state = 0;
      curShortFlagName = "";
    }
    else if (((state == 1) && (strcmp(argStrs[i - 1], "-sv") == 0)) ||
      ((state == 1) && (strcmp(argStrs[i - 1], "--synthvolume") == 0)))
    { // close option
      endOptionFlag(curShortFlagName, argStr);
      state = 0;
      curShortFlagName = "";
    }
    else
    //END HACK!!

		// short flag
		if (isShortOptionFlag(argStr))
		{
			if (state == 1)
			{
				// close flag (no data)
				endOptionFlag(curShortFlagName, "");
			}

//			if (state == 2)
//				return false; // error

			std::string shortFlagName = argStr.substr(1, argStr.size()-1);
			if (!isShortOptionFlagValid(shortFlagName))
				return false; // error

			if (getOptionByShortName(shortFlagName)->type != CLI_NONE)
				state = 1;
			else
			{
				endOptionFlag(shortFlagName, "");
				state = 0;
			}
			curShortFlagName = shortFlagName;
		}
		// long flag
		else if (isLongOptionFlag(argStr))
		{
			if (state == 1)
			{
				// close flag (no data)
				endOptionFlag(curShortFlagName, "");
			}

//			if (state == 2)
//				return false; // error

			std::string longFlagName = argStr.substr(2, argStr.size()-2);
			if (!isLongOptionFlagValid(longFlagName))
				return false; // error

			// look up short flagName from long flagName
			std::string shortFlagName = lookUpShortFromLongOptionFlag(longFlagName);

			if (getOptionByShortName(shortFlagName)->type != CLI_NONE)
				state = 1;
			else
			{
				endOptionFlag(shortFlagName, "");
				state = 0;
			}
			curShortFlagName = shortFlagName;
		}
		/*
		// list argument begin
		else if (isBeginList(argStr))
		{
			if (state == 2)
				return false; // error

			state = 2;
			curList = argStr.substr(1, argStr.size()-1);
		}
		// list middle/end
		else if (state == 2)
		{
			if (isEndList(argStr))
			{
				curList += argStr.substr(0, argStr.size()-1);

				// close option/argument
				state = 0;
				curList = "":
				curShortFlagName = "";
			}
			else
			{
				curList += argStr;
				state = 2;
			}
		}*/
		else // state != 2, != list begin, != short/long option flag
		{
			if (state == 0)
			{
				// close argument
				endArgument(argStr);
				++curArgumentIdx;
				state = 0;
			}
			else if (state == 1)
			{
				// close option
				endOptionFlag(curShortFlagName, argStr);
				state = 0;
				curShortFlagName = "";
			}
		}
	}

	// Check if all required options are set and have correct (convertible) types:
	for (int i = 0; i < options_.size(); ++i)
	{
		bool isOptionSet = hasOption(options_[i].shortName);
		bool isOptionOptional = options_[i].isOptional;

		// option isn't set and required:
		if (!isOptionOptional && !isOptionSet)
			return false; // error

		// option is set, but of wrong type:
		if (isOptionSet && !isStringConvertibleToType((*parsedOptions_.find(options_[i].shortName)).second, options_[i].type))
			return false; // error

		// option isn't set and has default value, use default value:
		if (!isOptionSet && isOptionOptional && options_[i].defaultValue != "")
			endOptionFlag(options_[i].shortName, options_[i].defaultValue);
	}

	// Check Arguments:
	if (parsedArguments_.size() != arguments_.size())
		return false; // error

	for (int i = 0; i < arguments_.size(); ++i)
	{
		if (!isStringConvertibleToType(parsedArguments_[i], arguments_[i].type))
			return false; // error
	}

	return true; // successful
}

// -----------------------------------------------------------------------------------

inline bool CliParser::hasOption(std::string shortFlagName)
{
	return (parsedOptions_.find(shortFlagName) != parsedOptions_.end());
}

inline int CliParser::getOptionAsInt(std::string shortFlagName, int inexistant)
{
	if (!hasOption(shortFlagName))
		return inexistant;

	std::stringstream ss;
	ss << (*parsedOptions_.find(shortFlagName)).second;
	int result;
	ss >> result;

	return result;
}

inline float CliParser::getOptionAsFloat(std::string shortFlagName, float inexistant)
{
	if (!hasOption(shortFlagName))
		return inexistant;

	std::stringstream ss;
	ss << (*parsedOptions_.find(shortFlagName)).second;
	float result;
	ss >> result;

	return result;
}

inline double CliParser::getOptionAsDouble(std::string shortFlagName, double inexistant)
{
	if (!hasOption(shortFlagName))
		return inexistant;

	std::stringstream ss;
	ss << (*parsedOptions_.find(shortFlagName)).second;
	double result;
	ss >> result;

	return result;
}

inline std::string CliParser::getOptionAsString(std::string shortFlagName, std::string inexistant)
{
	if (!hasOption(shortFlagName))
		return inexistant;

	return (*parsedOptions_.find(shortFlagName)).second;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline std::string CliParser::getArgumentAsString(int idx, std::string inexistant)
{
	if (idx < 0 || idx >= parsedArguments_.size())
		return inexistant;

	return parsedArguments_[idx];
}

// -----------------------------------------------------------------------------------

inline std::string CliParser::generateUsageMessage(FlagType flagType, bool hideAltFlag, int wrapAt, int tabSize)
{
	// Compute column widths for pretty printing:
	int maxShortFlagLength = 0;
	int maxValueDescLength = 0;
	int maxArgNameLength = 0;

	for (int i = 0; i < options_.size(); ++i)
	{
		maxShortFlagLength = std::max((int)options_[i].getName(flagType, true).size(), maxShortFlagLength);
		maxValueDescLength = std::max((int)options_[i].valueDescription.size(), maxValueDescLength);
	}

	for (int i = 0; i < arguments_.size(); ++i)
	{
		maxArgNameLength = std::max((int)arguments_[i].name.size(), maxArgNameLength);
	}

	std::stringstream msg;

	std::string params[3] = {"--mode", "--duration", "--interval"};

	// Command:
	//msg << std::endl;

	std::string executableOnlyNoExtension;
	std::string::size_type beginFilenameMin1 = executableName_.rfind("\\");
	std::string::size_type endFilename = executableName_.rfind(".");
	if (beginFilenameMin1 != std::string::npos && endFilename != std::string::npos)
	{
		std::string::size_type beginFilename = beginFilenameMin1+1;
		executableOnlyNoExtension = executableName_.substr(beginFilename, endFilename - beginFilename);
	}

	std::string usageLine = "Usage: BeepBox " + executableOnlyNoExtension;
	//if (options_.size() > 0)
		//usageLine += " ";
	std::string argumentsLine = "";

	for (int i = 0; i < options_.size(); ++i)
	{

		if (options_[i].isOptional) {

			int found = 0 ;

			for (int y = 0; y < 4; y++)
			{
				if (options_[i].getName(flagType, true) == params[y])
				{
					found = 1 ;

					break ;					
				}
			}

			if (found == 0 )
				continue;

		}

		if (i > 0)
			argumentsLine += " ";
		if (options_[i].isOptional)
			argumentsLine += "[";
		argumentsLine += options_[i].getName(flagType, true);
		if (options_[i].valueDescription.size() > 0)
			argumentsLine += " " + options_[i].valueDescription;
		if (options_[i].isOptional)
			argumentsLine += "]";
	}
	for (int i = 0; i < arguments_.size(); ++i)
	{
		argumentsLine += " ";
		argumentsLine += arguments_[i].name;
	}

	msg << usageLine << wrapTextExceptFirstLine(argumentsLine, usageLine.size(), wrapAt) << std::endl;
	msg << std::endl;	

		// Options descriptions:
		if (options_.size() > 0)
	{
		msg << "Options:" << std::endl
			<< std::endl;
		for (int i = 0; i < options_.size(); ++i)
		{

			if (options_[i].isOptional)
			{

				int found = 0;

				for (int y = 0; y < 4; y++)
				{
					if (options_[i].getName(flagType, true) == params[y])
					{
						found = 1;

						break;
					}
				}

				if (found == 0)
					continue;
			}

				std::string optLine = "";
			optLine += spacer("", tabSize);
			optLine += spacer(options_[i].getName(flagType, true), maxShortFlagLength + tabSize);
			optLine += spacer(options_[i].valueDescription, (maxValueDescLength/tabSize + 1)*tabSize);
			std::string fullDescription = options_[i].description;
			if (options_[i].longName.size() > 0 )
			{
				if (!hideAltFlag)
				{
					if (flagType == CLI_SHORT)
						fullDescription += "\n" + std::string("Long name: ") + options_[i].getName(CLI_LONG, true);
					else
						fullDescription += "\n" + std::string("Short name: ") + options_[i].getName(CLI_SHORT, true);
				}

				if (options_[i].isOptional)
					if (options_[i].defaultValue.size() == 0)
						fullDescription += "\n" + std::string("Optional.");
					else
						fullDescription += "\n" + std::string("Optional ") + "(default: " + options_[i].defaultValueFormatted() + ").";
				else
					;//fullDescription += "\n" + std::string("Mandatory.");
			}
			else
			{
				if (options_[i].isOptional)
					if (options_[i].defaultValue.size() == 0)
						fullDescription += std::string("Optional.");
					else
						fullDescription += std::string("Optional ") + "(default: " + options_[i].defaultValueFormatted() + ").";
				else
					;//fullDescription += std::string("Mandatory.");
			}
			optLine += wrapTextExceptFirstLine(fullDescription, optLine.size(), wrapAt);
			optLine += "\n";
			msg << optLine;
		}
	}

	// Argument descriptions:
	if (arguments_.size() > 0)
	{
		msg << std::endl;
		msg << "Arguments:" << std::endl;
		for (int i = 0; i < arguments_.size(); ++i)
		{
			std::string argLine = "";
			argLine += spacer("", tabSize);
			argLine += spacer(arguments_[i].name, (maxArgNameLength/tabSize + 1)*tabSize);
			argLine += wrapTextExceptFirstLine(arguments_[i].description, argLine.size(), wrapAt);
			argLine += "\n";
			msg << argLine;
		}
	}

	msg << std::endl;

	return msg.str();
}

// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------


#pragma warning(pop)

// ---------------------------------------------------------------------------------------

#endif

// ---------------------------------------------------------------------------------------

#if (TEST_CLI_PARSER != 0)
#include <iostream>


int main(int nArgs, char *args[])
{
	CliParser parser;
	parser.addOption("np", "no_pitch_guides", CliParser::CLI_NONE, true, "", "Do not generate pitch guide files.");
	parser.addOption("nx", "no_db_xml", CliParser::CLI_NONE, true, "", "Do not generate database XML files.");
	parser.addArgument("input-path", CliParser::CLI_STRING, "Path to input files (e.g. \"./\").");
	parser.addArgument("output-path", CliParser::CLI_STRING, "Path where output files will be written.");

	if (parser.parse(nArgs, args) != true)
	{
		std::cout << parser.generateUsageMessage();
		return 1;
	}

	bool noPitchGuides = parser.hasOption("np");
	bool noDbXml = parser.hasOption("nx");
	std::string inputPath = parser.getArgumentAsString(0);
	std::string outPath = parser.getArgumentAsString(1);

	return 0;
}
#endif
// ---------------------------------------------------------------------------------------
