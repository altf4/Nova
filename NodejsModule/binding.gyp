  {
    'targets': [
      {
        'target_name': 'novaconfig',
        'defines': [
          'DEFINE_FOO',
          'DEFINE_A_VALUE=value',
        ],
        'include_dirs': [
  	      ".",
          '..',
          "../Nova_UI_Core/src",
          "../NovaLibrary/src",
		  "/usr/include/cvv8/", 
        ],
	    'cflags_cc!': [ '-fno-exceptions', '-fno-rtti' ],
        'cflags': [
          "-g",
          "-D_FILE_OFFSET_BITS=64",
          "-D_LARGEFILE_SOURCE",
          "-Wall",
          "-W",
          "-std=c++0x",
          "-D_REENTRANT",
          "-pipe",
          "-pthread",
		  "-fexceptions",
		  "-frtti",
		],
		'link_settings': {
		  'libraries': [
			"-L../../NovaLibrary",
			"-L../../Nova_UI_Core",
		    "-lNovaLibrary",
		    "-lNova_UI_Core",
		    "-lann",
            "-lcurl",
            "-lboost_filesystem",
            "-lboost_program_options",
            "-lprotobuf", 
            "-lz",
		  ],
		},
        'sources': [
          "NovaNode.cpp",
          "NovaConfig.cpp",
          "NovaConfigBinding.cpp",
          "HoneydConfigBinding.cpp",
          "HoneydTypesJs.cpp",
          "VendorMacDbBinding.cpp",
          "OsPersonalityDbBinding.cpp",
          "HoneydProfileBinding.cpp",
          "CustomizeTraining.cpp",
          "WhitelistConfigurationBinding.cpp",
          "TrainingDumpBinding.cpp",
          "LoggerBinding.cpp",
        ],
      }]
  }
