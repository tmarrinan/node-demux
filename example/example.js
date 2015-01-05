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
    nframes = metadata.num_frames;
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
    console.log("received frame " + frameIdx + " (size: " + data.length + ")");
    if(frameIdx === 500) {
    	video.seek(800);
    }
    
    /*if(frameIdx >= 300 && frameIdx < 315) {
    	fs.writeFile("out/aframe_"+frameIdx+"."+format, data, function(err) {
    	
    	});
    }*/
});
video.load("skyfall_1080p.mp4");
//video.loadVideo("big-buck-bunny_trailer.webm");
video.seek(300);
video.play();