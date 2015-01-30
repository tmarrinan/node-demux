var fs    = require('fs');
var demux = require('../../node-demux');


var start;
var nframes;
var format;

var video = new demux();
video.on('error', function(err) {
    console.log(err);
});
video.on('metadata', function(metadata) {
	nframes = 0;
    format = metadata.pixel_format;
    console.log(metadata);
});
video.on('start', function() {
    start = Date.now();
    console.log("start demuxing");
});
video.on('end', function() {
    var time = (Date.now() - start) / 1000;
    console.log("finished demuxing");
    console.log("  total time: " + time.toFixed(3) + "sec");
    console.log("  average frames per second: " + (nframes/time).toFixed(3) + "fps");
});
video.on('frame', function(frameIdx, data) {
    nframes++;
    console.log("received frame " + frameIdx + " (size: " + data.length + ")");
	
	/*
    if(frameIdx === 100) {
    	video.stop(function() {
			console.log("video stopped");
		});
    }*/
});
video.load("big-buck-bunny_trailer.mp4", {decodeFirstFrame: true});
/*video.seek(9.8, function() {
	console.log("seek 1 complete");
	video.play();
});*/

setTimeout(function() {
	video.seek(9.8, function(){
		console.log("finished seek");
		setTimeout(function() {
			video.play();
		}, 1000);
	});
}, 1500);

