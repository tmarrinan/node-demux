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
					'link_settings': {
						'libraries': [
				
						],
					}
				},
			],
        ]
	}]
}