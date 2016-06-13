var fs    = require('fs');
var path  = require('path');

var demux = require('../../node-demux');


var video = new demux();
var frameCount = 0;

video.on('error', function(err) {
	console.log(err);
});
video.on('metadata', function(metadata) {
	console.log(metadata);    
});

video.on('end', function() {
	console.log("finished demuxing");
});

video.on('frame', function(frameIdx, data) {
	frameCount ++;
	console.log("received frame " + frameIdx + " (size: " + data.length + ")");

	video.nextFrame();
});

video.load(path.join(__dirname, "big-buck-bunny_trailer.mp4"), {'colorspace': 'rgb24'});
video.nextFrame();
