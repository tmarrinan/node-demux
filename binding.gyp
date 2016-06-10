{
	'targets': [{
		'target_name': "node_demux",
        'sources': [
        	"node-demux.cc",
        	"node-videodemux.cc",
        	"loadworker.cc",
        	"demuxworker.cc",
        	"seekworker.cc",
        	"demuxbaton.cc",
        	"videoframe.cc"
        ],
		'default_configuration': "Release",
		'conditions': [
			['OS=="linux"', 
				{
					'include_dirs' : [
						"<!(node -e \"require('nan')\")",
						"/usr/include/ffmpeg"
					],
					'libraries': [
						"-lavcodec",
						"-lavformat",
						"-lavutil",
						"-lswscale"

					],
					'cflags': [
						"-D__STDC_CONSTANT_MACROS"
					]
				},
            ],
			['OS=="mac"',
				{
					'include_dirs' : [
						"<!(node -e \"require('nan')\")",
						"/usr/local/include"
					],
					'libraries': [
						"-L/usr/local/lib",
						"-lavcodec",
						"-lavformat",
						"-lavutil",
						"-lswscale"
					]
				},
			],
			['OS=="win"',
				{
					'include_dirs': [
                        "<!(node -e \"require('nan')\")",
                        "C:/Dev/ffmpeg-win64-dev/include"
                    ],
                    'libraries': [
						"C:/Dev/ffmpeg-win64-dev/lib/avcodec.lib",
						"C:/Dev/ffmpeg-win64-dev/lib/avformat.lib",
						"C:/Dev/ffmpeg-win64-dev/lib/avutil.lib",
						"C:/Dev/ffmpeg-win64-dev/lib/swscale.lib"
					]
				},
			]
        ]
	}]
}
