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
					'libraries': [
						"C:\local\dev\lib\libavcodec.dll.a",
						"C:\local\dev\lib\libavformat.dll.a",
						"C:\local\dev\lib\libavutil.dll.a"
					]
				},
			],
        ]
	}]
}