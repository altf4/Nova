{
	'targets': [
	{
		'target_name': 'nova',
			'sources': [ 'NovaNode.cpp', 'SuspectJs.cpp' ],
			'libraries': ['-lNova_UI_Core','-lann','-lNovaLibrary','-lnotify'],
			'include_dirs': ['../Nova_UI_Core/src', '../NovaLibrary/src', '/usr/include/cvv8/'],
			'cflags':['-I../../NovaLibrary/src/ -I../../Nova_UI_Core/src -O0 -g3 -Wall -c -fmessage-length=0 `pkg-config --libs --cflags libnotify` -pthread -std=c++0x -fexceptions'],
			'link_settings': {
				'libraries': ['-L../../NovaLibrary/Debug'],
			},
	}
	]
}
