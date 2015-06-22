Pebble.addEventListener('ready', function(e) {
	//initialise here
});

Pebble.addEventListener('showConfiguration', function(e) {
	Pebble.openURL('https://dl.dropboxusercontent.com/u/6609866/StarfieldConfig.html');
});

Pebble.addEventListener("webviewclosed", function(e){
	var settings = JSON.parse(decodeURIComponent(e.response));
	Pebble.sendAppMessage(settings);
});