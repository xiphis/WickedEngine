#include "wiArguments.h"
#include "wiHelper.h"
#include "wiUnorderedSet.h"

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <sstream>
#include <iterator>
#include <Windows.h>
#include <shellapi.h>

namespace wi::arguments
{
	wi::unordered_set<std::string> params;

	void Parse(const wchar_t* args)
	{
#ifdef _WIN32
		int argc;
		LPWSTR* argcw = CommandLineToArgvW(args, &argc);
		if (!argcw) {
			wprintf(L"CommandLineToArgvW failed\n");
			ExitProcess(1);
		}
		int num = WideCharToMultiByte(CP_UTF8, 0, args, -1, NULL, 0, NULL, NULL);
		char** argv = (char**)alloca(sizeof(char*) * (argc + 2) + num + 1);
		char* target = (char*)(argv + argc + 1);
		for (int i = 0; i < argc; i++)
		{
			int len = WideCharToMultiByte(CP_UTF8, 0, argcw[i], -1, target, num, NULL, NULL);
			argv[i] = target;
			target[len++] = 0;
			target += len;
			num -= len;
		}
		argv[argc] = 0;
		LocalFree(argcw);

		gflags::ParseCommandLineFlags(&argc, &argv, true);
		google::InitGoogleLogging(argv[0]);

		for (int i = 1; i < argc; i++)
		{
			params.insert(std::string(argv[i]));
		}
#else
		std::wstring from = args;
		std::string to;
		wi::helper::StringConvert(from, to);

		std::istringstream iss(to);

		params =
		{
			std::istream_iterator<std::string>{iss},
			std::istream_iterator<std::string>{}
		};
#endif
	}

	void Parse(int argc, char *argv[])
    {
		gflags::ParseCommandLineFlags(&argc, &argv, true);
		google::InitGoogleLogging(argv[0]);

		for (int i = 1; i < argc; i++)
		{
			params.insert(std::string(argv[i]));
		}
    }

	bool HasArgument(const std::string& value)
	{
		return params.find(value) != params.end();
	}
}
