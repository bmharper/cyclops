
local common = {
	Env = {
		TARGETARCH = {
			{ "x64"; Config = "linux-*"; },
		},
		CPPPATH = {
			"third_party",
			".", -- The root directory of this project
		},
	},
}

local unix_common = {
	Inherit = common,
	Env = {
		CXXOPTS = {
			{ "-std=c++14" },
			{ "-fPIC" },
			{ "-g" },
			--{ "-mavx" },
			{ "-O2"; Config = "linux-*-release-*" },
			--{ "-g -fsanitize=address" },
			--{ "-fsanitize-address-use-after-scope" },
			-- Use-after-return (runtime flag ASAN_OPTIONS=detect_stack_use_after_return=1)
			--{ "-fno-omit-frame-pointer -fno-optimize-sibling-calls" }, -- Don't think this is necessary, but MIGHT be for better callstacks with ASAN
			-- Breaks OpenCV linkage. We should build it with this on.
			--{ "-D_GLIBCXX_DEBUG"; Config = "linux-clang-debug-*" },
		},
		CCOPTS = {
			{ "-fPIC" },
			{ "-g" },
			--{ "-mavx" },
			{ "-O2"; Config = "linux-*-release-*" },
		},
		--SHLIBOPTS = {
		--	{ "-g -fsanitize=address" },
		--},
		PROGOPTS = {
			--{ "-g -fsanitize=address" },
			--{ "-s" },
		},
	}
}

Build {
	Units = "units.lua",
	Passes= {
		PchGen = { Name = "Precompiled Header Generation", BuildOrder = 1 },
	},
	Variants = { "debug", "release" },
	SubVariants = { "default", "analyze" },
	DefaultSubVariant = "default",
	Configs = {
		{
			Name = "linux-clang",
			DefaultOnHost = "linux",
			Inherit = unix_common,
			Tools = { "clang" },
		},
	},
}
