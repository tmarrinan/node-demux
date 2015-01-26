var node_demux = require('./build/Release/node_demux');

var messages = [
	"error",
	"metadata",
	"start",
	"end",
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
	
	this.pause = function(cb) {
		if(!this.video.IsBusy()){
			this.video.PauseDemuxing(cb);
		}
		else {
			var _this = this;
			setTimeout(function() {
				_this.pause(cb);
			}, 16);
		}
	};
	
	this.stop = function(cb) {
		if(!this.video.IsBusy()){
			this.video.StopDemuxing(cb);
		}
		else {
			var _this = this;
			setTimeout(function() {
				_this.stop(cb);
			}, 16);
		}
	};
	
	this.seek = function(timestamp, cb) {
		if(!this.video.IsBusy()){
			this.video.SeekVideo(timestamp, cb);
		}
		else {
			var _this = this;
			setTimeout(function() {
				_this.seek(timestamp, cb);
			}, 16);
		}
	};
	
	this.on = function(type, cb) {
		if (messages.indexOf(type) >= 0) {
			this.video.On(type, cb);
		}
	};
}

module.exports = demux;
