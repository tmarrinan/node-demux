node-demux
==========

### Usage ###
declare new demuxer: `var video = new demux();`

| Method                  | Use |
|-------------------------|-----|
| `video.load(filename)`  | loads video from file (`filename` is path to desired file) |
| `video.play()`          | starts demuxing video (callback for `'frame'` on each new decoded frame |
| `video.pause()`         | pauses demuxing |
| `video.stop()`          | stops demuxing (same as pause, but also seeks video back to start) |
| `video.seek(timestamp)` | seeks video to desired time in seconds (not 100% accurate - goes to nearest keyframe) |
| `video.on(message)`     | `message` is one of `'error'`, `'metadata'`, `'start'`, `'end'`, or `'frame'` |


### Example ###
```
var demux = require('node-demux');


var start;
var nframes;

var video = new demux();
video.on('error', function(err) {
    console.log(err);
});
video.on('metadata', function(metadata) {
	nframes = 0;
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
});
video.load("big-buck-bunny_trailer.mp4");
video.play();
```
