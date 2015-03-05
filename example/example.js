var fs    = require('fs');
var path  = require('path');

var demux = require('../../node-demux');


var video = new demux();

video.on('error', function(err) {
    console.log(err);
});
video.on('metadata', function(metadata) {
    console.log(metadata);
});
video.on('start', function() {
    console.log("start demuxing");
});
video.on('end', function() {
    console.log("finished demuxing");
});
video.on('frame', function(frameIdx, data) {
    console.log("received frame " + frameIdx + " (size: " + data.length + ")");
});

video.load(path.join(__dirname, "big-buck-bunny_trailer.mp4"), {decodeFirstFrame: true});
video.play();
setTimeout(function() {
	video.pause(function() {
		console.log("paused: 1.5 sec");
	});
}, 1500);
setTimeout(function() {
	video.play();
	console.log("play: 3.0 sec");
}, 3000);
setTimeout(function() {
	video.pause(function() {
		console.log("paused: 4.5 sec");
	});
}, 4500);
setTimeout(function() {
	video.play();
	console.log("play: 6.0 sec");
}, 6000);
setTimeout(function() {
	video.seek(13.2, function() {
		console.log("seeked: 7.5 sec");
		video.play();
		console.log("play: 7.5 sec");
	});
}, 7500);





/*
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
	
    //if(frameIdx === 100) {
    //	video.stop(function() {
	//		console.log("video stopped");
	//	});
    //}
});
video.load("big-buck-bunny_trailer.mp4", {decodeFirstFrame: true});
//video.seek(9.8, function() {
//	console.log("seek 1 complete");
//	video.play();
//});

setTimeout(function() {
	video.seek(9.8, function(){
		console.log("finished seek");
		setTimeout(function() {
			video.play();
		}, 1000);
	});
}, 1500);
*/
