var node_demux = require('./build/Release/node_demux');

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
	
	this.seek = function(frameIdx) {
		this.video.SeekVideo(frameIdx);
	};
	
	this.on = function(type, cb) {
		if (type === "error" || type == "metadata" || type === "start" || type === "end" || type === "frame") {
			this.video.On(type, cb);
		}
	};
}

module.exports = demux;
