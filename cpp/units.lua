require 'tundra.syntax.glob'
require "tundra.syntax.files"
require "tundra.syntax.ispc"
local native = require "tundra.native"
local host_platform = native.host_platform
-- host_platform == 'linux'

local linuxFilter = "linux-*-*-*"
local unixFilter = { linuxFilter }

local winCrt = ExternalLibrary {
	Name = "winCrt",
}

local warningsAsErrors = ExternalLibrary {
	Name = "warningsAsErrors",
	Propagate = {
		Env = {
			CXXOPTS = {
				{ "-Werror"; Config = unixFilter },
				--{ "/WX"; Config = winFilter },
			},
			CCOPTS = {
				{ "-Werror"; Config = unixFilter },
				--{ "/WX"; Config = winFilter },
			},
		}
	},
}

local unixStdCpp = ExternalLibrary {
	Name = "unixCrt",
	Propagate = {
		Libs = {
			{ "stdc++"; Config = unixFilter }
		},
	},
}

local ideHintApp = {
	Msvc = {
		SolutionFolder = "Applications"
	}
}

local ideHintThirdParty = {
	Msvc = {
		SolutionFolder = "Third Party"
	}
}

local ideHintLibrary = {
	Msvc = {
		SolutionFolder = "Libraries"
	}
}

-- Return an FGlob node that has our standard filters applied
local function makeGlob(dir, options)
	local filters = {
		{ Pattern = "_unix"; Config = unixFilter },
		{ Pattern = "_linux"; Config = linuxFilter },
		{ Pattern = "_android"; Config = "ignore" },       -- Android stuff is built with a different build system
		{ Pattern = "[/\\]_[^/\\]*$"; Config = "ignore" }, -- Any file that starts with an underscore is ignored
	}
	if options.Ignore ~= nil then
		for _, ignore in ipairs(options.Ignore) do
			filters[#filters + 1] = { Pattern = ignore; Config = "ignore" }
		end
	end

	return FGlob {
		Dir = dir,
		Extensions = { ".c", ".cpp", ".h" },
		Filters = filters,
	}
end

local function copyfile_to_output(source, config)
	-- extract just the final part of the path (ie the filename)
	local filename = source:match("/([^/$]+)$")

	if config then
		return CopyFile { Source = source, Target = "$(OBJECTDIR)$(SEP)" .. filename; Config = config }
	else
		return CopyFile { Source = source, Target = "$(OBJECTDIR)$(SEP)" .. filename }
	end
end

local libcurl = ExternalLibrary {
	Name = "libcurl",
	Propagate = {
		Libs = {
			{ "curl"; Config = linuxFilter },
		}
	}
}

local openssl = ExternalLibrary {
	Name = "openssl",
	Propagate = {
		Libs = {
			{ "ssl", "crypto"; Config = linuxFilter },
		}
	}
}

local zlib = ExternalLibrary {
	Name = "zlib",
	Propagate = {
		Libs = {
			{ "z"; Config = linuxFilter },
		}
	}
}

local lz4 = ExternalLibrary {
	Name = "lz4",
	Propagate = {
		Libs = {
			{ "lz4"; Config = unixFilter },
		}
	}
}

local zstd = ExternalLibrary {
	Name = "zstd",
	Propagate = {
		Libs = {
			{ "zstd"; Config = unixFilter },
		}
	}
}

local png = ExternalLibrary {
	Name = "png",
	Propagate = {
		Libs = {
			{ "png"; Config = unixFilter },
		}
	}
}

local jpegturbo = ExternalLibrary {
	Name = "jpegturbo",
	Propagate = {
		Libs = {
			{ "turbojpeg"; Config = unixFilter },
		}
	}
}

local opencv = ExternalLibrary {
	Name = "opencv",
	Propagate = {
		Libs = {
			{"opencv_core", "opencv_features2d", "opencv_xfeatures2d", "opencv_imgproc"; Config = linuxFilter },
		},
	},
}

local CxxUrl = StaticLibrary {
	Name = "CxxUrl",
	Sources = {
		"third_party/bmhpal/third_party/CxxUrl/url.cpp",
		"third_party/bmhpal/third_party/CxxUrl/url.h",
	},
	IdeGenerationHints = ideHintThirdParty,
}

local utfz = StaticLibrary {
	Name = "utfz",
	Depends = { winCrt, },
	Sources = {
		"third_party/bmhpal/third_party/utfz/utfz.cpp",
		"third_party/bmhpal/third_party/utfz/utfz.h",
	},
	IdeGenerationHints = ideHintThirdParty,
}

local xxHash = StaticLibrary {
	Name = "xxHash",
	Depends = { winCrt, },
	Sources = {
		"third_party/bmhpal/third_party/xxHash/xxhash.c",
		"third_party/bmhpal/third_party/xxHash/xxhash.h",
	},
	IdeGenerationHints = ideHintThirdParty,
}

local spooky = StaticLibrary {
	Name = "spooky",
	Depends = { winCrt, },
	Sources = {
		"third_party/bmhpal/third_party/spooky/spooky.c",
		"third_party/bmhpal/third_party/spooky/spooky.h",
	},
	IdeGenerationHints = ideHintThirdParty,
}

local phttp = StaticLibrary {
	Name = "phttp",
	Depends = { winCrt, },
	Sources = {
		"third_party/phttp/http11/http11_common.h",
		"third_party/phttp/http11/http11_parser.c",
		"third_party/phttp/http11/http11_parser.h",
		"third_party/phttp/sha1.c",
		"third_party/phttp/phttp.cpp",
		"third_party/phttp/phttp.h",
	},
	IdeGenerationHints = ideHintThirdParty,
}

local modp = StaticLibrary {
	Name = "modp",
	Depends = { winCrt, },
	Sources = {
		makeGlob("third_party/bmhpal/third_party/modp", {}),
	},
	IdeGenerationHints = ideHintThirdParty,
}

local stb = StaticLibrary {
	Name = "stb",
	SourceDir = "third_party/stb",
	Sources = {
		"stb_all.cpp",
		"stb_image.h",
		"stb_image_write.h",
		"stb_truetype.h",
	},
	IdeGenerationHints = ideHintThirdParty,
}

local StackWalker
if host_platform == 'linux' then
	StackWalker = ExternalLibrary {
		Name = "StackWalker",
	}
else
	StackWalker = StaticLibrary {
		Name = "StackWalker",
		Depends = { winCrt, },
		Sources = {
			"third_party/bmhpal/third_party/TinyTest/StackWalker/StackWalker.cpp",
			"third_party/bmhpal/third_party/TinyTest/StackWalker/StackWalker.h",
		},
		IdeGenerationHints = ideHintThirdParty,
	}
end	

local tsf = StaticLibrary {
	Name = "tsf",
	Sources = {
		"third_party/bmhpal/third_party/tsf/tsf.cpp",
		"third_party/bmhpal/third_party/tsf/tsf.h",
	},
	IdeGenerationHints = ideHintThirdParty,
}

local uberlogger = Program {
	Name = "uberlogger-cyclops",
	Depends = { unixStdCpp },
	Libs = {
		{ "rt"; Config = linuxFilter },
	},
	SourceDir = "third_party/uberlog",
	Sources = {
		"uberlog.cpp",
		"uberlog.h",
		"uberlogger.cpp",
		"tsf.cpp",
		"tsf.h",
	},
	IdeGenerationHints = ideHintApp,
}

local uberlog = StaticLibrary {
	Name = "uberlog",
	Depends = { uberlogger },
	SourceDir = "third_party/uberlog",
	Sources = {
		"uberlog.cpp",
		"uberlog.h",
		"tsf.cpp",
		"tsf.h",
	},
	IdeGenerationHints = ideHintThirdParty,
}

local pal = StaticLibrary {
	Name = "pal",
	Depends = { utfz, libcurl, tsf, openssl, modp, spooky, xxHash },
	Libs = {
		{ "uuid", "curl"; Config = linuxFilter },
	},
	Includes = {
		"third_party/bmhpal",
		"third_party/bmhpal/src",
	},
	PrecompiledHeader = {
		Source = "third_party/bmhpal/src/pch.cpp",
		Header = "pch.h",
		Pass = "PchGen",
	},
	Sources = {
		makeGlob("third_party/bmhpal/src", {}),
	},
	IdeGenerationHints = ideHintLibrary,
}

--[[
local cudaHome
local torchRoot
local torchIncludeRoot

if host_platform == 'linux' then
	-- Docker build, or local machine after running /agent/build/extract-libtorch
	cudaHome = "/usr/local/cuda" -- Might need cuda-10.2 here, for docker builds
	torchRoot = "/usr/local/libtorch"
	
	-- Stock build
	torchIncludeRoot = torchRoot .. "/include"
else
	cudaHome = "cuda-home-not-defined"
	torchRoot = "third_party/libtorch"
	torchIncludeRoot = torchRoot .. "/include"
end

-- Stock build Stable 1.3
local torchSOs = {
	"libtorch.so",
	"libc10.so",
	"libc10_cuda.so",
	"libcaffe2_nvrtc.so",
	"libgomp-753e6e92.so.1",
	"libcudart-1b201d85.so.10.1",
	"libnvrtc-5e8a26c9.so.10.1",
	"libnvToolsExt-3965bdd0.so.1",
}

local torchDeploy = {}
for i, so in ipairs(torchSOs) do
	torchDeploy[#torchDeploy + 1] = copyfile_to_output(torchRoot .. "/lib/" .. so, linuxFilter)
end

local torch = ExternalLibrary {
	Name = "torch",
	Depends = torchDeploy,
	Propagate = {
		Libs = {
			-- Stable 1.2
			{ "torch", "c10", "cuda", "nvrtc"; Config = linuxFilter },
		},
		Env = {
			LIBPATH = {
				torchRoot .. "/lib",
				{ cudaHome .. "/lib64"; Config = linuxFilter },
			},
		},
		Includes = {
			torchIncludeRoot,
			torchIncludeRoot .. "/torch/csrc/api/include",
		},
	}
}

-- This was necessary for building AI with LibTorch in Docker
local rpathLink = ExternalLibrary {
	Name = "rpathLink",
	Propagate = {
		Env = {
			PROGOPTS = {
				--  -rpath-link means that libraries use hardcoded paths at link time, but dynamic paths at runtime.
				--  -rpath      means that libraries use hardcoded paths at link time and runtime.
				"-Wl,-rpath-link=$(@:D)"; Config = linuxFilter
			}
		}
	}
}
--]]

--[[
local AI = Program {
	Name = "AI",
	Depends = {
		rpathLink, winCrt, warningsAsErrors, Video, pal, tsf, phttp, libcurl, modp, lz4, zlib, zstd, utfz, jpegturbo, png, stb, spooky, uberlog, torch
	},
	Libs = {
		{ "Ws2_32.lib"; Config = winFilter },
		{ "unwind", "m", "pthread", "rt", "stdc++"; Config = linuxFilter },
	},
	PrecompiledHeader = {
		Source = "app/AI/pch.cpp",
		Header = "pch.h",
		Pass = "PchGen",
	},
	Includes = {
		"app/AI", -- This is purely here for VS intellisense. Without this, VS can't find pch.h from cpp files that are not in the same dir as pch.h
	},
	Sources = {
		makeGlob("app/AI", {}),
	},
	IdeGenerationHints = ideHintApp,
}
--]]

--[[
local Runner = Program {
	Name = "Runner",
	Depends = {
		warningsAsErrors, pal, tsf, utfz, phttp, libcurl, modp, stb, spooky, uberlog, openssl, CxxUrl, xxHash
	},
	Libs = {
		{ "unwind", "m", "pthread", "rt", "stdc++"; Config = linuxFilter },
	},
	PrecompiledHeader = {
		Source = "app/Runner/pch.cpp",
		Header = "pch.h",
		Pass = "PchGen",
	},
	Defines = {
		-- "_WEBSOCKETPP_CPP11_STL_",
	},
	Includes = {
		"app/Runner",
		"third_party/bmhpal",
	},
	Sources = {
		makeGlob("app/Runner", {}),
	},
	IdeGenerationHints = ideHintApp,
}

Default(Runner)
--]]
	
local hello = Program {
	Name = "hello",
	Depends = {
		warningsAsErrors, pal, tsf, utfz, phttp, libcurl, modp, stb, spooky, uberlog, openssl, CxxUrl, xxHash
		--warningsAsErrors, pal, tsf, utfz
	},
	Libs = {
		{ "unwind", "m", "pthread", "rt", "stdc++"; Config = linuxFilter },
	},
	PrecompiledHeader = {
		Source = "app/hello/pch.cpp",
		Header = "pch.h",
		Pass = "PchGen",
	},
	Includes = {
		"app/hello",
		"third_party/bmhpal",
	},
	Sources = {
		makeGlob("app/hello", {}),
	},
	IdeGenerationHints = ideHintApp,
}
Default(hello)
