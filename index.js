var node_demux = require('./build/Release/node_demux');

var messages = [
	"error",
	"metadata",
	"start",
	"end",
	"seek",
	"frame"
];

function demux() {
	this.video = new node_demux.VideoDemux();
	
	this.load = function(filename) {
		this.video.LoadVideo(filename);
	};
	
	this.play = function() {
		this.video.StartDemuxing();
	};
	
	this.pause = function() {
		this.video.PauseDemuxing();
	};
	
	this.stop = function() {
		this.video.StopDemuxing();
	};
	
	this.seek = function(timestamp) {
		this.video.SeekVideo(timestamp);
	};
	
	this.on = function(type, cb) {
		if (messages.indexOf(type) >= 0) {
			this.video.On(type, cb);
		}
	};
}

module.exports = demux;
