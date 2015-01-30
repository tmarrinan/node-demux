{
	'targets': [{
		'target_name': "node_demux",
        'sources': [
        	"node-demux.cc",
        	"node-videodemux.cc"
        ],
		'default_configuration': "Release",
		'conditions': [
			['OS=="linux"', 
				{
					'libraries': [
						"-lavcodec",
						"-lavformat",
						"-lavutil"
					],
					'cflags': [
						"-D__STDC_CONSTANT_MACROS"
					]
				},
            ],
			['OS=="mac"',
				{
					'libraries': [
						"-lavcodec",
						"-lavformat",
						"-lavutil"
					]
				},
			],
			['OS=="win"',
				{
					'include_dirs': [
                        "C:/Dev/ffmpeg-win64-dev/include",
                    ],
                    'libraries': [
						"C:/Dev/ffmpeg-win64-dev/lib/avcodec.lib",
						"C:/Dev/ffmpeg-win64-dev/lib/avformat.lib",
						"C:/Dev/ffmpeg-win64-dev/lib/avutil.lib"
					]
				},
			]
        ]
	}]
}
